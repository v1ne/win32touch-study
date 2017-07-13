// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "ViewBase.h"

#include <map>
#include <list>
#include <vector>

#define MOUSE_CURSOR_ID 0

class CComTouchDriver {
public:
    CComTouchDriver(HWND hWnd);
    ~CComTouchDriver();
    
    // Initializes Core Objects, Manipulation Processors and Inertia Processors
    bool Initialize();

    // Processes the input information and activates the appropriate processor
    void ProcessInputEvent(const TOUCHINPUT* inData);

    // Sets up the initial state of the objects
    void RenderInitialState(const int iCWidth, const int iCHeight);

    void RunInertiaProcessorsAndRender();
        
    // Localizes point for high-DPI
    inline Point2F CComTouchDriver::LocalizePoint(Point2I p)
    {
        return Point2F(p) * mDpiScale;
    }

private:
    // Renders the objects to the screen
    void RenderObjects();

    // Event helpers for processing input events

    bool DownEvent(ViewBase* pViewBase, const TOUCHINPUT* inData);
    void MoveEvent(const TOUCHINPUT* inData);
    void UpEvent(const TOUCHINPUT* inData);

    // Map of cursor ids and core obejcts
    std::map<DWORD, ViewBase*> m_mCursorObject;
  
    // List of core objects to be manipulated
    std::list<ViewBase*> m_lCoreObjects;
    std::vector<ViewBase*> m_lCoreObjectsInInitialOrder;

    // The client width and height
    int m_iCWidth;
    int m_iCHeight;

    // Scale for converting between dpi's
    float mDpiScale = 1.0f;

    CD2DDriver* mD2dDriver;

    // Handle to window
    HWND mhWnd;
};
