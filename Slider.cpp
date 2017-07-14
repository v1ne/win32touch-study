// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) v1ne

#include "Geometry.h"
#include "MidiOutput.h"
#include "Slider.h"

#include <manipulations.h>
#include <math.h>
#include <unordered_map>


extern MidiOutput gMidiOutput;

class DialOnALeash: public CTransformableDrawingObject {
public:
  static constexpr auto sInnerRadius = 150.f;
  static constexpr auto sAngleRange = 320.f;

  DialOnALeash::DialOnALeash(HWND hWnd, CD2DDriver* d2dDriver, CSlider* pParent, Point2F center)
      : CTransformableDrawingObject(hWnd, d2dDriver)
      , mpSlider(pParent) {
    mpManipulationProc->put_SupportedManipulations(
      MANIPULATION_PROCESSOR_MANIPULATIONS::MANIPULATION_ROTATE);
    
    //mpManipulationProc->put_MinimumScaleRotateRadius(100'000.f);
    m_lastMatrix = D2D1::Matrix3x2F::Identity();

    mSize = Point2F{2.f * sInnerRadius + 100.f};
    ResetState(center - mSize/2.f, mClientArea, mSize);
  }

  ~DialOnALeash() override {
    if (!mContactsToTypeMap.empty())
      mpManipulationProc->CompleteManipulation();

    if(mIsInertiaActive)
      mpInertiaProc->Complete();
  };

  bool HandleTouchEvent(TouchEventType type, Point2F pos, const TOUCHINPUT* pData) override {
    bool success = false;
    switch(type) {
    case DOWN: {
      switch(mContactsToTypeMap.size()) {
      case 0:
        mContactsToTypeMap.emplace(pData->dwID, ContactTypes::PivotPoint);
        break;
      case 1:
        if (mContactsToTypeMap.begin()->second == ContactTypes::PivotPoint) {
          mContactsToTypeMap.emplace(pData->dwID, ContactTypes::OuterHandle);
          success = SUCCEEDED(mpManipulationProc->ProcessDownWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));
        } else
          mContactsToTypeMap.emplace(pData->dwID, ContactTypes::PivotPoint);
        break;
      default:
        mContactsToTypeMap.emplace(pData->dwID, ContactTypes::Ignored);
        break;
      }

      //success = SUCCEEDED(mpManipulationProc->ProcessDownWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));
      break; }

    case MOVE: {
      auto iEntry = mContactsToTypeMap.find(pData->dwID);
      if (iEntry != mContactsToTypeMap.end()) {
        switch (iEntry->second) {
        case ContactTypes::PivotPoint:
          mPos = pos - Size()/2.f;
          success = true;
          break;
        case ContactTypes::OuterHandle: {
          const auto center = Center();
          const auto fingerDistance = ::fmaxf(120.f, (center - pos).mag());
          const auto currentOuterRadius = mSize.x/2;
          if (fingerDistance > 0.9f * currentOuterRadius)
            mSize = Point2F{2.f * fingerDistance / 0.9f};
          if (fingerDistance < 0.5f * mSize.x/2.f)
            mSize = Point2F{2.f * fingerDistance / 0.5f};

          mPos = center - mSize/2.f;

          success = SUCCEEDED(mpManipulationProc->ProcessMoveWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));
          break; }
        default:
          break;
        }
      } else
        success = SUCCEEDED(mpManipulationProc->ProcessMoveWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));
      break; }

    case UP: {
      auto iEntry = mContactsToTypeMap.find(pData->dwID);
      if (iEntry != mContactsToTypeMap.end() && iEntry->second == ContactTypes::PivotPoint && mContactsToTypeMap.size() > 1) {
        auto iFirst = mContactsToTypeMap.begin();
        auto iOtherEntry = iFirst != iEntry ? iFirst : std::next(iFirst);
        iOtherEntry->second = ContactTypes::PivotPoint;
        success = SUCCEEDED(mpManipulationProc->CompleteManipulation());
      }
      else
        success = SUCCEEDED(mpManipulationProc->ProcessUpWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));

      mContactsToTypeMap.erase(pData->dwID);
      if (mContactsToTypeMap.empty())
        mpSlider->HideDial();
      break; }

    case INERTIA:
      if(mIsInertiaActive) {
        BOOL bCompleted = FALSE;
        success = SUCCEEDED(mpInertiaProc->Process(&bCompleted));
      }
      break;
    }

  return success;
}

  void ManipulationStarted(Point2F) override {
  }

  void ManipulationDelta(ViewBase::ManipDeltaParams params) {
    float rads = 180.0f / 3.14159f;

    SetManipulationOrigin(mPos);
    Rotate(params.dRotation * rads);
    //Scale(params.dScale);
    //Translate(params.dTranslation, params.isExtrapolated);

    const auto rawValue = mpSlider->mRawTouchValue + params.dRotation / (2*3.14159f);
    const auto clampedRawValue = ::fmaxf(-0.1f, ::fminf(1.1f, rawValue));
    mpSlider->mRawTouchValue = clampedRawValue;
    mpSlider->mValue = ::fmaxf(0, ::fminf(1, clampedRawValue));
    mpSlider->HandleValueChange();
  }

  void ManipulationCompleted(ViewBase::ManipCompletedParams) {
    if (mContactsToTypeMap.empty())
      mIsShown = false;
  }

  void Paint() override {
    if (!mIsShown)
      return;

    if((mpRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
      return;

    const auto identityMatrix = D2D1::Matrix3x2F::Identity();
    mpRenderTarget->SetTransform(&identityMatrix);

    const auto pos = Center();
    const auto innerRadius = sInnerRadius;
    const auto outerRadius = mSize.x/2;
    D2D1_ELLIPSE background = {pos.to<D2D1_POINT_2F>(), outerRadius, outerRadius};
    mD2dDriver->m_spD2DFactory->CreateEllipseGeometry(background, &mBackground);
    mpRenderTarget->FillGeometry(mBackground, mD2dDriver->m_spSemitransparentDarkBrush);

    mpRenderTarget->FillEllipse({pos.to<D2D1_POINT_2F>(), innerRadius, innerRadius}, mD2dDriver->m_spWhiteBrush);
    mpRenderTarget->FillEllipse({pos.to<D2D1_POINT_2F>(), 30.f, 30.f}, mD2dDriver->m_spDarkGreyBrush);

    const auto triangleAngle = 180.f;
    const auto triangleStrokeSize = Point2F{16.f, 4.f};
    const auto vecToTriangle = rotateDeg(Vec2Right(innerRadius + 2.f), triangleAngle);
    mD2dDriver->RenderTiltedRect(pos + vecToTriangle, 0, triangleAngle + 45, triangleStrokeSize, mD2dDriver->m_spWhiteBrush);
    mD2dDriver->RenderTiltedRect(pos + vecToTriangle, 0, triangleAngle - 45, triangleStrokeSize, mD2dDriver->m_spWhiteBrush);

    const auto angleStep = sAngleRange/100;
    const auto bigMarksEvery = 10;
    const auto shortMarkSize = Point2F{10.f, 1.f};
    const auto longMarkSize = Point2F{15.f, 3.f};
    const auto angularOffset = -mpSlider->mRawTouchValue * sAngleRange + triangleAngle;
    wchar_t buf[16];
    int stepCount = 0;
    const auto translateMatrix = D2D1::Matrix3x2F::Translation({0.f, -(innerRadius + 15.f)});
    for(float i = 0; i < (sAngleRange < 360.f ? sAngleRange + angleStep : sAngleRange - angleStep); i += angleStep, ++stepCount) {
      auto finalAngle = angularOffset + i;
      auto markSize = i == 0
        ? Point2F{innerRadius, longMarkSize.y}
        : stepCount % bigMarksEvery == 0 ? longMarkSize : shortMarkSize;
      markSize.x += i / 30.f;
      mD2dDriver->RenderTiltedRect(pos, innerRadius - markSize.x, finalAngle, markSize, 
        (i == 0 || i >= sAngleRange) ? mD2dDriver->m_spBlackBrush : mD2dDriver->m_spDarkGreyBrush);

      if (!(stepCount % bigMarksEvery)) {
        wsprintf(buf, L"%d%%", int(::roundf(100 * i / sAngleRange)));
        const auto finalTransform = translateMatrix * D2D1::Matrix3x2F::Rotation(-finalAngle + 90.f, Center().to<D2D1_POINT_2F>());
        mpRenderTarget->SetTransform(&finalTransform);
        mD2dDriver->RenderMediumText({pos.x-25.f, pos.y-20.f, pos.x + 25.f, pos.y + 20.f}, buf, wcslen(buf), mD2dDriver->m_spWhiteBrush);
        mpRenderTarget->SetTransform(&identityMatrix);
      }
    }

  }

  bool InRegion(Point2F pos) override {
    if (!mIsShown || !mBackground) return false;

    BOOL b = FALSE;
    mBackground->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
    return b;
  }

  float PivotRadius() override {
    return mSize.x;
  }

  enum class ContactTypes {
    PivotPoint,
    OuterHandle,
    Ignored
  };
  std::unordered_map<DWORD, ContactTypes> mContactsToTypeMap;

  bool mIsShown = false;
  ID2D1EllipseGeometryPtr mBackground;
  CSlider* mpSlider;
};



CSlider::CSlider(HWND hWnd, CD2DDriver* d2dDriver, SliderType type, InteractionMode mode, uint8_t numController)
  : CTransformableDrawingObject(hWnd, d2dDriver)
  , mMode(mode)
  , mType(type)
  , mValue(::rand() / float(RAND_MAX))
  , mNumController(numController)
{
  mpManipulationProc->put_SupportedManipulations(MANIPULATION_PROCESSOR_MANIPULATIONS::MANIPULATION_ALL
      & ~MANIPULATION_PROCESSOR_MANIPULATIONS::MANIPULATION_SCALE);
}


CSlider::~CSlider() {
  HideDial();
}

bool CSlider::HandleTouchEvent(TouchEventType type, Point2F pos, const TOUCHINPUT* pData) {
  switch(type) {
  case DOWN: {
    auto success = ViewBase::HandleTouchEvent(type, pos, pData);
    if (mpDial)
      success &= mpDial->HandleTouchEvent(type, pos, pData);
    return success; }
  case INERTIA:
  case UP: {
    bool success = true;
    if (mpDial)
      success = mpDial->HandleTouchEvent(type, pos, pData);
    success &= ViewBase::HandleTouchEvent(type, pos, pData);
    return success; }
  case MOVE:
    if(mpDial)
      return mpDial->HandleTouchEvent(type, pos, pData);
    else
      return ViewBase::HandleTouchEvent(type, pos, pData);
    break;
  }
  return false;
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
#if 0
  } else if(mMode == MODE_DIAL) {
    mpDial->ManipulationDelta(params);
    if (!InRegion(params.pos)) mpDial->mIsShown = true;
#endif
  } else {
    HandleTouch(params.pos.y, params.sumTranslation.x, params.dTranslation.y);
  }
}


void CSlider::ManipulationCompleted(ViewBase::ManipCompletedParams params) {
#if 0
  if(mMode == MODE_DIAL) {
    if(mpDial) mpDial->ManipulationCompleted(params);
    HideDial();
  } else {
#endif
    if(!gShiftPressed)
      HandleTouch(params.pos.y, params.sumTranslation.x, 0.f);
#if 0
  }
#endif
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
  HandleValueChange();
}


void CSlider::HandleTouchInRelativeInteractionMode(float cumulativeTranslationX, float deltaY) {
  const auto dragScalingFactor = 1 + ::fabsf(cumulativeTranslationX) / (2 * mSize.x);

  mRawTouchValue -= deltaY / mSliderHeight / dragScalingFactor;
  mRawTouchValue = ::fmaxf(-0.1f, ::fminf(1.1f, mRawTouchValue));
  mValue = ::fmaxf(0, ::fminf(1, mRawTouchValue));
  HandleValueChange();
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
  mD2dDriver->RenderText({mRenderPos.x, mRenderPos.y, mRenderPos.x + mSize.x, mRenderPos.y + topBorder}, buf, wcslen(buf), mD2dDriver->m_spDimGreyBrush);
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
  mD2dDriver->RenderText({center.x - mSize.x/3, center.y - border.y, center.x + mSize.x/3, center.y + border.y}, buf, wcslen(buf), mD2dDriver->m_spDimGreyBrush);
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
    mpDial = new DialOnALeash(mhWnd, mD2dDriver, this, center);
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


void CSlider::HandleValueChange() {
  auto currentValue = uint8_t(::roundf(127.f * mValue));
  if (currentValue != mLastMidiValue) {
    mLastMidiValue = currentValue;
    gMidiOutput.sendControllerChange(mNumController, currentValue);
  }
}