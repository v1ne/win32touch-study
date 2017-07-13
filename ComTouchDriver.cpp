// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "ComTouchDriver.h"

#include "Slider.h"
#include "Square.h"

#include <manipulations.h>

#define NUM_CORE_OBJECTS 4

#define NUM_SLIDERS 33
#define NUM_KNOBS 25

CComTouchDriver::CComTouchDriver(HWND hWnd)
  : mhWnd(hWnd)
{
  const auto success = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
  assert(success);
}

bool CComTouchDriver::Initialize() {
  mPhysicalPointsPerLogicalPoint = ::GetDpiForWindow(mhWnd) / 96.f;

  mD2dDriver = new CD2DDriver(mhWnd);
  auto success = SUCCEEDED(mD2dDriver->Initialize());
  if (!success) return false;

  for(int i = 0; i < NUM_CORE_OBJECTS; i++) {
    mCoreObjects.push_front(new CSquare(mhWnd, mD2dDriver, CSquare::DrawingColor(i % 4)));
  }

  for (int i = 0; i < NUM_SLIDERS + 1; i++) {
    mCoreObjects.push_front(new CSlider(mhWnd, mD2dDriver, CSlider::TYPE_SLIDER,
      i < NUM_SLIDERS ? CSlider::InteractionMode(i % CSlider::NUM_MODES) : CSlider::MODE_RELATIVE));
  }

  for (int i = 0; i < NUM_KNOBS; i++) {
    mCoreObjects.push_front(new CSlider(mhWnd, mD2dDriver,
      CSlider::TYPE_KNOB, CSlider::InteractionMode(i % CSlider::NUM_MODES)));
  }

  mCoreObjectsInInitialOrder.reserve(mCoreObjects.size());
  mCoreObjectsInInitialOrder.insert(mCoreObjectsInInitialOrder.begin(), mCoreObjects.begin(), mCoreObjects.end());

  return success;
}

CComTouchDriver::~CComTouchDriver() {
  for(const auto& pObject: mCoreObjects)
    delete pObject;
  mCoreObjects.clear();

  delete mD2dDriver;

  CoUninitialize();
}

void CComTouchDriver::ProcessInputEvent(const TOUCHINPUT* pData) {
  DWORD dwCursorID = pData->dwID;
  DWORD dwEvent = pData->dwFlags;

  if(dwEvent & TOUCHEVENTF_DOWN) {
    for(const auto& pObject: mCoreObjects) {
      auto found = DownEvent(pObject, pData);
      if(found) break;
    }
  } else if(dwEvent & TOUCHEVENTF_MOVE) {
    MoveEvent(pData);
  } else if(dwEvent & TOUCHEVENTF_UP) {
    UpEvent(pData);
  }
}

bool CComTouchDriver::DownEvent(ViewBase* pView, const TOUCHINPUT* pData) {
  auto p = PhysicalToLogical({pData->x, pData->y});

  if(!pView->InRegion(p))
    return false;

  if(FAILED(pView->HandleTouchEvent(ViewBase::DOWN, p, pData)))
    return false;

  mCursorIdToObjectMap.insert(std::pair<DWORD, ViewBase*>(pData->dwID, pView));

  mCoreObjects.remove(pView);
  mCoreObjects.push_front(pView);

  RenderObjects(); // Renders objects to bring new object to front

  return true;
}

void CComTouchDriver::MoveEvent(const TOUCHINPUT* pData) {
  DWORD dwCursorID = pData->dwID;
  auto p = PhysicalToLogical({pData->x, pData->y});

  auto iEntry = mCursorIdToObjectMap.find(dwCursorID);
  if(iEntry != mCursorIdToObjectMap.end())
    iEntry->second->HandleTouchEvent(ViewBase::MOVE, p, pData);
}

void CComTouchDriver::UpEvent(const TOUCHINPUT* pData) {
  DWORD dwCursorID = pData->dwID;
  auto p = PhysicalToLogical({pData->x, pData->y});

  auto iEntry = mCursorIdToObjectMap.find(dwCursorID);
  if(iEntry != mCursorIdToObjectMap.end()) {
    iEntry->second->HandleTouchEvent(ViewBase::UP, p, pData);
    mCursorIdToObjectMap.erase(dwCursorID);
  }
}

void CComTouchDriver::RunInertiaProcessorsAndRender() {
  for(const auto& pObject: mCoreObjects)
    pObject->HandleTouchEvent(ViewBase::INERTIA, {}, nullptr);

  RenderObjects();
}

void CComTouchDriver::RenderObjects() {
  if(mD2dDriver->GetRenderTarget()->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)
    return;

  mD2dDriver->BeginDraw();
  mD2dDriver->RenderBackground(mPhysicalClientArea);

  for(auto iObject = mCoreObjects.rbegin(); iObject != mCoreObjects.rend(); ++iObject)
    (*iObject)->Paint();

  mD2dDriver->EndDraw();
}

void CComTouchDriver::RenderInitialState(Point2I physicalClientArea) {
  D2D1_SIZE_U size = {UINT32(physicalClientArea.x), UINT32(physicalClientArea.y)};

  if (FAILED(mD2dDriver->GetRenderTarget()->Resize(size)))
  {
    mD2dDriver->DiscardDeviceResources();
    InvalidateRect(mhWnd, NULL, FALSE);
    return;
  }

  mPhysicalClientArea = Point2F(physicalClientArea);

  auto clientArea = PhysicalToLogical(physicalClientArea);

  const auto squareSize = Point2F(200.f);
  const auto numSquareColumns = int(sqrt(NUM_CORE_OBJECTS));
  auto iObject = mCoreObjectsInInitialOrder.rbegin();
  for(int i = 0; i < NUM_CORE_OBJECTS; i++) {
    const auto pos = clientArea - squareSize.dot(
      Point2I{i % numSquareColumns + 1, i / numSquareColumns + 1});
    ((CSquare*)*iObject)->ResetState(pos, clientArea, squareSize);
    ++iObject;
  }

  const auto sliderBorder = Point2F{5};
  const auto sliderSize = Point2F{50, 200};
  const auto sliderDistance = sliderSize + sliderBorder;
  const auto numSliderColumns = int(sqrt(NUM_SLIDERS * sliderDistance.y / sliderDistance.x));
  for(int i = 0; i < NUM_SLIDERS; i++) {
    const auto pos = sliderBorder + sliderDistance.dot(
      Point2I{i % numSliderColumns, i / numSliderColumns});
    ((CSlider*)*iObject)->ResetState(pos, clientArea, sliderSize);
    ++iObject;
  }

  const auto pos = Point2F{sliderBorder.x + sliderDistance.x*11, sliderBorder.y};
  const auto bigSliderSize = Point2F{50, 3*sliderDistance.y - sliderBorder.y};
  ((CSlider*)*iObject)->ResetState(pos, clientArea, bigSliderSize);

  const auto knobBorder = Point2F{2.f};
  const auto knobSize = Point2F{50.f};
  const auto knobDistance = knobSize + knobBorder;
  const auto numKnobColumns = int(sqrt(NUM_KNOBS * knobDistance.y / knobDistance.x));
  for(int i = 0; i < NUM_KNOBS; i++) {
    ++iObject;
    const auto pos = Point2F{
      knobBorder.x + knobDistance.x * (i % numKnobColumns),
      clientArea.y - (knobBorder.y + knobDistance.y * (1 + i / numKnobColumns))};
    ((CSlider*)*iObject)->ResetState(pos, clientArea, knobSize);
  }

  RenderObjects();
}
