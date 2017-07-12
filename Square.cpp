// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "Square.h"
#include <math.h>

#define INITIAL_OBJ_WIDTH	200
#define INITIAL_OBJ_HEIGHT	200
#define DEFAULT_DIRECTION	0

CSquare::CSquare(HWND hwnd, CD2DDriver* d2dDriver) :
    m_hWnd(hwnd),
    m_d2dDriver(d2dDriver)
{
    // Get the render target for drawing to
    m_spRT = m_d2dDriver->GetRenderTarget();
}

CSquare::~CSquare()
{
}


void CSquare::ManipulationStarted(FLOAT x, FLOAT y)
{
    RestoreRealPosition();
}


void CSquare::ManipulationDelta(FLOAT x, FLOAT y,
    FLOAT translationDeltaX, FLOAT translationDeltaY,
    FLOAT scaleDelta, FLOAT expansionDelta, FLOAT rotationDelta,
    FLOAT cumulativeTranslationX, FLOAT cumulativeTranslationY,
    FLOAT cumulativeScale, FLOAT cumulativeExpansion, FLOAT cumulativeRotation,
    bool isExtrapolated)
{
    FLOAT rads = 180.0f / 3.14159f;
    
    SetManipulationOrigin(x, y);

    Rotate(rotationDelta*rads);

    // Apply translation based on scaleDelta
    Scale(scaleDelta);

    // Apply translation based on translationDelta
    Translate(translationDeltaX, translationDeltaY, isExtrapolated);
}

void CSquare::ManipulationCompleted(
    FLOAT x,
    FLOAT y,
    FLOAT cumulativeTranslationX,
    FLOAT cumulativeTranslationY,
    FLOAT cumulativeScale,
    FLOAT cumulativeExpansion,
    FLOAT cumulativeRotation) {}


// Sets the default position, dimensions and color for the drawing object
void CSquare::ResetState(const float startX, const float startY, 
                                const int ixClient, const int iyClient,
                                float iScaledWidth, float iScaledHeight,
                                const DrawingColor colorChoice)
{
    // Set width and height of the client area
    // must adjust for dpi aware
    m_ClientAreaWidth = iScaledWidth;
    m_ClientAreaHeight = iScaledHeight;

    // Initialize width height of object
    m_fWidth   = INITIAL_OBJ_WIDTH;
    m_fHeight  = INITIAL_OBJ_HEIGHT;

    // Set outer elastic border
    UpdateBorders();

    // Set the top, left starting position

    // Set cooredinates given by processor
    m_fXI = startX;
    m_fYI = startY;

    // Set coordinates used for rendering
    m_fXR = startX;
    m_fYR = startY;

    // Set touch origin to 0
    m_fOX = 0.0f;
    m_fOY = 0.0f;

    // Initialize scaling factor
    m_fFactor = 1.0f;

    // Initialize angle
    m_fAngleCumulative = 0.0f;

    // Determines what brush to use for drawing this object and 
    // gets the brush from the D2DDriver class

    switch (colorChoice){
        case Blue:
            m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Blue);
            break;
        case Orange:
            m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Orange);
            break;
        case Green:
            m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Green);
            break;
        case Red:
            m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Red);
            break;
        default:
            m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Blue);
    }
}

void CSquare::Paint()
{
    if(!(m_spRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        float fGlOffset = 2.5f;
    
        // Setup our matrices for performing transforms

        D2D_MATRIX_3X2_F rotateMatrix;
        D2D_MATRIX_3X2_F identityMatrix;
        identityMatrix = D2D1::Matrix3x2F::Identity();
        
        // Apply rotate transform
        
        rotateMatrix = D2D1::Matrix3x2F::Rotation(
            m_fAngleCumulative, 
            D2D1::Point2F(
                m_fXR + m_fWidth/2.0f, 
                m_fYR + m_fHeight/2.0f 
            )
        );
        
        m_spRT->SetTransform(&rotateMatrix);

        // Store the rotate matrix to be used in hit testing
        m_lastMatrix = rotateMatrix;

        // Get glossy brush
        m_pGlBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Glossy);
        
        // Set positions of gradients based on the new coordinates of the objecs
        
        m_currBrush->SetStartPoint(
            D2D1::Point2F(
                m_fXR, 
                m_fYR
            )
        );
        
        m_currBrush->SetEndPoint(
            D2D1::Point2F(
                m_fXR, 
                m_fYR + m_fHeight
            )
        );
        
        m_pGlBrush->SetStartPoint(
            D2D1::Point2F(
                m_fXR, 
                m_fYR
            )
        );
        
        m_pGlBrush->SetEndPoint(
            D2D1::Point2F(
                m_fXR + m_fWidth/15.0f, 
                m_fYR + m_fHeight/2.0f
            )
        );

        // Create rectangle to draw

        D2D1_RECT_F rectangle = D2D1::RectF(
            m_fXR,
            m_fYR,
            m_fXR+m_fWidth,
            m_fYR+m_fHeight
        );
    
        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
            rectangle,
            10.0f, 10.0f
        );

        // Create glossy effect

        D2D1_RECT_F glossyRect = D2D1::RectF(
            m_fXR+fGlOffset,
            m_fYR+fGlOffset,
            m_fXR+m_fWidth-fGlOffset,
            m_fYR+m_fHeight/2.0f
        );
        
        D2D1_ROUNDED_RECT glossyRoundedRect = D2D1::RoundedRect(
            glossyRect, 
            10.0f, 
            10.0f
        );

        // D2D requires that a geometry is created for the rectangle
        m_d2dDriver->CreateGeometryRoundedRect(
            roundedRect, 
            &m_spRoundedRectGeometry
        );

        // Fill the geometry that was created
        m_spRT->FillGeometry(
            m_spRoundedRectGeometry, 
            m_currBrush
        );

        // Draw glossy effect
        m_spRT->FillRoundedRectangle(
            &glossyRoundedRect,
            m_pGlBrush
        );

        // Restore our transform to nothing
        m_spRT->SetTransform(&identityMatrix);
    }
}

void CSquare::Translate(float fdx, float fdy, bool bInertia)
{
    m_fdX = fdx;
    m_fdY = fdy;

    float fOffset[2];
    fOffset[0] = m_fOX - m_fdX;
    fOffset[1] = m_fOY - m_fdY;

    // Translate based on the offset caused by rotating 
    // and scaling in order to vary rotational behavior depending 
    // on where the manipulation started
    
    if(m_fAngleApplied != 0.0f)
    {
        float v1[2];
        v1[0] = GetCenterX() - fOffset[0];
        v1[1] = GetCenterY() - fOffset[1];

        float v2[2];
        RotateVector(v1, v2, m_fAngleApplied);

        m_fdX += v2[0] - v1[0];
        m_fdY += v2[1] - v1[1];
    }

    if(m_fFactor != 1.0f)
    {
        float v1[2];
        v1[0] = GetCenterX() - fOffset[0];
        v1[1] = GetCenterY() - fOffset[1];

        float v2[2];
        v2[0] = v1[0] * m_fFactor;
        v2[1] = v1[1] * m_fFactor;
        
        m_fdX += v2[0] - v1[0];
        m_fdY += v2[1] - v1[1];
    }

    m_fXI += m_fdX;
    m_fYI += m_fdY;

    // The following code handles the effect for 
    // bouncing off the edge of the screen.  It takes
    // the x,y coordinates computed by the inertia processor
    // and calculates the appropriate render coordinates
    // in order to achieve the effect.

    if (bInertia)
    {
        ComputeElasticPoint(m_fXI, &m_fXR, m_fBorderX);
        ComputeElasticPoint(m_fYI, &m_fYR, m_fBorderY);
    }
    else
    {
        m_fXR = m_fXI;
        m_fYR = m_fYI;

        // Make sure it stays on screen
        EnsureVisible();
    }
}

void CSquare::EnsureVisible()
{
    m_fXR = max(0,min(m_fXI, m_ClientAreaWidth-m_fWidth));
    m_fYR = max(0,min(m_fYI, m_ClientAreaHeight-m_fHeight));
    RestoreRealPosition();
}

void CSquare::Scale(const float dFactor)
{
    m_fFactor = dFactor;

    float scaledW = (dFactor-1) * m_fWidth;
    float scaledH = (dFactor-1) * m_fHeight;
    float scaledX = scaledW/2.0f;
    float scaledY = scaledH/2.0f;
    
    m_fXI -= scaledX;
    m_fYI -= scaledY;

    m_fWidth  += scaledW;
    m_fHeight += scaledH;
    
    // Only limit scaling in the case that the factor is not 1.0

    if(dFactor != 1.0f)
    {
        m_fXI = max(0, m_fXI);
        m_fYI = max(0, m_fYI);

        m_fWidth = min(min(m_ClientAreaWidth, m_ClientAreaHeight), m_fWidth);
        m_fHeight = min(min(m_ClientAreaWidth, m_ClientAreaHeight), m_fHeight);
    }

    // Readjust borders for the objects new size
    UpdateBorders();
}

void CSquare::Rotate(const float fAngle)
{
    m_fAngleCumulative += fAngle;
    m_fAngleApplied = fAngle;
}

void CSquare::SetManipulationOrigin(float x, float y)
{
    m_fOX = x;
    m_fOY = y;
}

// Helper method that rotates a vector using basic math transforms
void CSquare::RotateVector(float *vector, float *tVector, float fAngle)
{
    auto fAngleRads = fAngle / 180.0f * 3.14159f;
    auto fSin = float(sin(fAngleRads));
    auto fCos = float(cos(fAngleRads));

    auto fNewX = (vector[0]*fCos) - (vector[1]*fSin);
    auto fNewY = (vector[0]*fSin) + (vector[1]*fCos);

    tVector[0] = fNewX;
    tVector[1] = fNewY;
}


// Hit testing method handled with Direct2D
bool CSquare::InRegion(float x, float y)
{
    BOOL b = FALSE;
    m_spRoundedRectGeometry->FillContainsPoint(D2D1::Point2F(x, y), &m_lastMatrix, &b);
    return b;
}

// Sets the internal coordinates to render coordinates
void CSquare::RestoreRealPosition()
{
    m_fXI = m_fXR;
    m_fYI = m_fYR;
}

void CSquare::UpdateBorders()
{
    m_fBorderX = m_ClientAreaWidth  - m_fWidth;
    m_fBorderY = m_ClientAreaHeight - m_fHeight;
}

// Computes the the elastic point and sets the render coordinates
void CSquare::ComputeElasticPoint(float fIPt, float *fRPt, float fBSize)
{
    // If the border size is 0 then do not attempt
    // to calculate the render point for elasticity
    if(fBSize == 0)
        return;

    // Calculate render coordinate for elastic border effect

    // Divide the cumulative translation vector by the max border size
    auto q = int(fabsf(fIPt) / fBSize);
    int direction = q % 2;
    
    // Calculate the remainder this is the new render coordinate
    float newPt = fabsf(fIPt) - fBSize*q;
    
    if (direction == DEFAULT_DIRECTION)
    {
        *fRPt = newPt;
    }
    else
    {
        *fRPt = fBSize - newPt;
    }
}

float CSquare::GetPosY() { return m_fYI; }
float CSquare::GetPosX() { return m_fXI; }
float CSquare::GetWidth() { return m_fWidth; }
float CSquare::GetHeight() { return m_fHeight; }
float CSquare::GetCenterX() { return m_fXI + m_fWidth/2.0f; }
float CSquare::GetCenterY() { return m_fYI + m_fHeight/2.0f; }
