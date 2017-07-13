// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "ComTouchDriver.h"

#include "Slider.h"
#include "Square.h"

#define NUM_CORE_OBJECTS 4
#define DEFAULT_PPI 96.0f

#define NUM_SLIDERS 33
#define NUM_KNOBS 25

CComTouchDriver::CComTouchDriver(HWND hWnd)
  : mhWnd(hWnd)
{
  const auto success = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
  assert(success);
}

bool CComTouchDriver::Initialize() {
  mDpiScale = float(DEFAULT_PPI)/::GetDpiForWindow(mhWnd);

  mD2dDriver = new (std::nothrow) CD2DDriver(mhWnd);
  auto success = SUCCEEDED(mD2dDriver->Initialize());
  if (!success) return false;

  for(int i = 0; i < NUM_CORE_OBJECTS; i++) {
    auto object = new (std::nothrow) CCoreObject(mhWnd, i, mD2dDriver, i == 0);
    success = object->Initialize(new (std::nothrow) CSquare(mhWnd, mD2dDriver, CSquare::DrawingColor(i % 4)));
    if (!success) return false;

    m_lCoreObjects.push_front(object);
  }

  for (int i = 0; i < NUM_SLIDERS + 1; i++) {
    auto pObject = new (std::nothrow) CCoreObject(mhWnd, i, mD2dDriver, false);
    success = pObject->Initialize(new (std::nothrow) CSlider(mhWnd, mD2dDriver,
      CSlider::TYPE_SLIDER,
      i < NUM_SLIDERS ? CSlider::InteractionMode(i % CSlider::NUM_MODES) : CSlider::MODE_RELATIVE));
    if (!success) return false;
    m_lCoreObjects.push_front(pObject);
  }

  for (int i = 0; i < NUM_KNOBS; i++) {
    auto pObject = new (std::nothrow) CCoreObject(mhWnd, i, mD2dDriver, false);
    auto success = pObject->Initialize(new (std::nothrow) CSlider(mhWnd, mD2dDriver,
      CSlider::TYPE_KNOB, CSlider::InteractionMode(i % CSlider::NUM_MODES)));
    if (!success) return false;
    m_lCoreObjects.push_front(pObject);
  }

  m_lCoreObjectsInInitialOrder.reserve(m_lCoreObjects.size());
  m_lCoreObjectsInInitialOrder.insert(m_lCoreObjectsInInitialOrder.begin(), m_lCoreObjects.begin(), m_lCoreObjects.end());

  return success;
}

CComTouchDriver::~CComTouchDriver() {
  for(const auto& pObject: m_lCoreObjects)
    delete pObject;
  m_lCoreObjects.clear();

  delete mD2dDriver;

  CoUninitialize();
}

void CComTouchDriver::ProcessInputEvent(const TOUCHINPUT* inData) {
  DWORD dwCursorID = inData->dwID;
  DWORD dwEvent = inData->dwFlags;

  if(dwEvent & TOUCHEVENTF_DOWN) {
    for(const auto& pObject: m_lCoreObjects) {
      auto found = DownEvent(pObject, inData);
      if(found) break;
    }
  } else if(dwEvent & TOUCHEVENTF_MOVE) {
    if(m_mCursorObject.count(inData->dwID) > 0)
      MoveEvent(inData);
  } else if(dwEvent & TOUCHEVENTF_UP) {
    if(m_mCursorObject.count(inData->dwID) > 0) {
      UpEvent(inData);
    }
  }
}

bool CComTouchDriver::DownEvent(CCoreObject* coRef, const TOUCHINPUT* inData) {
  DWORD dwCursorID = inData->dwID;
  DWORD dwPTime = inData->dwTime;
  auto p = LocalizePoint({inData->x, inData->y});

  // Check that the user has touched within an objects region and feed to the objects manipulation processor

  if(!coRef->mpDrawingObject->InRegion(p)) {
    return false;
  }

  if(FAILED(coRef->mManipulationProc->ProcessDownWithTime(dwCursorID, p.x, p.y, dwPTime))) {
    return false;
  }

  m_mCursorObject.insert(std::pair<DWORD, CCoreObject*>(dwCursorID, coRef));

  m_lCoreObjects.remove(coRef);
  m_lCoreObjects.push_front(coRef);

  RenderObjects(); // Renders objects to bring new object to front

  return true;
}

void CComTouchDriver::MoveEvent(const TOUCHINPUT* inData) {
  DWORD dwCursorID  = inData->dwID;
  auto p = LocalizePoint({inData->x, inData->y});

  auto iEntry = m_mCursorObject.find(dwCursorID);
  if(iEntry != m_mCursorObject.end())
    iEntry->second->mManipulationProc->ProcessMoveWithTime(dwCursorID, p.x, p.y, inData->dwTime);
}

void CComTouchDriver::UpEvent(const TOUCHINPUT* inData) {
  DWORD dwCursorID = inData->dwID;
  auto p = LocalizePoint({inData->x, inData->y});

  auto iEntry = m_mCursorObject.find(dwCursorID);
  if(iEntry != m_mCursorObject.end()) {
    iEntry->second->mManipulationProc->ProcessUpWithTime(dwCursorID, p.x, p.y, inData->dwTime);
    m_mCursorObject.erase(dwCursorID);
  }
}

void CComTouchDriver::RunInertiaProcessorsAndRender() {
  for(const auto& pObject: m_lCoreObjects) {
    if(pObject->mIsInertiaActive == TRUE) {
      BOOL bCompleted = FALSE;
      pObject->mInertiaProc->Process(&bCompleted);
    }
  }

  RenderObjects();
}

void CComTouchDriver::RenderObjects() {
  mD2dDriver->BeginDraw();
  mD2dDriver->RenderBackground((FLOAT)m_iCWidth, (FLOAT)m_iCHeight);

  for(auto iObject = m_lCoreObjects.rbegin(); iObject != m_lCoreObjects.rend(); ++iObject)
    (*iObject)->mpDrawingObject->Paint();

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
  auto clientArea = LocalizePoint({iCWidth, iCHeight});

  const auto squareSize = Point2F(200.f);
  const auto numSquareColumns = int(sqrt(NUM_CORE_OBJECTS));
  auto iObject = m_lCoreObjectsInInitialOrder.rbegin();
  for(int i = 0; i < NUM_CORE_OBJECTS; i++) {
    const auto pos = clientArea - squareSize.dot(
      Point2I{i % numSquareColumns + 1, i / numSquareColumns + 1});
    ((CSquare*)(*iObject)->mpDrawingObject)->ResetState(pos, clientArea, squareSize);
    ++iObject;
  }

  const auto sliderBorder = Point2F{5};
  const auto sliderSize = Point2F{50, 200};
  const auto sliderDistance = sliderSize + sliderBorder;
  const auto numSliderColumns = int(sqrt(NUM_SLIDERS * sliderDistance.y / sliderDistance.x));
  for(int i = 0; i < NUM_SLIDERS; i++) {
    const auto pos = sliderBorder + sliderDistance.dot(
      Point2I{i % numSliderColumns, i / numSliderColumns});
    ((CSlider*)(*iObject)->mpDrawingObject)->ResetState(pos, clientArea, sliderSize);
    ++iObject;
  }

  const auto pos = Point2F{sliderBorder.x + sliderDistance.x*11, sliderBorder.y};
  const auto bigSliderSize = Point2F{50, 3*sliderDistance.y - sliderBorder.y};
  ((CSlider*)(*iObject)->mpDrawingObject)->ResetState(pos, clientArea, bigSliderSize);

  const auto knobBorder = Point2F{2.f};
  const auto knobSize = Point2F{50.f};
  const auto knobDistance = knobSize + knobBorder;
  const auto numKnobColumns = int(sqrt(NUM_KNOBS * knobDistance.y / knobDistance.x));
  for(int i = 0; i < NUM_KNOBS; i++) {
    ++iObject;
    const auto pos = Point2F{
      knobBorder.x + knobDistance.x * (i % numKnobColumns),
      clientArea.y - (knobBorder.y + knobDistance.y * (1 + i / numKnobColumns))};
    ((CSlider*)(*iObject)->mpDrawingObject)->ResetState(pos, clientArea, knobSize);
  }

  RenderObjects();
}
