// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#ifndef MANIPULATIONEVENTSINK_H
#define MANIPULATIONEVENTSINK_H

#include <ocidl.h>
#include <manipulations.h>

#define DESIRED_MILLISECONDS 10

// Forward declaration of CCoreObject
class CCoreObject;

class CManipulationEventSink : _IManipulationEvents
{
public:
    CManipulationEventSink::CManipulationEventSink(HWND hWnd, CCoreObject *coRef, int iTimerId, BOOL inertia): 
    m_coRef(coRef), 
    m_hWnd(hWnd), 
    m_iTimerId(iTimerId), 
    m_bInertia(inertia), 
    m_pConnPoint(NULL),
    m_cRefCount(1)
{
}


   
    // Handles event when the manipulation begins
    HRESULT STDMETHODCALLTYPE ManipulationStarted(FLOAT x, FLOAT y) override;

    // Handles event when the manipulation is progress
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

    // Handles event when the manipulation ends
    HRESULT STDMETHODCALLTYPE ManipulationCompleted(
        FLOAT x,
        FLOAT y,
        FLOAT cumulativeTranslationX,
        FLOAT cumulativeTranslationY,
        FLOAT cumulativeScale,
        FLOAT cumulativeExpansion,
        FLOAT cumulativeRotation) override;
    
    // Helper for creating a connection point
    BOOL SetupConnPt(IUnknown* manipulationProc);
    VOID RemoveConnPt();

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);

private:
    HRESULT SetupInertia(float x, float y, IInertiaProcessor* ip, IManipulationProcessor* mp);
    
    IConnectionPoint* m_pConnPoint;
    volatile unsigned int m_cRefCount;
    DWORD m_uID;
    
    // Reference to the object to manipulate
    CCoreObject* m_coRef;

    // Handle to the window
    HWND m_hWnd;

    // Unique timer identifer for this processor
    int m_iTimerId;

    // Flag the determines if this event sink handles inertia
    BOOL m_bInertia;
};

#endif