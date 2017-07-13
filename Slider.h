// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) v1ne

#pragma once

#include "D2DDriver.h"
#include "ViewBase.h"
#include <windows.h>

class DialOnALeash;

class CSlider : public CTransformableDrawingObject {
public:
  enum SliderType {TYPE_SLIDER, TYPE_KNOB};
  enum InteractionMode { MODE_ABSOLUTE, MODE_RELATIVE, MODE_DIAL, NUM_MODES};

  CSlider(HWND hwnd, CD2DDriver* d2dDriver, SliderType type, InteractionMode mode);
  ~CSlider() override;

  void ManipulationStarted(Point2F Po) override;
  void ManipulationDelta(ViewBase::ManipDeltaParams) override;
  void ManipulationCompleted(ViewBase::ManipCompletedParams) override;

  void Paint() override;
  bool InRegion(Point2F pos) override;

private:
  ID2D1SolidColorBrush* BrushForMode();
  void PaintSlider();
  void PaintKnob();
  bool InMyRegion(Point2F pos);

  bool HandleTouchEvent(TouchEventType type, Point2F pos, const TOUCHINPUT* pData) override;
  void HandleTouch(float y, float cumulativeTranslationX, float deltaY);
  void HandleTouchInAbsoluteInteractionMode(float y);
  void HandleTouchInRelativeInteractionMode(float cumulativeTranslationX, float deltaY);

  void MakeDial(Point2F center);
  void HideDial();

  ID2D1RectangleGeometryPtr mpOutlineGeometry;
  float mBottomPos;
  float mSliderHeight;
  
  float mValue = 0.0f;
  float mRawTouchValue = 0.0f;

  InteractionMode mMode;
  SliderType mType;
  DialOnALeash* mpDial = nullptr;
  friend class DialOnALeash;
};
