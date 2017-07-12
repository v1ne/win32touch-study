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


class DialOnALeash: public CTransformableDrawingObject {
public:
  DialOnALeash::DialOnALeash(HWND hWnd, CD2DDriver* d2dDriver)
    : CTransformableDrawingObject(hWnd, d2dDriver) {}
  ~DialOnALeash() override {};

  void ManipulationStarted(Point2F) override {}

  void ManipulationDelta(Point2F pos, Point2F dTranslation,
    float dScale, float dExtension, float dRotation,
    Point2F sumTranslation, float sumScale, float sumExpansion, float sumRotation,
      bool isExtrapolated) override {}

  void ManipulationCompleted(Point2F pos, Point2F sumTranslation,
    float sumScale, float sumExpansion, float sumRotation) {}

  void Paint() override {
    const auto radius = 300.f;
    D2D1_ELLIPSE background = {Center().to<D2D1_POINT_2F>(), radius, radius};
    m_d2dDriver->m_spD2DFactory->CreateEllipseGeometry(background, &mBackground);
    m_spRT->FillGeometry(mBackground, m_d2dDriver->m_spSomePinkishBlueBrush);

    ID2D1PathGeometryPtr pathGeometry;
    auto hr = m_d2dDriver->m_spD2DFactory->CreatePathGeometry(&pathGeometry);
    if (!SUCCEEDED(hr)) {
      return;
    }

    ID2D1GeometrySink *pSink = NULL;
    hr = pathGeometry->Open(&pSink);
    if (SUCCEEDED(hr)) {
      pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

      pSink->BeginFigure(
        D2D1::Point2F(100, 300), // Start point of the top half circle
        D2D1_FIGURE_BEGIN_FILLED
        );

      // Add the top half circle
      pSink->AddArc(
        D2D1::ArcSegment(
        D2D1::Point2F(400, 300), // end point of the top half circle, also the start point of the bottom half circle
        D2D1::SizeF(150, 150), // radius
        0.0f, // rotation angle
        D2D1_SWEEP_DIRECTION_CLOCKWISE,
        D2D1_ARC_SIZE_SMALL
        ));

      // Add the bottom half circle
      pSink->AddArc(
        D2D1::ArcSegment(
        D2D1::Point2F(100, 300), // end point of the bottom half circle
        D2D1::SizeF(150, 150),   // radius of the bottom half circle, same as previous one.
        0.0f, // rotation angle
        D2D1_SWEEP_DIRECTION_CLOCKWISE,
        D2D1_ARC_SIZE_SMALL
        ));

      pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
    }
    hr = pSink->Close();
    m_spRT->FillGeometry(pathGeometry, m_d2dDriver->m_spSomePinkishBlueBrush);

    for(int i = 0; i < 360; i += 30) {
      m_d2dDriver->RenderTiltedRect({300, 300}, 50, float(i), {10.f, 1.f}, m_d2dDriver->m_spDarkGreyBrush);
    }
  }

  bool InRegion(Point2F pos) override {
    BOOL b = FALSE;
    mBackground->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
    return b;
  }

  ID2D1EllipseGeometryPtr mBackground;
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
    m_d2dDriver->m_spD2DFactory->CreateRectangleGeometry(bgRect, &m_spRectGeometry);
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
  m_d2dDriver->m_spD2DFactory->CreateRectangleGeometry(fgRect, &fgGeometry);
  m_spRT->FillGeometry(fgGeometry,
    m_mode == MODE_RELATIVE ? m_d2dDriver->m_spCornflowerBrush : m_d2dDriver->m_spSomePinkishBlueBrush);

  wchar_t buf[16];
  wsprintf(buf, L"%d%%", int(m_value*100));
  m_d2dDriver->RenderText({mRenderPos.x, mRenderPos.y, mRenderPos.x + mSize.x, mRenderPos.y + topBorder}, buf, wcslen(buf));

  DialOnALeash(m_hWnd, m_d2dDriver).Paint();
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
  m_d2dDriver->m_spD2DFactory->CreateEllipseGeometry(knobOutlineParams, &knobOutline);
  m_spRT->FillGeometry(knobOutline, m_d2dDriver->m_spDarkGreyBrush);

  const auto knobMarkAngle = -135.f - m_value * 270;
  const auto markSize = Point2F{10.f, 5.f};
  m_d2dDriver->RenderTiltedRect(Center(), knobRadius - markSize.x, knobMarkAngle, markSize,
    m_mode == MODE_RELATIVE ? m_d2dDriver->m_spCornflowerBrush : m_d2dDriver->m_spSomePinkishBlueBrush);
  for(int i = 0; i <= 270; i += 30) {
    m_d2dDriver->RenderTiltedRect(Center(), knobRadius, float(-135 - i), {3.f, 1.f}, m_d2dDriver->m_spBlackBrush);
  }

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
