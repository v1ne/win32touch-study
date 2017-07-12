// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "ComTouchDriver.h"

#include "Slider.h"
#include "Square.h"

#define NUM_SLIDERS 33
#define NUM_KNOBS 25

CComTouchDriver::CComTouchDriver(HWND hWnd):
    m_hWnd(hWnd),
    m_uNumContacts(0),
    m_dpiScaleX(1.0f),
    m_dpiScaleY(1.0f)
{
}

BOOL CComTouchDriver::Initialize()
{
    BOOL success = TRUE;

    // Calculate dpi for high-DPI systems

    HDC hdcScreen = GetDC(m_hWnd);

    if(hdcScreen)
    {
        // Direct2D automatically does work in logical, so compute the
        // scale to convert from physical to logical coordinates

        m_dpiScaleX = (FLOAT)(DEFAULT_PPI / GetDeviceCaps(hdcScreen, LOGPIXELSX));
        m_dpiScaleY = (FLOAT)(DEFAULT_PPI / GetDeviceCaps(hdcScreen, LOGPIXELSY));
        DeleteDC(hdcScreen);
    }

    // Create and initialize D2DDriver

    m_d2dDriver = new (std::nothrow) CD2DDriver(m_hWnd);
    if(m_d2dDriver == NULL)
    {
        success = FALSE;
    }

    if(success)
    {
        success = SUCCEEDED(m_d2dDriver->Initialize());
    }

    // Create and initialize core objects

    if(success)
    {
        for(int i = 0; i < NUM_CORE_OBJECTS; i++)
        {
            CCoreObject* object = NULL;

            object = new (std::nothrow) CCoreObject(m_hWnd, i, m_d2dDriver, i == 0);

            if(object == NULL)
            {
                success = FALSE;
            }

            if(success)
            {
                success = object->Initialize(new (std::nothrow) CSquare(m_hWnd, m_d2dDriver));
            }

            // Append core object to the list
            if(success)
            {
                try
                {
                    m_lCoreObjects.push_front(object);
                }
                catch(std::bad_alloc)
                {
                    success = FALSE;
                }
            }

            // Clean up and leave if initialization failed
            if(!success)
            {
                if(object)
                {
                    delete object;
                }
                break;
            }
        }

        for (int i = 0; i < NUM_SLIDERS + 1; i++)
        {
          auto pObject = new (std::nothrow) CCoreObject(m_hWnd, i, m_d2dDriver, false);
          pObject->Initialize(new (std::nothrow) CSlider(m_hWnd, m_d2dDriver,
            CSlider::TYPE_SLIDER, CSlider::InteractionMode(i % CSlider::NUM_MODES)));
          m_lCoreObjects.push_front(pObject);
        }

        for (int i = 0; i < NUM_KNOBS; i++)
        {
          auto pObject = new (std::nothrow) CCoreObject(m_hWnd, i, m_d2dDriver, false);
          pObject->Initialize(new (std::nothrow) CSlider(m_hWnd, m_d2dDriver,
            CSlider::TYPE_KNOB, CSlider::InteractionMode(i % CSlider::NUM_MODES)));
          m_lCoreObjects.push_front(pObject);
        }
    }
    return success;
}

CComTouchDriver::~CComTouchDriver()
{
    std::list<CCoreObject*>::iterator it;

    // Clean up all core objects in the list
    for(it = m_lCoreObjects.begin(); it != m_lCoreObjects.end(); ++it)
    {
        if(*it)
        {
            delete *it;
        }
    }

    m_lCoreObjects.clear();

    // Clean up d2d driver
    if(m_d2dDriver)
    {
        delete m_d2dDriver;
    }
}

VOID CComTouchDriver::ProcessInputEvent(const TOUCHINPUT* inData)
{
    DWORD dwCursorID = inData->dwID;
    DWORD dwEvent = inData->dwFlags;
    BOOL bFoundObj = FALSE;	


    // Check if contacts should be incremented
    if((dwEvent & TOUCHEVENTF_DOWN) && (dwCursorID != MOUSE_CURSOR_ID))
    {
        m_uNumContacts++;
    }

    // Screen the types of inputs and the number of contacts
    if((m_uNumContacts == 0) && (dwCursorID != MOUSE_CURSOR_ID))
    {
        return;
    }
    else if((m_uNumContacts > 0) && (dwCursorID == MOUSE_CURSOR_ID))
    {
        return;
    }

    // Check if contacts should be decremented
    if((dwEvent & TOUCHEVENTF_UP) && (dwCursorID != MOUSE_CURSOR_ID))
    {
        m_uNumContacts--;
    }

    // Find the object and associate the cursor id with the object
    if(dwEvent & TOUCHEVENTF_DOWN)
    {
        std::list<CCoreObject*>::iterator it;
        for(it = m_lCoreObjects.begin(); it != m_lCoreObjects.end(); it++)
        {
            DownEvent(*it, inData, &bFoundObj);
            if(bFoundObj) break;
        }
    }
    else if(dwEvent & TOUCHEVENTF_MOVE)
    {
        if(m_mCursorObject.count(inData->dwID) > 0)
        {
            MoveEvent(inData);
        }
    }
    else if(dwEvent & TOUCHEVENTF_UP)
    {
        if(m_mCursorObject.count(inData->dwID) > 0)
        {
            UpEvent(inData);
        }
    }
}

// The subsequent methods are helpers for processing the event input

VOID CComTouchDriver::DownEvent(CCoreObject* coRef, const TOUCHINPUT* inData, BOOL* bFound)
{
    DWORD dwCursorID = inData->dwID;
    DWORD dwPTime = inData->dwTime;
    int x = GetLocalizedPointX(inData->x);
    int y = GetLocalizedPointY(inData->y);
    BOOL success = TRUE;

    // Check that the user has touched within an objects region and feed to the objects manipulation processor

    if(coRef->doDrawing->InRegion(x, y))
    {
        // Feed values to the Manipulation Processor
        success = SUCCEEDED(coRef->manipulationProc->ProcessDownWithTime(dwCursorID, (FLOAT)x, (FLOAT)y, dwPTime));

        if(success)
        {
            try
            {
                // Add to the cursor id -> object mapping
                m_mCursorObject.insert(std::pair<DWORD, CCoreObject*>(dwCursorID, coRef));
            }
            catch(std::bad_alloc)
            {
                coRef->manipulationProc->CompleteManipulation();
                success = FALSE;
            }
        }

        if(success)
        {
            // Make the current object the new head of the list
            m_lCoreObjects.remove(coRef);
            m_lCoreObjects.push_front(coRef);

            *bFound = TRUE;

            // Renders objects to bring new object to front
            RenderObjects();
        }
    }
    else
    {
        *bFound = FALSE;
    }
}

VOID CComTouchDriver::MoveEvent(const TOUCHINPUT* inData)
{
    DWORD dwCursorID  = inData->dwID;
    DWORD dwPTime = inData->dwTime;
    int x = GetLocalizedPointX(inData->x);
    int y = GetLocalizedPointY(inData->y);

    // Get the object associated with this cursor id
    std::map<DWORD, CCoreObject*>::iterator it = m_mCursorObject.find(dwCursorID);
    if(it != m_mCursorObject.end())
    {
        CCoreObject* coRef = (*it).second;

        // Feed values into the manipulation processor
        coRef->manipulationProc->ProcessMoveWithTime(dwCursorID, (FLOAT)x, (FLOAT)y, dwPTime);
    }
}

VOID CComTouchDriver::UpEvent(const TOUCHINPUT* inData)
{
    DWORD dwCursorID = inData->dwID;
    DWORD dwPTime = inData->dwTime;
    int x = GetLocalizedPointX(inData->x);
    int y = GetLocalizedPointY(inData->y);
    BOOL success = FALSE;

    // Get the CoreObject associated with this cursor id
    std::map<DWORD, CCoreObject*>::iterator it = m_mCursorObject.find(dwCursorID);
    if(it != m_mCursorObject.end())
    {
        CCoreObject* coRef = (*it).second;

        // Feed values into the manipulation processor
        success = SUCCEEDED(coRef->manipulationProc->ProcessUpWithTime(dwCursorID, (FLOAT)x, (FLOAT)y, dwPTime));
    }

    // Remove the cursor, object mapping
    if(success)
    {
        m_mCursorObject.erase(dwCursorID);
    }
}

// Handler for activating the inerita processor
VOID CComTouchDriver::ProcessChanges()
{
    // Run through the list of core objects and process any of its active inertia processors
    std::list<CCoreObject*>::iterator it;

    for(it = m_lCoreObjects.begin(); it != m_lCoreObjects.end(); ++it)
    {
        if((*it)->bIsInertiaActive == TRUE)
        {
            BOOL bCompleted = FALSE;
            (*it)->inertiaProc->Process(&bCompleted);
        }
    }
    // Render all the changes
    RenderObjects();
}

// The subsequent set of methods are for handling the rendering work

VOID CComTouchDriver::RenderObjects()
{
    m_d2dDriver->BeginDraw();
    m_d2dDriver->RenderBackground((FLOAT)m_iCWidth, (FLOAT)m_iCHeight);	

    std::list<CCoreObject*>::reverse_iterator it;

    for(it = m_lCoreObjects.rbegin(); it != m_lCoreObjects.rend(); ++it)
    {
        (*it)->doDrawing->Paint();
    }

    m_d2dDriver->EndDraw();
}

VOID CComTouchDriver::RenderInitialState(const int iCWidth, const int iCHeight)
{

    for(const auto& ObjectMapping : m_mCursorObject)
    {
      const auto& pObject = ObjectMapping.second;
      if(pObject->bIsInertiaActive)
        pObject->inertiaProc->Complete();
      else
        pObject->manipulationProc->CompleteManipulation();
    }
    m_mCursorObject.clear();

    m_iCWidth = iCWidth;
    m_iCHeight = iCHeight;

    int widthScaled = GetLocalizedPointX(iCWidth);
    int heightScaled = GetLocalizedPointY(iCHeight);

    const float squareDistance = 205;
    const auto numSquareColumns = int(sqrt(NUM_CORE_OBJECTS));
    auto iObject = m_lCoreObjects.rbegin();
    for(int i = 0; i < NUM_CORE_OBJECTS; i++) {
      const auto pos = POINTF{widthScaled - squareDistance * (i % numSquareColumns + 1), heightScaled - squareDistance * (i / numSquareColumns + 1)};
      ((CSquare*)(*iObject)->doDrawing)->ResetState(pos.x, pos.y, iCWidth, iCHeight, widthScaled, heightScaled, CSquare::DrawingColor(i % 4));
      ++iObject;
    }

    const auto sliderBorder = 5;
    const auto sliderDistance = POINTF{50 + sliderBorder, 200 + sliderBorder};
    const auto numSliderColumns = int(sqrt(NUM_SLIDERS * sliderDistance.y / sliderDistance.x));
    for(int i = 0; i < NUM_SLIDERS; i++) {
      const auto pos = POINTF{sliderBorder + sliderDistance.x * (i % numSliderColumns), sliderBorder + sliderDistance.y * (i / numSliderColumns)};
      ((CSlider*)(*iObject)->doDrawing)->ResetState(pos.x, pos.y, iCWidth, iCHeight, widthScaled, heightScaled, 50, 200);
      ++iObject;
    }

    const auto pos = POINTF{sliderBorder + sliderDistance.x*11, float(sliderBorder)};
    ((CSlider*)(*iObject)->doDrawing)->ResetState(pos.x, pos.y, iCWidth, iCHeight, widthScaled, heightScaled, 50, int(3*sliderDistance.y - sliderBorder));

    const auto knobBorder = 2;
    const auto knobDistance = POINTF{50 + knobBorder, 50 + knobBorder};
    const auto numKnobColumns = int(sqrt(NUM_KNOBS * knobDistance.y / knobDistance.x));
    for(int i = 0; i < NUM_KNOBS; i++) {
      ++iObject;
      const auto pos = POINTF{knobBorder + knobDistance.x * (i % numKnobColumns), heightScaled - (knobBorder + knobDistance.y * (1 + i / numKnobColumns))};
      ((CSlider*)(*iObject)->doDrawing)->ResetState(pos.x, pos.y, iCWidth, iCHeight, widthScaled, heightScaled, 50, 50);
    }


RenderObjects();
}

int CComTouchDriver::GetLocalizedPointX(int ptX)
{
    return (int)(ptX * m_dpiScaleX);
}

int CComTouchDriver::GetLocalizedPointY(int ptY)
{
    return (int)(ptY * m_dpiScaleY);
}
