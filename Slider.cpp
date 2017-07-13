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

  void ManipulationDelta(ViewBase::ManipDeltaParams params) {
    float rads = 180.0f / 3.14159f;

    //SetManipulationOrigin(params.pos);
    Rotate(params.dRotation * rads);
    Scale(params.dScale);
    //Translate(params.dTranslation, params.isExtrapolated);

    const auto rawValue = mpSlider->mRawTouchValue += params.dRotation / (2*3.14159f);
    mpSlider->mValue = ::fmaxf(0, ::fminf(1, rawValue));
  }

  void ManipulationCompleted(ViewBase::ManipCompletedParams params) {
    mIsShown = false;
  }

  void Paint() override {
    if (!mIsShown)
      return;

    if((mpRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
      return;

    const auto rotateMatrix = D2D1::Matrix3x2F::Rotation(
      0,//m_fAngleCumulative,
      (mRenderPos + mSize / 2.f).to<D2D1_POINT_2F>());

    mpRenderTarget->SetTransform(&rotateMatrix);
    m_lastMatrix = rotateMatrix;

    const auto pos = Center();
    const auto innerRadius = mSize.x / 2.f;
    const auto outerRadius = innerRadius * 1.5f;
    D2D1_ELLIPSE background = {pos.to<D2D1_POINT_2F>(), outerRadius, outerRadius};
    mD2dDriver->m_spD2DFactory->CreateEllipseGeometry(background, &mBackground);
    mpRenderTarget->FillGeometry(mBackground, mD2dDriver->m_spTransparentWhiteBrush);

    mpRenderTarget->FillEllipse({pos.to<D2D1_POINT_2F>(), innerRadius, innerRadius}, mD2dDriver->m_spWhiteBrush);
    mpRenderTarget->FillEllipse({pos.to<D2D1_POINT_2F>(), 30.f, 30.f}, mD2dDriver->m_spDarkGreyBrush);

    const auto shortMarkSize = Point2F{10.f, 1.f};
    const auto longMarkSize = Point2F{15.f, 1.f};
    const auto angularOffset = -mpSlider->mRawTouchValue * 360.f;
    for(float i = 0; i < 360.f; i += 3.6f) {
      auto markSize = (int(i) % 36) == 0 ? longMarkSize : shortMarkSize;
      mD2dDriver->RenderTiltedRect(pos, innerRadius - markSize.x, i + angularOffset, markSize, mD2dDriver->m_spDarkGreyBrush);
    }

    const auto triangleAngle = 180.f;
      const auto triangleStrokeSize = Point2F{16.f, 4.f};
      const auto vecToTriangle = rotateDeg(Vec2Right(innerRadius), triangleAngle);
      mD2dDriver->RenderTiltedRect(pos + vecToTriangle, 0, triangleAngle + 45, triangleStrokeSize, mD2dDriver->m_spBlackBrush);
      mD2dDriver->RenderTiltedRect(pos + vecToTriangle, 0, triangleAngle - 45, triangleStrokeSize, mD2dDriver->m_spBlackBrush);

    const auto identityMatrix = D2D1::Matrix3x2F::Identity();
    mpRenderTarget->SetTransform(&identityMatrix);
  }

  bool InRegion(Point2F pos) override {
    if (!mIsShown || !mBackground) return false;

    BOOL b = FALSE;
    mBackground->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
    return b;
  }

  Point2F PivotPoint() override {
    return Pos();
  }


  float PivotRadius() override {
    return mSize.x;
  }

  bool mIsShown = false;
  ID2D1EllipseGeometryPtr mBackground;
  CSlider* mpSlider;
};

CSlider::CSlider(HWND hWnd, CD2DDriver* d2dDriver, SliderType type, InteractionMode mode)
  : CTransformableDrawingObject(hWnd, d2dDriver)
  , mMode(mode)
  , mType(type)
  , mValue(::rand() / float(RAND_MAX))
{ }

CSlider::~CSlider() {
  HideDial();
}


void CSlider::ManipulationStarted(Point2F pos) {
  RestoreRealPosition();

  mRawTouchValue = mValue;

  if(mMode == MODE_DIAL) {
    MakeDial(pos);
    mpDial->mIsShown=true;
    mpDial->ManipulationStarted(pos);
  } else {
    if(!gShiftPressed)
      HandleTouch(pos.y, 0.f, 0.f);
  }
}


void CSlider::ManipulationDelta(ViewBase::ManipDeltaParams params) {
  if(gShiftPressed) {
    float rads = 180.0f / 3.14159f;

    SetManipulationOrigin(params.pos);
    Rotate(params.dRotation * rads);
    Scale(params.dScale);
    Translate(params.dTranslation, params.isExtrapolated);
  } else if(mMode == MODE_DIAL) {
    mpDial->ManipulationDelta(params);
    if (!InRegion(params.pos)) mpDial->mIsShown = true;
  } else {
    HandleTouch(params.pos.y, params.sumTranslation.x, params.dTranslation.y);
  }
}


void CSlider::ManipulationCompleted(ViewBase::ManipCompletedParams params) {
  if(mMode == MODE_DIAL) {
    if(mpDial) mpDial->ManipulationCompleted(params);
    HideDial();
  } else {
    if(!gShiftPressed)
      HandleTouch(params.pos.y, params.sumTranslation.x, 0.f);
  }
}


void CSlider::HandleTouch(float y, float cumultiveTranslationX, float deltaY) {
  switch(mMode) {
  case MODE_ABSOLUTE:
    HandleTouchInAbsoluteInteractionMode(y);
    break;
  case MODE_RELATIVE:
    HandleTouchInRelativeInteractionMode(cumultiveTranslationX, deltaY);
    break;
  }
}


void CSlider::HandleTouchInAbsoluteInteractionMode(float y) {
  mValue = ::fmaxf(0, ::fminf(1, (mBottomPos - y) / mSliderHeight));
}


void CSlider::HandleTouchInRelativeInteractionMode(float cumulativeTranslationX, float deltaY) {
  const auto dragScalingFactor = 1 + ::fabsf(cumulativeTranslationX) / (2 * mSize.x);

  mRawTouchValue -= deltaY / mSliderHeight / dragScalingFactor;
  mValue = ::fmaxf(0, ::fminf(1, mRawTouchValue));
}


void CSlider::Paint() {
  const auto rotateMatrix = D2D1::Matrix3x2F::Rotation(
    m_fAngleCumulative,
    (mRenderPos + mSize / 2.f).to<D2D1_POINT_2F>());

  mpRenderTarget->SetTransform(&rotateMatrix);

  // Store the rotate matrix to be used in hit testing
  m_lastMatrix = rotateMatrix;

  const auto bgRect = D2D1::RectF(mRenderPos.x, mRenderPos.y, mRenderPos.x+mSize.x, mRenderPos.y+mSize.y);
  mD2dDriver->m_spD2DFactory->CreateRectangleGeometry(bgRect, &mpOutlineGeometry);
  mpRenderTarget->FillGeometry(mpOutlineGeometry, mD2dDriver->m_spLightGreyBrush);

  switch(mType) {
  case TYPE_SLIDER:
    PaintSlider();
    break;
  case TYPE_KNOB:
    PaintKnob();
    break;
  }

  // Restore our transform to nothing
  const auto identityMatrix = D2D1::Matrix3x2F::Identity();
  mpRenderTarget->SetTransform(&identityMatrix);

  if(mpDial) mpDial->Paint();
}


void CSlider::PaintSlider()
{
  const auto borderWidth = mSize.x / 4;
  const auto topBorder = mSize.y * 10 / 100;
  const auto topEnd = mRenderPos.y + topBorder;
  const auto bottomPos = mRenderPos.y+mSize.y;
  const auto sliderHeight = bottomPos - topEnd;
  const auto topPos = bottomPos - mValue * sliderHeight;

  mBottomPos = bottomPos;
  mSliderHeight = sliderHeight;

  const auto fgRect = D2D1::RectF(mRenderPos.x + borderWidth, topPos, mRenderPos.x+mSize.x - borderWidth, bottomPos);
  ID2D1RectangleGeometryPtr fgGeometry;
  mD2dDriver->m_spD2DFactory->CreateRectangleGeometry(fgRect, &fgGeometry);
  mpRenderTarget->FillGeometry(fgGeometry, BrushForMode());

  wchar_t buf[16];
  wsprintf(buf, L"%d%%", int(mValue*100));
  mD2dDriver->RenderText({mRenderPos.x, mRenderPos.y, mRenderPos.x + mSize.x, mRenderPos.y + topBorder}, buf, wcslen(buf));
}


void CSlider::PaintKnob() {
  const auto border = POINTF{mSize.x / 8, mSize.y / 8};
  const auto center = Center().to<D2D1_POINT_2F>();
  const auto knobRadius = ::fminf((mSize.x - border.x)/2, (mSize.y - border.y)/2);

  mSliderHeight = mSize.y * 3;
  mBottomPos = mRenderPos.y + mSize.y / 2;

  D2D1_ELLIPSE knobOutlineParams = {center, knobRadius, knobRadius};
  ID2D1EllipseGeometryPtr knobOutline;
  mD2dDriver->m_spD2DFactory->CreateEllipseGeometry(knobOutlineParams, &knobOutline);
  mpRenderTarget->FillGeometry(knobOutline, mD2dDriver->m_spDarkGreyBrush);

  const auto knobMarkAngle = -135.f - mValue * 270;
  const auto markSize = Point2F{10.f, 5.f};
  mD2dDriver->RenderTiltedRect(Center(), knobRadius - markSize.x, knobMarkAngle, markSize, BrushForMode());

  for(int i = 0; i <= 270; i += 30) {
    mD2dDriver->RenderTiltedRect(Center(), knobRadius, float(-135 - i), {3.f, 1.f}, mD2dDriver->m_spBlackBrush);
  }

  wchar_t buf[16];
  wsprintf(buf, L"%d%%", int(mValue*100));
  mD2dDriver->RenderText({center.x - mSize.x/3, center.y - border.y, center.x + mSize.x/3, center.y + border.y}, buf, wcslen(buf));
}


bool CSlider::InMyRegion(Point2F pos) {
  BOOL b = FALSE;
  mpOutlineGeometry->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
  return b;
}


bool CSlider::InRegion(Point2F pos) {
  return InMyRegion(pos) || (mpDial && mpDial->InRegion(pos));
}


void CSlider::MakeDial(Point2F center) {
  if(mpDial)
    ::OutputDebugStringA("Dial already present. You're too fast!");
  else
    mpDial = new DialOnALeash(mhWnd, mD2dDriver, this);

  const auto dialSize = Point2F{200.f};
  mpDial->ResetState(center - dialSize/2.f, mClientArea, dialSize);
}


void CSlider::HideDial()
{
  delete mpDial;
  mpDial = nullptr;
}

ID2D1SolidColorBrush* CSlider::BrushForMode() {
  switch(mMode) {
  case MODE_ABSOLUTE: return mD2dDriver->m_spSomePinkishBlueBrush;
  case MODE_RELATIVE: return mD2dDriver->m_spCornflowerBrush;
  case MODE_DIAL: return mD2dDriver->m_spSomeGreenishBrush;
  }
  return nullptr;
}
