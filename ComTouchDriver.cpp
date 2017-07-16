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

#define NUM_CORE_OBJECTS 2

#define NUM_SLIDERS 14
#define NUM_KNOBS 15

CComTouchDriver::CComTouchDriver(HWND hWnd)
  : mhWnd(hWnd)
{
  const auto success = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
#ifdef _DEBUG
  assert(success);
#else
  UNREFERENCED_PARAMETER(success);
#endif
}

bool CComTouchDriver::Initialize() {
  mPhysicalPointsPerLogicalPoint = ::GetDpiForWindow(mhWnd) / 96.f;

  mD2dDriver = new CD2DDriver(mhWnd);
  auto success = SUCCEEDED(mD2dDriver->Initialize());
  if (!success) return false;

  for(int i = 0; i < NUM_CORE_OBJECTS; i++) {
    mCoreObjects.push_front(new CSquare(mhWnd, mD2dDriver, CSquare::DrawingColor(i % 4)));
  }

  uint8_t numController = 0;

  for (int i = 0; i < NUM_SLIDERS + 1; i++) {
    mCoreObjects.push_front(new CSlider(mhWnd, mD2dDriver, CSlider::TYPE_SLIDER, numController++));
  }

  for (int i = 0; i < NUM_KNOBS; i++) {
    mCoreObjects.push_front(new CSlider(mhWnd, mD2dDriver, CSlider::TYPE_KNOB, numController++));
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
  auto cursorId = pData->dwID;

  // Skip spurious mouse events if there are touch valid points
  if (cursorId == MOUSE_CURSOR_ID && mNumTouchContacts)
    return;

  auto flags = pData->dwFlags;
  if(flags & TOUCHEVENTF_DOWN) {
    if (cursorId != MOUSE_CURSOR_ID)
      mNumTouchContacts++;

    for(const auto& pObject: mCoreObjects) {
      auto found = DownEvent(pObject, pData);
      if(found) break;
    }
  } else if(flags & TOUCHEVENTF_MOVE) {
    MoveEvent(pData);
  } else if(flags & TOUCHEVENTF_UP) {
    if (cursorId != MOUSE_CURSOR_ID)
      mNumTouchContacts--;

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

  ::InvalidateRect(mhWnd, NULL, FALSE);
  return true;
}

void CComTouchDriver::MoveEvent(const TOUCHINPUT* pData) {
  DWORD cursorId = pData->dwID;
  auto p = PhysicalToLogical({pData->x, pData->y});

  auto iEntry = mCursorIdToObjectMap.find(cursorId);
  if(iEntry != mCursorIdToObjectMap.end())
    iEntry->second->HandleTouchEvent(ViewBase::MOVE, p, pData);
}

void CComTouchDriver::UpEvent(const TOUCHINPUT* pData) {
  DWORD cursorId = pData->dwID;
  auto p = PhysicalToLogical({pData->x, pData->y});

  auto iEntry = mCursorIdToObjectMap.find(cursorId);
  if(iEntry != mCursorIdToObjectMap.end()) {
    iEntry->second->HandleTouchEvent(ViewBase::UP, p, pData);
    mCursorIdToObjectMap.erase(cursorId);
  }
}

void CComTouchDriver::RunInertiaProcessorsAndRender() {
  for(const auto& pObject: mCoreObjects)
    pObject->HandleTouchEvent(ViewBase::INERTIA, {}, nullptr);

  ::InvalidateRect(mhWnd, NULL, FALSE);
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
    const auto pos = clientArea - squareSize.mulByComponent(
      Point2I{i % numSquareColumns + 1, i / numSquareColumns + 1});
    ((CSquare*)*iObject)->ResetState(pos, clientArea, squareSize);
    ++iObject;
  }

  const auto sliderBorder = Point2F{5};
  const auto sliderSize = Point2F{50, 200};
  const auto sliderDistance = sliderSize + sliderBorder;
  const auto numSliderColumns = int(sqrt(NUM_SLIDERS * sliderDistance.y / sliderDistance.x));
  for(int i = 0; i < NUM_SLIDERS; i++) {
    const auto pos = sliderBorder + sliderDistance.mulByComponent(
      Point2I{i % numSliderColumns, i / numSliderColumns});
    ((CSlider*)*iObject)->ResetState(pos, clientArea, sliderSize);
    ++iObject;
  }

  const auto bigSliderPos = Point2F{sliderBorder.x + sliderDistance.x*7, sliderBorder.y};
  const auto bigSliderSize = Point2F{50, 2*sliderDistance.y - sliderBorder.y};
  ((CSlider*)*iObject)->ResetState(bigSliderPos, clientArea, bigSliderSize);

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

  InvalidateRect(mhWnd, NULL, FALSE);
}
