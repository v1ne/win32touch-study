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

class CSlider : public CTransformableDrawingObject {
public:
    enum SliderType {TYPE_SLIDER, TYPE_KNOB};
    enum InteractionMode { MODE_ABSOLUTE, MODE_RELATIVE, NUM_MODES};

    CSlider(HWND hwnd, CD2DDriver* d2dDriver, SliderType type, InteractionMode mode);
    ~CSlider() override;

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
    void PaintSlider();
    void PaintKnob();

    void HandleTouch(float y, float cumulativeTranslationX, float deltaY);
    void HandleTouchInAbsoluteInteractionMode(float y);
    void HandleTouchInRelativeInteractionMode(float cumulativeTranslationX, float deltaY);

    float m_value = 0.0f;
    float m_rawTouchValue = 0.0f;

    ID2D1RectangleGeometryPtr m_spRectGeometry;
    
    float m_bottomPos;
    float m_sliderHeight;

    InteractionMode m_mode;
    SliderType m_type;
};
