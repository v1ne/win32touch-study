// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) v1ne

#pragma once

#include "D2DDriver.h"
#include "DrawingObject.h"
#include <windows.h>

class CSlider : public CDrawingObject {
public:
    enum Mode { MODE_ABSOLUTE, MODE_RELATIVE, NUM_MODES};

    CSlider(HWND hwnd, CD2DDriver* d2dDriver, Mode mode);
    ~CSlider() override;

    void ManipulationStarted(FLOAT x, FLOAT y) override;

    // Handles event when the manipulation is progress
    void ManipulationDelta(
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
        FLOAT cumulativeRotation,
        bool isExtrapolated) override;

    // Handles event when the manipulation ends
    void ManipulationCompleted(
        FLOAT x,
        FLOAT y,
        FLOAT cumulativeTranslationX,
        FLOAT cumulativeTranslationY,
        FLOAT cumulativeScale,
        FLOAT cumulativeExpansion,
        FLOAT cumulativeRotation) override;

    void ResetState(const float startX, const float startY, 
        const int ixClient, const int iyClient, 
        const int iScaledWidth, const int iScaledHeight,
        const int iInitialWidth, const int iInitialHeight);

    void Paint() override;
    void Translate(float fdx, float fdy, bool bInertia) override;
    void Scale(const float fFactor) override;
    void Rotate(const float fAngle) override;
    bool InRegion(LONG lX, LONG lY) override;
    void RestoreRealPosition() override;

    // Public set method
    void SetManipulationOrigin(float x, float y) override;

    // Public get methods
    float GetPosY() override;
    float GetPosX() override;
    float GetWidth() override;
    float GetHeight() override;
    float GetCenterX() override;
    float GetCenterY() override;


private:
    void HandleTouch(float y, float cumulativeTranslationX, float deltaY);
    void HandleTouchInAbsoluteMode(float y);
    void HandleTouchInRelativeMode(float cumulativeTranslationX, float deltaY);

    void RotateVector(float* vector, float* tVector, float fAngle);
    void ComputeElasticPoint(float fIPt, float* fRPt, int iDimension);
    void UpdateBorders(); 
    void EnsureVisible();

    float m_value = 0.0f;

    HWND m_hWnd;

    CD2DDriver* m_d2dDriver;

    // D2D brushes
    ID2D1HwndRenderTargetPtr	m_spRT;
    
    ID2D1RectangleGeometryPtr m_spRectGeometry;
    
    // Keeps the last matrix used to perform the rotate transform
    D2D_MATRIX_3X2_F m_lastMatrix;

    // Coordinates of where manipulation started
    float m_fOX;
    float m_fOY;

    // Internal top, left coordinates of object (Real inertia values)
    float m_fXI;
    float m_fYI;
    
    // Rendered top, left coordinates of object
    float m_fXR;
    float m_fYR;
    
    // Width and height of the object
    float m_fWidth;
    float m_fHeight;

    // Scaling factor applied to the object
    float m_fFactor;

    // Cumulative angular rotation applied to the object
    float m_fAngleCumulative;

    // Current angular rotation applied to object
    float m_fAngleApplied;

    // Delta x, y values
    float m_fdX;
    float m_fdY;

    // Right and bottom borders relative to the object's size
    int m_iBorderX;
    int m_iBorderY;

    // Client width and height
    int m_iCWidth;
    int m_iCHeight;

    Mode m_mode;
};
