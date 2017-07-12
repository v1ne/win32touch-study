// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) v1ne

#include "Geometry.h"
#include "Slider.h"
#include <math.h>


class DialOnALeash: public CDrawingObject {
public:
  ~DialOnALeash() override {};

  void ManipulationStarted(Point2F) override {}

  // Handles event when the manipulation is progress
  void ManipulationDelta(Point2F pos, Point2F dTranslation,
      float dScale, float dExtension, float dRotation,
      Point2F sumTranslation, float sumScale, float sumExpansion, float sumRotation,
      bool isExtrapolated) override {}

  // Handles event when the manipulation ends
  void ManipulationCompleted(
      float x,
      float y,
      float cumulativeTranslationX,
      float cumulativeTranslationY,
      float cumulativeScale,
      float cumulativeExpansion,
      float cumulativeRotation) {}
    
  void Paint() override {}
  bool InRegion(Point2F pos) override {return false; }
};

CSlider::CSlider(HWND hWnd, CD2DDriver* d2dDriver, SliderType type, InteractionMode mode)
  : CTransformableDrawingObject(hWnd, d2dDriver)
  , m_mode(mode)
  , m_type(type)
  , m_value(::rand() / float(RAND_MAX)) {
}

CSlider::~CSlider()
{
}


void CSlider::ManipulationStarted(Point2F pos) {
  RestoreRealPosition();

  m_rawTouchValue = m_value;

  if(!gShiftPressed)
    HandleTouch(pos.y, 0.f, 0.f);
}


void CSlider::ManipulationDelta(Point2F pos, Point2F dTranslation,
    float dScale, float dExtension, float dRotation,
    Point2F sumTranslation, float sumScale, float sumExpansion, float sumRotation,
    bool isExtrapolated) {
  if(gShiftPressed) {
    float rads = 180.0f / 3.14159f;

    SetManipulationOrigin(pos);

    Rotate(dRotation * rads);

    // Apply translation based on scaleDelta
    Scale(dScale);

    // Apply translation based on translationDelta
    Translate(dTranslation, isExtrapolated);
  }
  else
    HandleTouch(pos.y, sumTranslation.x, dTranslation.y);
}


void CSlider::ManipulationCompleted(Point2F pos, Point2F sumTranslation,
      float sumScale, float sumExpansion, float sumRotation) {
  if(!gShiftPressed)
    HandleTouch(pos.y, sumTranslation.x, 0.f);
}


void CSlider::HandleTouch(float y, float cumultiveTranslationX, float deltaY)
{
  switch(m_mode) {
  case MODE_ABSOLUTE:
    HandleTouchInAbsoluteInteractionMode(y);
    break;
  case MODE_RELATIVE:
    HandleTouchInRelativeInteractionMode(cumultiveTranslationX, deltaY);
    break;
  }
}


void CSlider::HandleTouchInAbsoluteInteractionMode(float y)
{
  m_value = ::fmaxf(0, ::fminf(1, (m_bottomPos - y) / m_sliderHeight));
}


void CSlider::HandleTouchInRelativeInteractionMode(float cumulativeTranslationX, float deltaY)
{
  const auto dragScalingFactor = 1 + ::fabsf(cumulativeTranslationX) / (2 * mSize.x);

  m_rawTouchValue -= deltaY / m_sliderHeight / dragScalingFactor;
  m_value = ::fmaxf(0, ::fminf(1, m_rawTouchValue));
}


void CSlider::Paint()
{
  if(!(m_spRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
  {
    const auto rotateMatrix = D2D1::Matrix3x2F::Rotation(
      m_fAngleCumulative,
      (mRenderPos + mSize / 2.f).to<D2D1_POINT_2F>());

    m_spRT->SetTransform(&rotateMatrix);

    // Store the rotate matrix to be used in hit testing
    m_lastMatrix = rotateMatrix;

    const auto bgRect = D2D1::RectF(mRenderPos.x, mRenderPos.y, mRenderPos.x+mSize.x, mRenderPos.y+mSize.y);
    m_d2dDriver->CreateGeometryRect(bgRect, &m_spRectGeometry);
    m_spRT->FillGeometry(m_spRectGeometry, m_d2dDriver->m_spLightGreyBrush);

    switch(m_type) {
    case TYPE_SLIDER:
      PaintSlider();
      break;
    case TYPE_KNOB:
      PaintKnob();
      break;
    }

    // Restore our transform to nothing
    const auto identityMatrix = D2D1::Matrix3x2F::Identity();
    m_spRT->SetTransform(&identityMatrix);
  }
}


void CSlider::PaintSlider()
{
  const auto borderWidth = mSize.x / 4;
  const auto topBorder = mSize.y * 10 / 100;
  const auto topEnd = mRenderPos.y + topBorder;
  const auto bottomPos = mRenderPos.y+mSize.y;
  const auto sliderHeight = bottomPos - topEnd;
  const auto topPos = bottomPos - m_value * sliderHeight;

  m_bottomPos = bottomPos;
  m_sliderHeight = sliderHeight;

  const auto fgRect = D2D1::RectF(mRenderPos.x + borderWidth, topPos, mRenderPos.x+mSize.x - borderWidth, bottomPos);
  ID2D1RectangleGeometryPtr fgGeometry;
  m_d2dDriver->CreateGeometryRect(fgRect, &fgGeometry);
  m_spRT->FillGeometry(fgGeometry,
    m_mode == MODE_RELATIVE ? m_d2dDriver->m_spCornflowerBrush : m_d2dDriver->m_spSomePinkishBlueBrush);

  wchar_t buf[16];
  wsprintf(buf, L"%d%%", int(m_value*100));
  m_d2dDriver->RenderText({mRenderPos.x, mRenderPos.y, mRenderPos.x + mSize.x, mRenderPos.y + topBorder}, buf, wcslen(buf));
}


void CSlider::PaintKnob()
{
  const auto border = POINTF{mSize.x / 8, mSize.y / 8};
  const auto center = Center().to<D2D1_POINT_2F>();
  const auto knobRadius = ::fminf((mSize.x - border.x)/2, (mSize.y - border.y)/2);

  m_sliderHeight = mSize.y * 3;
  m_bottomPos = mRenderPos.y + mSize.y / 2;

  D2D1_ELLIPSE knobOutlineParams = {center, knobRadius, knobRadius};
  ID2D1EllipseGeometryPtr knobOutline;
  m_d2dDriver->CreateEllipseGeometry(knobOutlineParams, &knobOutline);
  m_spRT->FillGeometry(knobOutline, m_d2dDriver->m_spDarkGreyBrush);

  const auto dotRadius = 3;
  const auto knobRelPos = Point2F{0, knobRadius - dotRadius};
  const auto knobRelPosRotated = rotateDeg(knobRelPos, 45 + m_value * 270);
  D2D1_ELLIPSE knobDotParams = {{center.x + knobRelPosRotated.x, center.y + knobRelPosRotated.y}, dotRadius, dotRadius};
  ID2D1EllipseGeometryPtr knobDot;
  m_d2dDriver->CreateEllipseGeometry(knobDotParams, &knobDot);
  m_spRT->FillGeometry(knobDot,
    m_mode == MODE_RELATIVE ? m_d2dDriver->m_spCornflowerBrush : m_d2dDriver->m_spSomePinkishBlueBrush);

  wchar_t buf[16];
  wsprintf(buf, L"%d%%", int(m_value*100));
  m_d2dDriver->RenderText({center.x - mSize.x/3, center.y - border.y, center.x + mSize.x/3, center.y + border.y}, buf, wcslen(buf));
}


bool CSlider::InRegion(Point2F pos)
{
  BOOL b = FALSE;
  m_spRectGeometry->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
  return b;
}
