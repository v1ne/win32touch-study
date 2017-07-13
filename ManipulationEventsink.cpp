// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "ManipulationEventsink.h"

#include <math.h>

HRESULT STDMETHODCALLTYPE CManipulationEventSink::ManipulationStarted(FLOAT x, FLOAT y) {
  UNREFERENCED_PARAMETER(x);
  UNREFERENCED_PARAMETER(y);

  // Stop object if it is in the state of inertia

  mpViewObject->mIsInertiaActive = FALSE;
  KillTimer(mhWnd, mTimerId);

  mpViewObject->ManipulationStarted({x, y});

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CManipulationEventSink::ManipulationDelta(
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
    FLOAT cumulativeRotation) {
  UNREFERENCED_PARAMETER(cumulativeRotation);
  UNREFERENCED_PARAMETER(cumulativeExpansion);
  UNREFERENCED_PARAMETER(cumulativeScale);
  UNREFERENCED_PARAMETER(cumulativeTranslationX);
  UNREFERENCED_PARAMETER(cumulativeTranslationY);
  UNREFERENCED_PARAMETER(expansionDelta);

  const auto IsInertiaProcessor = !mpManipulationProcessor;

  mpViewObject->ManipulationDelta({{x, y},
    {translationDeltaX, translationDeltaY},
    scaleDelta, expansionDelta, rotationDelta,
    {cumulativeTranslationX, cumulativeTranslationY},
    cumulativeScale, cumulativeExpansion, cumulativeRotation, IsInertiaProcessor});

  HRESULT hr = S_OK;
   if(!IsInertiaProcessor) {
      const auto pivotPoint = mpViewObject->PivotPoint();
      HRESULT hrPPX = mpManipulationProcessor->put_PivotPointX(pivotPoint.x);
      HRESULT hrPPY = mpManipulationProcessor->put_PivotPointY(pivotPoint.y);
      HRESULT hrPR  = mpManipulationProcessor->put_PivotRadius(mpViewObject->PivotRadius());

      if(FAILED(hrPPX) || FAILED(hrPPY) || FAILED(hrPR))
      {
          hr = E_FAIL;
      }
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE CManipulationEventSink::ManipulationCompleted(
    FLOAT x,
    FLOAT y,
    FLOAT cumulativeTranslationX,
    FLOAT cumulativeTranslationY,
    FLOAT cumulativeScale,
    FLOAT cumulativeExpansion,
    FLOAT cumulativeRotation)
{
  UNREFERENCED_PARAMETER(cumulativeRotation);
  UNREFERENCED_PARAMETER(cumulativeExpansion);
  UNREFERENCED_PARAMETER(cumulativeScale);
  UNREFERENCED_PARAMETER(cumulativeTranslationX);
  UNREFERENCED_PARAMETER(cumulativeTranslationY);
  UNREFERENCED_PARAMETER(x);
  UNREFERENCED_PARAMETER(y);

  HRESULT hr = S_OK;
  if(mpManipulationProcessor)
  {
      HRESULT hrSI = SetupInertia(x, y);
      HRESULT hrCO = S_OK;

      if(FAILED(hrSI) || FAILED(hrCO))
      {
          hr = E_FAIL;
      }

      mpViewObject->mIsInertiaActive = TRUE;

      SetTimer(mhWnd, mTimerId, DESIRED_MILLISECONDS, NULL);
  }
  else
  {
      mpViewObject->mIsInertiaActive = FALSE;

      mpViewObject->ManipulationCompleted({{x, y},
        {cumulativeTranslationX, cumulativeTranslationY},
        cumulativeScale, cumulativeExpansion, cumulativeRotation});

      KillTimer(mhWnd, mTimerId);
  }

  return hr;
}

HRESULT CManipulationEventSink::SetupInertia(float x, float y) {
  HRESULT hr = S_OK;

  HRESULT hrPutDD = mpInertiaProcessor->put_DesiredDeceleration(0.003f); // px/ms^2
  HRESULT hrPutDAD = mpInertiaProcessor->put_DesiredAngularDeceleration(0.000015f); // rad/ms^2

  HRESULT hrPutIOX = mpInertiaProcessor->put_InitialOriginX(x);
  HRESULT hrPutIOY = mpInertiaProcessor->put_InitialOriginY(y);

  FLOAT fVX, fVY, fVR;
  HRESULT hrPutVX = mpManipulationProcessor->GetVelocityX(&fVX);
  HRESULT hrGetVY = mpManipulationProcessor->GetVelocityY(&fVY);
  HRESULT hrGetAV = mpManipulationProcessor->GetAngularVelocity(&fVR);

  HRESULT hrPutIVX = mpInertiaProcessor->put_InitialVelocityX(fVX);
  HRESULT hrPutIVY = mpInertiaProcessor->put_InitialVelocityY(fVY);
  HRESULT hrPutIAV = mpInertiaProcessor->put_InitialAngularVelocity(fVR);

  if(FAILED(hrPutDD) || FAILED(hrPutDAD) || FAILED(hrPutIOX) || FAILED(hrPutIOY)
      || FAILED(hrPutVX) || FAILED(hrGetVY) || FAILED(hrGetAV) || FAILED(hrPutIVX)
      || FAILED(hrPutIVY) || FAILED(hrPutIAV))
  {
      hr = E_FAIL;
  }

  return hr;
}

ULONG CManipulationEventSink::AddRef()
{
  return ++mRefCount;
}

ULONG CManipulationEventSink::Release()
{
  mRefCount--;

  if(mRefCount == 0) {
    delete this;
    return 0;
  } else
    return mRefCount;
}

HRESULT CManipulationEventSink::QueryInterface(REFIID riid, LPVOID *ppvObj) {
  HRESULT hr = S_OK;

  if(ppvObj == NULL) {
    hr = E_POINTER;
  } else {
    *ppvObj = NULL;

    if (IID__IManipulationEvents == riid)
      *ppvObj = static_cast<_IManipulationEvents*>(this);
    else if (IID_IUnknown == riid)
      *ppvObj = static_cast<IUnknown*>(this);

    if(*ppvObj)
      AddRef();
    else
      hr = E_NOINTERFACE;
  }

  return hr;
}

// Set up the connection to a mManipulation or inertia processor
bool CManipulationEventSink::SetupConnPt(IUnknown *mManipulationProc) {
  if (mpConnPoint != NULL) {
    return false;
  }

  BOOL success = FALSE;
  IConnectionPointContainer* pConPointContainer = NULL;
  // Check if supports connectable objects
  success = SUCCEEDED(mManipulationProc->QueryInterface(IID_IConnectionPointContainer,
    (LPVOID*)&(pConPointContainer)));

  // Get connection point interface
  if(success) {
    success = SUCCEEDED(pConPointContainer->FindConnectionPoint(
        _uuidof(_IManipulationEvents),
        &(mpConnPoint)));
  }

  // Clean up connection point container
  if (pConPointContainer != NULL) {
    pConPointContainer->Release();
    pConPointContainer = NULL;
  }

  // Hook event object to the connection point
  IUnknown* pUnk = NULL;
  if(success) {
    // Get pointer to mManipulation event sink's IUnknown pointer
    success = SUCCEEDED(QueryInterface(IID_IUnknown, (LPVOID*)&pUnk));
  }

  // Establish connection point to callback interface
  if(success) {
    success = SUCCEEDED(mpConnPoint->Advise(pUnk, &(mID)));
  }

  // Clean up IUnknown pointer
  if(pUnk != NULL) {
    pUnk->Release();
  }

  if (!success && mpConnPoint != NULL)
  {
    mpConnPoint->Release();
    mpConnPoint = NULL;
  }

  return success;
}

void CManipulationEventSink::RemoveConnPt() {
  if(!mpConnPoint)
    return;

  mpConnPoint->Unadvise(mID);
  mpConnPoint->Release();
  mpConnPoint = NULL;
}