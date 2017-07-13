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
    void RenderInitialState(Point2I physicalClientArea);

    void RunInertiaProcessorsAndRender();
        
    inline Point2F PhysicalToLogical(Point2I p)
    {
        return Point2F(p) / mPhysicalPointsPerLogicalPoint;
    }

private:
    // Renders the objects to the screen
    void RenderObjects();

    // Event helpers for processing input events

    bool DownEvent(ViewBase* pViewBase, const TOUCHINPUT* inData);
    void MoveEvent(const TOUCHINPUT* inData);
    void UpEvent(const TOUCHINPUT* inData);

    unsigned int mNumTouchContacts = 0;
    std::map<DWORD, ViewBase*> mCursorIdToObjectMap;
  
    // List of core objects to be manipulated
    std::list<ViewBase*> mCoreObjects;
    std::vector<ViewBase*> mCoreObjectsInInitialOrder;

    Point2F mPhysicalClientArea;

    float mPhysicalPointsPerLogicalPoint = 1.0f;

    CD2DDriver* mD2dDriver;

    // Handle to window
    HWND mhWnd;
};
