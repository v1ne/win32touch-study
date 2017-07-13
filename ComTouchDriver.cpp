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

void CComTouchDriver::ProcessInputEvent(const TOUCHINPUT* inData) {
  DWORD dwCursorID = inData->dwID;
  DWORD dwEvent = inData->dwFlags;

  if(dwEvent & TOUCHEVENTF_DOWN) {
    for(const auto& pObject: mCoreObjects) {
      auto found = DownEvent(pObject, inData);
      if(found) break;
    }
  } else if(dwEvent & TOUCHEVENTF_MOVE) {
    MoveEvent(inData);
  } else if(dwEvent & TOUCHEVENTF_UP) {
    UpEvent(inData);
  }
}

bool CComTouchDriver::DownEvent(ViewBase* pView, const TOUCHINPUT* inData) {
  DWORD dwCursorID = inData->dwID;
  auto p = PhysicalToLogicalPoint({inData->x, inData->y});

  if(!pView->InRegion(p)) {
    return false;
  }

  if(FAILED(pView->mManipulationProc->ProcessDownWithTime(dwCursorID, p.x, p.y, inData->dwTime))) {
    return false;
  }

  mCursorIdToObjectMap.insert(std::pair<DWORD, ViewBase*>(dwCursorID, pView));

  mCoreObjects.remove(pView);
  mCoreObjects.push_front(pView);

  RenderObjects(); // Renders objects to bring new object to front

  return true;
}

void CComTouchDriver::MoveEvent(const TOUCHINPUT* inData) {
  DWORD dwCursorID = inData->dwID;
  auto p = PhysicalToLogicalPoint({inData->x, inData->y});

  auto iEntry = mCursorIdToObjectMap.find(dwCursorID);
  if(iEntry != mCursorIdToObjectMap.end())
    iEntry->second->mManipulationProc->ProcessMoveWithTime(dwCursorID, p.x, p.y, inData->dwTime);
}

void CComTouchDriver::UpEvent(const TOUCHINPUT* inData) {
  DWORD dwCursorID = inData->dwID;
  auto p = PhysicalToLogicalPoint({inData->x, inData->y});

  auto iEntry = mCursorIdToObjectMap.find(dwCursorID);
  if(iEntry != mCursorIdToObjectMap.end()) {
    iEntry->second->mManipulationProc->ProcessUpWithTime(dwCursorID, p.x, p.y, inData->dwTime);
    mCursorIdToObjectMap.erase(dwCursorID);
  }
}

void CComTouchDriver::RunInertiaProcessorsAndRender() {
  for(const auto& pObject: mCoreObjects) {
    if(pObject->mIsInertiaActive == TRUE) {
      BOOL bCompleted = FALSE;
      pObject->mInertiaProc->Process(&bCompleted);
    }
  }

  RenderObjects();
}

void CComTouchDriver::RenderObjects() {
  if(mD2dDriver->GetRenderTarget()->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)
    return;

  mD2dDriver->BeginDraw();
  mD2dDriver->RenderBackground((FLOAT)m_iCWidth, (FLOAT)m_iCHeight);

  for(auto iObject = mCoreObjects.rbegin(); iObject != mCoreObjects.rend(); ++iObject)
    (*iObject)->Paint();

  mD2dDriver->EndDraw();
}

void CComTouchDriver::RenderInitialState(const int iCWidth, const int iCHeight) {
  D2D1_SIZE_U size = {UINT32(iCWidth), UINT32(iCHeight)};
  if (FAILED(mD2dDriver->GetRenderTarget()->Resize(size)))
  {
    mD2dDriver->DiscardDeviceResources();
    InvalidateRect(mhWnd, NULL, FALSE);
    return;
  }

  m_iCWidth = iCWidth;
  m_iCHeight = iCHeight;

  auto unscaledClientArea = Point2F(Point2I(iCWidth, iCHeight));
  auto clientArea = PhysicalToLogicalPoint({iCWidth, iCHeight});

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
