// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) v1ne

#include "ManipulationEventsink.h"
#include "ViewBase.h"

#include <manipulations.h>
#include <manipulations_i.c>
#include <math.h>

#define DEFAULT_DIRECTION	0

bool gShiftPressed = false;


ViewBase::ViewBase(HWND hWnd, CD2DDriver* pD2dDriver)
  : mhWnd(hWnd)
  , mpRenderTarget(pD2dDriver->GetRenderTarget())
  , mD2dDriver(pD2dDriver)
{ InitializeBase(); }

ViewBase::~ViewBase() {
  mManipulationEventSink->RemoveConnPt();
  mManipulationEventSink->Release();
  mManipulationEventSink = NULL;

  mInertiaEventSink->RemoveConnPt();
  mInertiaEventSink->Release();
  mInertiaEventSink = NULL;

  mpManipulationProc->Release();
  mpInertiaProc->Release();
}


bool ViewBase::InitializeBase() {
  if(FAILED(CoCreateInstance(CLSID_ManipulationProcessor, NULL,
      CLSCTX_INPROC_SERVER, IID_IUnknown, (VOID**)(&mpManipulationProc))))
    return false;

  if(FAILED(CoCreateInstance(CLSID_InertiaProcessor, NULL,
      CLSCTX_INPROC_SERVER, IID_IUnknown, (VOID**)(&mpInertiaProc))))
    return false;

  if(!mCanRotate) {
    auto manipulations = MANIPULATION_PROCESSOR_MANIPULATIONS::MANIPULATION_ALL;
    // TODO: Besser lösen!
    //manipulations &= ~MANIPULATION_ROTATE;
    manipulations = MANIPULATION_ROTATE;
    mpManipulationProc->put_SupportedManipulations(manipulations);
  }

  mManipulationEventSink = new CManipulationEventSink(mhWnd, this, mpManipulationProc, mpInertiaProc);
  if(!mManipulationEventSink->SetupConnPt(mpManipulationProc))
    return false;

  mInertiaEventSink = new CManipulationEventSink(mhWnd, this, nullptr, mpInertiaProc);
  if(!mInertiaEventSink->SetupConnPt(mpInertiaProc))
    return false;

  mIsInertiaActive = FALSE;

  return true;
}


bool ViewBase::HandleTouchEvent(TouchEventType type, Point2F pos, const TOUCHINPUT* pData)
{
  bool success = false;
  switch(type) {
  case DOWN:
    success = SUCCEEDED(mpManipulationProc->ProcessDownWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));
    break;
  case MOVE:
    success = SUCCEEDED(mpManipulationProc->ProcessMoveWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));
    break;
  case UP:
    success = SUCCEEDED(mpManipulationProc->ProcessUpWithTime(pData->dwID, pos.x, pos.y, pData->dwTime));
    break;
  case INERTIA:
    if(mIsInertiaActive) {
      BOOL bCompleted = FALSE;
      success = SUCCEEDED(mpInertiaProc->Process(&bCompleted));
    }
  }

  return success;
}


void CTransformableDrawingObject::ResetState(Point2F start, Point2F clientArea, Point2F initialSize)
{
  mClientArea = clientArea;
  mSize = initialSize;

  // Set outer elastic border
  UpdateBorders();

  mPos = start;
  mRenderPos = start;
  mManipulationStartPos = Point2F{0.f};

  m_fFactor = 1.0f;
  m_fAngleCumulative = 0.0f;
}


void CTransformableDrawingObject::Translate(Point2F delta, bool bInertia)
{
  const auto Offset = mManipulationStartPos - delta;

  // Translate based on the offset caused by rotating
  // and scaling in order to vary rotational behavior depending
  // on where the manipulation started

  auto v1 = Center() - Offset;
  if(m_fAngleApplied != 0.0f)
  {
    auto v2 = rotateDeg(v1, -m_fAngleApplied);
    delta += v2 - v1;
  }

  if(m_fFactor != 1.0f)
  {
    auto v2 = v1 * m_fFactor;
    auto temp = v2 - v1;

    delta += v2 - v1;
  }

  mPos += delta;

  // The following code handles the effect for
  // bouncing off the edge of the screen.  It takes
  // the x,y coordinates computed by the inertia processor
  // and calculates the appropriate render coordinates
  // in order to achieve the effect.

  if (bInertia)
  {
    ComputeElasticPoint(mPos.x, &mRenderPos.x, mRightBottomBorders.x);
    ComputeElasticPoint(mPos.y, &mRenderPos.y, mRightBottomBorders.y);
  }
  else
  {
    mRenderPos = mPos;

    // Make sure it stays on screen
    EnsureVisible();
  }
}

void CTransformableDrawingObject::EnsureVisible()
{
  const auto lastValidBottomRightCoordinate = mClientArea - mSize;
  mRenderPos = maxByComponent(Point2F{0.f}, minByComponent(mPos, lastValidBottomRightCoordinate));
  RestoreRealPosition();
}

void CTransformableDrawingObject::Scale(const float dFactor)
{
  m_fFactor = dFactor;

  auto scaledSize = mSize * (dFactor-1);
  auto scaledPos = scaledSize / 2.f;

  mPos -= scaledPos;
  mSize  += scaledSize;

  // Only limit scaling in the case that the factor is not 1.0

  if(dFactor != 1.0f)
  {
    mPos = maxByComponent(Point2F{0.f}, mPos);
    mSize = minByComponent(Point2F{min(mClientArea.x, mClientArea.y)}, mSize);
  }

  // Readjust borders for the objects new size
  UpdateBorders();
}

void CTransformableDrawingObject::Rotate(const float fAngle)
{
  m_fAngleCumulative += fAngle;
  m_fAngleApplied = fAngle;
}

void CTransformableDrawingObject::SetManipulationOrigin(Point2F origin)
{
  mManipulationStartPos = origin;
}

// Sets the internal coordinates to render coordinates
void CTransformableDrawingObject::RestoreRealPosition()
{
  mPos = mRenderPos;
}


void CTransformableDrawingObject::UpdateBorders()
{
  mRightBottomBorders = mClientArea - mSize;
}


// Computes the the elastic point and sets the render coordinates
void CTransformableDrawingObject::ComputeElasticPoint(float fIPt, float *fRPt, float fBSize)
{
  // If the border size is 0 then do not attempt
  // to calculate the render point for elasticity
  if(fBSize == 0)
    return;

  // Calculate render coordinate for elastic border effect

  // Divide the cumulative translation vector by the max border size
  auto q = int(fabsf(fIPt) / fBSize);
  int direction = q % 2;

  // Calculate the remainder this is the new render coordinate
  float newPt = fabsf(fIPt) - fBSize*q;

  if (direction == DEFAULT_DIRECTION)
  {
    *fRPt = newPt;
  }
  else
  {
    *fRPt = fBSize - newPt;
  }
}


Point2F CTransformableDrawingObject::PivotPoint()
{
  return Center();
}


float CTransformableDrawingObject::PivotRadius()
{
  const auto halfSize = Size() / 2.f;
  return ::sqrtf(::powf(halfSize.x, 2) + ::powf(halfSize.y, 2)) * 0.4f;
}
