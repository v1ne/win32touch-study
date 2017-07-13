// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "D2DDriver.h"
#include "ViewBase.h"
#include <windows.h>

class CSquare : public CTransformableDrawingObject {
public:
    enum DrawingColor {Blue, Orange, Green, Red};

    CSquare(HWND hwnd, CD2DDriver* d2dDriver, const DrawingColor drawingColor);
    ~CSquare() override;

    void ManipulationStarted(Point2F start) override;
    void ManipulationDelta(ViewBase::ManipDeltaParams) override;
    void ManipulationCompleted(ViewBase::ManipCompletedParams) override;

    void Paint() override;
    bool InRegion(Point2F pos) override;

private:
    ID2D1LinearGradientBrushPtr m_pGlBrush;
    ID2D1LinearGradientBrushPtr m_currBrush;
    ID2D1RoundedRectangleGeometryPtr m_spRoundedRectGeometry;
};
