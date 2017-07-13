// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) v1ne

#include "CoreObject.h"
#include "Square.h"
#include <manipulations_i.c>
#include <new>


bool gShiftPressed = false;

CCoreObject::CCoreObject(HWND hwnd, int iTimerId, CD2DDriver* d2dDriver, bool canRotate)
  : mhWnd(hwnd)
  , mTimerId(iTimerId)
  , mD2dDriver(d2dDriver)
  , mpDrawingObject(NULL)
  , mManipulationProc(NULL)
  , mInertiaProc(NULL)
  , mCanRotate(canRotate)
{ }

bool CCoreObject::Initialize(CDrawingObject* pDrawingObject) {
  assert(pDrawingObject);

  if(FAILED(CoCreateInstance(CLSID_ManipulationProcessor, NULL,
      CLSCTX_INPROC_SERVER, IID_IUnknown, (VOID**)(&mManipulationProc))))
    return false;

  if(FAILED(CoCreateInstance(CLSID_InertiaProcessor, NULL,
      CLSCTX_INPROC_SERVER, IID_IUnknown, (VOID**)(&mInertiaProc))))
    return false;

  if(!mCanRotate) {
    auto manipulations = MANIPULATION_PROCESSOR_MANIPULATIONS::MANIPULATION_ALL;
    // TODO: Besser lösen!
    //manipulations &= ~MANIPULATION_ROTATE;
    manipulations = MANIPULATION_ROTATE;
    mManipulationProc->put_SupportedManipulations(manipulations);
  }

  mpDrawingObject = pDrawingObject;

  mManipulationEventSink = new CManipulationEventSink(mhWnd, this, mTimerId, FALSE);
  if(!mManipulationEventSink->SetupConnPt(mManipulationProc))
    return false;

  mInertiaEventSink = new CManipulationEventSink(mhWnd, this, mTimerId, TRUE);
  if(!mInertiaEventSink->SetupConnPt(mInertiaProc))
    return false;

  mIsInertiaActive = FALSE;

  return true;
}

CCoreObject::~CCoreObject() {
  delete mpDrawingObject;

  mManipulationEventSink->RemoveConnPt();
  mManipulationEventSink->Release();
  mManipulationEventSink = NULL;

  mInertiaEventSink->RemoveConnPt();
  mInertiaEventSink->Release();
  mInertiaEventSink = NULL;

  mManipulationProc->Release();

  mInertiaProc->Release();
}
