// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "D2DDriver.h"
#include "DrawingObject.h"
#include <windows.h>

class CSquare : public CDrawingObject {
public:
    enum DrawingColor {Blue, Orange, Green, Red};

    CSquare(HWND hwnd, CD2DDriver* d2dDriver);
    ~CSquare() override;

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
        float scaledWidth, float scaledHeight,
        const DrawingColor colorChoice);

    void Paint() override;
    bool InRegion(float x, float y) override;

    // Public get methods
    float GetPosY() override;
    float GetPosX() override;
    float GetWidth() override;
    float GetHeight() override;
    float GetCenterX() override;
    float GetCenterY() override;


private:
    void RestoreRealPosition();
    void SetManipulationOrigin(float x, float y);
    void Translate(float fdx, float fdy, bool bInertia);
    void Scale(const float fFactor);
    void Rotate(const float fAngle);

    void RotateVector(float* vector, float* tVector, float fAngle);
    void ComputeElasticPoint(float fIPt, float* fRPt, float fBorderSize);
    void UpdateBorders();
    void EnsureVisible();

    HWND m_hWnd;

    CD2DDriver* m_d2dDriver;

    // D2D brushes
    ID2D1HwndRenderTargetPtr	m_spRT;
    ID2D1LinearGradientBrushPtr m_pGlBrush;
    ID2D1LinearGradientBrushPtr m_currBrush;

    ID2D1RoundedRectangleGeometryPtr m_spRoundedRectGeometry;

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
    float m_fBorderX;
    float m_fBorderY;

    // Client width and height
    float m_ClientAreaWidth;
    float m_ClientAreaHeight;
};
