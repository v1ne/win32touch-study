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
  DialOnALeash::DialOnALeash(HWND hWnd, CD2DDriver* d2dDriver, CSlider* pParent)
      : CTransformableDrawingObject(hWnd, d2dDriver)
      , mpSlider(pParent) {
  }
  ~DialOnALeash() override {};

  void ManipulationStarted(Point2F) override {
  }

  void ManipulationDelta(CDrawingObject::ManipDeltaParams params) {
    float rads = 180.0f / 3.14159f;

    SetManipulationOrigin(params.pos);
    Rotate(params.dRotation * rads);
    Scale(params.dScale);
    Translate(params.dTranslation, params.isExtrapolated); 
  }

  void ManipulationCompleted(CDrawingObject::ManipCompletedParams params) {
    mIsShown = false;
  }

  void Paint() override {
    if (!mIsShown)
      return;

    const auto pos = mRenderPos;
    const auto innerRadius = mSize.x / 2.f;
    const auto outerRadius = innerRadius * 1.5f;
    D2D1_ELLIPSE background = {pos.to<D2D1_POINT_2F>(), outerRadius, outerRadius};
    m_d2dDriver->m_spD2DFactory->CreateEllipseGeometry(background, &mBackground);
    m_spRT->FillGeometry(mBackground, m_d2dDriver->m_spTransparentWhiteBrush);

    m_spRT->FillEllipse({pos.to<D2D1_POINT_2F>(), innerRadius, innerRadius}, m_d2dDriver->m_spWhiteBrush);
    m_spRT->FillEllipse({pos.to<D2D1_POINT_2F>(), 30.f, 30.f}, m_d2dDriver->m_spDarkGreyBrush);

    const auto markSize = Point2F{10.f, 1.f};
    for(float i = 0; i < 360.f; i += 3.6f) {
      m_d2dDriver->RenderTiltedRect(pos, innerRadius - markSize.x, i, markSize, m_d2dDriver->m_spDarkGreyBrush);
    }

    for(auto triangleAngle = 0.f; triangleAngle < 360; triangleAngle += 20) {
      const auto triangleStrokeSize = Point2F{16.f, 4.f};
      const auto vecToTriangle = rotateDeg(Vec2Right(innerRadius), triangleAngle);
      m_d2dDriver->RenderTiltedRect(pos + vecToTriangle, 0, triangleAngle + 45, triangleStrokeSize, m_d2dDriver->m_spBlackBrush);
      m_d2dDriver->RenderTiltedRect(pos + vecToTriangle, 0, triangleAngle - 45, triangleStrokeSize, m_d2dDriver->m_spBlackBrush);
    }
  }

  bool InRegion(Point2F pos) override {
    if (!mIsShown || !mBackground) return false;

    BOOL b = FALSE;
    mBackground->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
    return b;
  }

  bool mIsShown = false;
  ID2D1EllipseGeometryPtr mBackground;
  CSlider* mpSlider;
};

CSlider::CSlider(HWND hWnd, CD2DDriver* d2dDriver, SliderType type, InteractionMode mode)
  : CTransformableDrawingObject(hWnd, d2dDriver)
  , m_mode(mode)
  , m_type(type)
  , m_value(::rand() / float(RAND_MAX)) {
}

CSlider::~CSlider()
{
  HideDial();
}


void CSlider::ManipulationStarted(Point2F pos) {
  RestoreRealPosition();

  m_rawTouchValue = m_value;

  if(!gShiftPressed)
    HandleTouch(pos.y, 0.f, 0.f);

  if(m_mode == MODE_DIAL) {
    MakeDial();
    mpDial->ManipulationStarted(pos);
  }
}


void CSlider::ManipulationDelta(CDrawingObject::ManipDeltaParams params) {
  if(gShiftPressed) {
    float rads = 180.0f / 3.14159f;

    SetManipulationOrigin(params.pos);
    Rotate(params.dRotation * rads);
    Scale(params.dScale);
    Translate(params.dTranslation, params.isExtrapolated);
  } else {
    if(mpDial) {
      mpDial->ManipulationDelta(params);

      if (!InRegion(params.pos)) mpDial->mIsShown = true;
    }

    if (!mpDial || !mpDial->InRegion(params.pos))
      HandleTouch(params.pos.y, params.sumTranslation.x, params.dTranslation.y);
  }
}


void CSlider::ManipulationCompleted(CDrawingObject::ManipCompletedParams params) {
  if(!gShiftPressed)
    HandleTouch(params.pos.y, params.sumTranslation.x, 0.f);

  if(mpDial) mpDial->ManipulationCompleted(params);
  HideDial();
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

    if(mpDial) mpDial->Paint();

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
  m_spRT->FillGeometry(fgGeometry, BrushForMode());

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
  m_d2dDriver->m_spD2DFactory->CreateEllipseGeometry(knobOutlineParams, &knobOutline);
  m_spRT->FillGeometry(knobOutline, m_d2dDriver->m_spDarkGreyBrush);

  const auto knobMarkAngle = -135.f - m_value * 270;
  const auto markSize = Point2F{10.f, 5.f};
  m_d2dDriver->RenderTiltedRect(Center(), knobRadius - markSize.x, knobMarkAngle, markSize, BrushForMode());

  for(int i = 0; i <= 270; i += 30) {
    m_d2dDriver->RenderTiltedRect(Center(), knobRadius, float(-135 - i), {3.f, 1.f}, m_d2dDriver->m_spBlackBrush);
  }

  wchar_t buf[16];
  wsprintf(buf, L"%d%%", int(m_value*100));
  m_d2dDriver->RenderText({center.x - mSize.x/3, center.y - border.y, center.x + mSize.x/3, center.y + border.y}, buf, wcslen(buf));
}


bool CSlider::InMyRegion(Point2F pos)
{
  BOOL b = FALSE;
  m_spRectGeometry->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
  return b;
}


bool CSlider::InRegion(Point2F pos) {
  return InMyRegion(pos) || (mpDial && mpDial->InRegion(pos));
}


void CSlider::MakeDial()
{
  assert(!mpDial);
  mpDial = new DialOnALeash(m_hWnd, m_d2dDriver, this);
  mpDial->ResetState(mPos, mClientArea, {300.f, 300.f});
}


void CSlider::HideDial()
{
  delete mpDial;
  mpDial = nullptr;
}

ID2D1SolidColorBrush* CSlider::BrushForMode()
{
  switch(m_mode) {
  case MODE_ABSOLUTE: return m_d2dDriver->m_spSomePinkishBlueBrush;
  case MODE_RELATIVE: return m_d2dDriver->m_spCornflowerBrush;
  case MODE_DIAL: return m_d2dDriver->m_spSomeGreenishBrush;
  }
  return nullptr;
}
