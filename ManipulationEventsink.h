// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#ifndef MANIPULATIONEVENTSINK_H
#define MANIPULATIONEVENTSINK_H

#include "ManipulationCallbacks.h"

#include <ocidl.h>
#include <manipulations.h>

#define DESIRED_MILLISECONDS 10


class CManipulationEventSink : _IManipulationEvents {
public:
  CManipulationEventSink::CManipulationEventSink(
    HWND hWnd, IManipulationCallbacks *pViewObject,
    IManipulationProcessor* pManipulationProcessor, IInertiaProcessor* pInertiaProcessor)
    : mpViewObject(pViewObject)
    , mpManipulationProcessor(pManipulationProcessor)
    , mpInertiaProcessor(pInertiaProcessor)
    , mhWnd(hWnd)
    , mTimerId((int)(ptrdiff_t)this) // it's unique enough for a hack
    , mpConnPoint(NULL)
    , mRefCount(1)
  {
  }
   
  HRESULT STDMETHODCALLTYPE ManipulationStarted(FLOAT x, FLOAT y) override;

  HRESULT STDMETHODCALLTYPE ManipulationDelta(
      FLOAT x,
      FLOAT y,
      FLOAT translationDeltaX,
      FLOAT translationDeltaY,
      FLOAT scaleDelta,
      FLOAT expansionDelta,
      FLOAT rotationDelta,
      FLOAT cumulativeTranslationX,
      FLOAT cumulativeTranslationY,
      FLOAT cumulativeScale,
      FLOAT cumulativeExpansion,
      FLOAT cumulativeRotation) override;

  HRESULT STDMETHODCALLTYPE ManipulationCompleted(
      FLOAT x,
      FLOAT y,
      FLOAT cumulativeTranslationX,
      FLOAT cumulativeTranslationY,
      FLOAT cumulativeScale,
      FLOAT cumulativeExpansion,
      FLOAT cumulativeRotation) override;
  
  bool SetupConnPt(IUnknown* manipulationProc);
  void RemoveConnPt();

  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();
  STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);

private:
  HRESULT SetupInertia(float x, float y);
  
  IManipulationCallbacks* mpViewObject;
  IManipulationProcessor* mpManipulationProcessor; // nullptr for sink on an inertia processor
  IInertiaProcessor* mpInertiaProcessor;
  HWND mhWnd;
  int mTimerId;
  IConnectionPoint* mpConnPoint;
  volatile unsigned int mRefCount;
  DWORD mID;
};

#endif