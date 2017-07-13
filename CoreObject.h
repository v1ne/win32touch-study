// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#ifndef COREOBJECT_H
#define COREOBJECT_H

#include "ManipulationEventsink.h"
#include "DrawingObject.h"

class CCoreObject {
public:
  CCoreObject(HWND hwnd, int iTimerId, CD2DDriver* d2dDriver, bool canRotate);
  ~CCoreObject();
  bool Initialize(CDrawingObject* pDrawingObject);

  CDrawingObject* mpDrawingObject;

  CManipulationEventSink* mManipulationEventSink;
  CManipulationEventSink* mInertiaEventSink;

  IManipulationProcessor* mManipulationProc;
  IInertiaProcessor* mInertiaProc;

  // This flag is set by the mManipulation event sink
  // when the ManipulationCompleted method is invoked
  bool mIsInertiaActive;

private:
  HWND mhWnd;
  CD2DDriver* mD2dDriver;
  int mTimerId;
  bool mCanRotate;
};

#endif