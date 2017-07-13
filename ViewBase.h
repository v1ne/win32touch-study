// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) v1ne

#pragma once

#include "D2DDriver.h"
#include "Geometry.h"
#include "ManipulationCallbacks.h"

extern bool gShiftPressed;

class ViewBase: public IManipulationCallbacks {
public:
  ViewBase(HWND hwnd, CD2DDriver* d2dDriver);
  virtual ~ViewBase();
    
  enum TouchEventType {DOWN, MOVE, UP, INERTIA};
  virtual bool HandleTouchEvent(TouchEventType type, Point2F pos, const TOUCHINPUT* pData);

  virtual void Paint() = 0;
  virtual bool InRegion(Point2F pos) = 0;

  inline Point2F Pos() { return mPos; }
  inline Point2F Size() { return mSize; }
  Point2F Center() { return Pos() + Size() / 2.f; }
  virtual Point2F PivotPoint() = 0;
  virtual float PivotRadius() = 0;

protected:
  HWND mhWnd;
  CD2DDriver* mD2dDriver;
  ID2D1HwndRenderTargetPtr mpRenderTarget;

  // Real top-left coordinate of object
  Point2F mPos;
  Point2F mSize;

  IManipulationProcessor* mpManipulationProc = nullptr;
  CManipulationEventSink* mManipulationEventSink = nullptr;
  IInertiaProcessor* mpInertiaProc = nullptr;
  CManipulationEventSink* mInertiaEventSink = nullptr;

private:
  bool InitializeBase();
};


class CTransformableDrawingObject: public ViewBase
{
public:
  CTransformableDrawingObject(HWND hWnd, CD2DDriver* d2dDriver)
    : ViewBase(hWnd, d2dDriver)
  {}

  void ResetState(Point2F start, Point2F clientArea, Point2F initialSize);

  Point2F PivotPoint() override;
  float PivotRadius() override;

protected:
  void RestoreRealPosition();
  void SetManipulationOrigin(Point2F origin);
  void Translate(Point2F delta, bool bInertia);
  void Scale(const float fFactor);
  void Rotate(const float fAngle);

  void ComputeElasticPoint(float fIPt, float* fRPt, float fBorderSize);
  void UpdateBorders(); 
  void EnsureVisible();

  Point2F mManipulationStartPos; // Coordinates of where manipulation started
  Point2F mRenderPos; // Rendered top, left coordinates of object
  Point2F mRightBottomBorders; // Right and bottom borders relative to the object's size
  Point2F mClientArea; // Client width and height

  D2D_MATRIX_3X2_F m_lastMatrix = {}; // Keeps the last matrix used to perform the rotate transform
  float m_fFactor; // Scaling factor applied to the object
  float m_fAngleCumulative; // Cumulative angular rotation applied to the object
  float m_fAngleApplied; // Current angular rotation applied to object
};
