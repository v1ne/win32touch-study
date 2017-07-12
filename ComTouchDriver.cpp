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
                success = object->Initialize(new (std::nothrow) CSquare(
                    m_hWnd, m_d2dDriver, CSquare::DrawingColor(i % 4)));
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

    m_lCoreObjectsInInitialOrder.reserve(m_lCoreObjects.size());
    m_lCoreObjectsInInitialOrder.insert(m_lCoreObjectsInInitialOrder.begin(), m_lCoreObjects.begin(), m_lCoreObjects.end());

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
    auto p = LocalizePoint({inData->x, inData->y});
    BOOL success = TRUE;

    // Check that the user has touched within an objects region and feed to the objects manipulation processor

    if(coRef->doDrawing->InRegion(p))
    {
        // Feed values to the Manipulation Processor
        success = SUCCEEDED(coRef->manipulationProc->ProcessDownWithTime(dwCursorID, p.x, p.y, dwPTime));

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
    auto p = LocalizePoint({inData->x, inData->y});

    // Get the object associated with this cursor id
    std::map<DWORD, CCoreObject*>::iterator it = m_mCursorObject.find(dwCursorID);
    if(it != m_mCursorObject.end())
    {
        CCoreObject* coRef = (*it).second;

        // Feed values into the manipulation processor
        coRef->manipulationProc->ProcessMoveWithTime(dwCursorID, p.x, p.y, dwPTime);
    }
}

VOID CComTouchDriver::UpEvent(const TOUCHINPUT* inData)
{
    DWORD dwCursorID = inData->dwID;
    DWORD dwPTime = inData->dwTime;
    auto p = LocalizePoint({inData->x, inData->y});
    BOOL success = FALSE;

    // Get the CoreObject associated with this cursor id
    std::map<DWORD, CCoreObject*>::iterator it = m_mCursorObject.find(dwCursorID);
    if(it != m_mCursorObject.end())
    {
        CCoreObject* coRef = (*it).second;

        // Feed values into the manipulation processor
        success = SUCCEEDED(coRef->manipulationProc->ProcessUpWithTime(dwCursorID, p.x, p.y, dwPTime));
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
    D2D1_SIZE_U  size = {UINT32(iCWidth), UINT32(iCHeight)};
    if (FAILED(m_d2dDriver->GetRenderTarget()->Resize(size)))
    {
      m_d2dDriver->DiscardDeviceResources();
      InvalidateRect(m_hWnd, NULL, FALSE);
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
      const auto pos = Point2F{
        clientArea.x - squareSize.x * (i % numSquareColumns + 1),
        clientArea.y - squareSize.y * (i / numSquareColumns + 1)};
      ((CSquare*)(*iObject)->doDrawing)->ResetState(pos, clientArea, squareSize);
      ++iObject;
    }

    const auto sliderBorder = Point2F{5};
    const auto sliderSize = Point2F{50, 200};
    const auto sliderDistance = sliderSize + sliderBorder;
    const auto numSliderColumns = int(sqrt(NUM_SLIDERS * sliderDistance.y / sliderDistance.x));
    for(int i = 0; i < NUM_SLIDERS; i++) {
      const auto pos = Point2F{
        sliderBorder.x + sliderDistance.x * (i % numSliderColumns),
        sliderBorder.y + sliderDistance.y * (i / numSliderColumns)};
      ((CSlider*)(*iObject)->doDrawing)->ResetState(pos, clientArea, sliderSize);
      ++iObject;
    }

    const auto pos = Point2F{sliderBorder.x + sliderDistance.x*11, sliderBorder.y};
    const auto bigSliderSize = Point2F{50, 3*sliderDistance.y - sliderBorder.y};
    ((CSlider*)(*iObject)->doDrawing)->ResetState(pos, clientArea, bigSliderSize);

    const auto knobBorder = Point2F{2.f};
    const auto knobSize = Point2F{50.f};
    const auto knobDistance = knobSize + knobBorder;
    const auto numKnobColumns = int(sqrt(NUM_KNOBS * knobDistance.y / knobDistance.x));
    for(int i = 0; i < NUM_KNOBS; i++) {
      ++iObject;
      const auto pos = Point2F{
        knobBorder.x + knobDistance.x * (i % numKnobColumns),
        clientArea.y - (knobBorder.y + knobDistance.y * (1 + i / numKnobColumns))};
      ((CSlider*)(*iObject)->doDrawing)->ResetState(pos, clientArea, knobSize);
    }


RenderObjects();
}
