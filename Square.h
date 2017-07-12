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

class CSquare : public CTransformableDrawingObject {
public:
    enum DrawingColor {Blue, Orange, Green, Red};

    CSquare(HWND hwnd, CD2DDriver* d2dDriver, const DrawingColor drawingColor);
    ~CSquare() override;

    void ManipulationStarted(Point2F Po) override;
    void ManipulationDelta(Point2F pos, Point2F dTranslation,
        float dScale, float dExtension, float dRotation,
        Point2F sumTranslation, float sumScale, float sumExpansion, float sumRotation,
        bool isExtrapolated) override;
    void ManipulationCompleted(Point2F pos, Point2F sumTranslation,
        float sumScale, float sumExpansion, float sumRotation) override;

    void Paint() override;
    bool InRegion(Point2F pos) override;

private:
    // D2D brushes
    ID2D1LinearGradientBrushPtr m_pGlBrush;
    ID2D1LinearGradientBrushPtr m_currBrush;

    ID2D1RoundedRectangleGeometryPtr m_spRoundedRectGeometry;
};
