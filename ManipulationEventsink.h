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
  CManipulationEventSink::CManipulationEventSink(HWND hWnd, IManipulationCallbacks *pViewObject, bool inertia)
    : mpViewObject(pViewObject)
    , m_hWnd(hWnd)
    , m_iTimerId((int)(ptrdiff_t)this) // it's unique enough for a hack
    , m_bInertia(inertia)
    , m_pConnPoint(NULL)
    , m_cRefCount(1)
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
  HRESULT SetupInertia(float x, float y, IInertiaProcessor* ip, IManipulationProcessor* mp);
  
  IConnectionPoint* m_pConnPoint;
  volatile unsigned int m_cRefCount;
  DWORD m_uID;
  IManipulationCallbacks* mpViewObject;
  HWND m_hWnd;
  int m_iTimerId;
  bool m_bInertia;
};

#endif