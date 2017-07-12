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
#include <windows.h>

extern bool gShiftPressed;

class CDrawingObject {
public:
  CDrawingObject(HWND hwnd, CD2DDriver* d2dDriver)
    : m_hWnd(hwnd)
    , m_spRT(d2dDriver->GetRenderTarget())
    , m_d2dDriver(d2dDriver) {}
  virtual ~CDrawingObject() {};

  virtual void ManipulationStarted(Point2F pos) = 0;
  virtual void ManipulationDelta(Point2F pos, Point2F dTranslation,
    float dScale, float dExtension, float dRotation,
    Point2F sumTranslation, float sumScale, float sumExpansion, float sumRotation,
    bool isExtrapolated) = 0;
  virtual void ManipulationCompleted(Point2F pos, Point2F sumTranslation,
    float sumScale, float sumExpansion, float sumRotation) = 0;
    
  virtual void Paint() = 0;
  virtual bool InRegion(Point2F pos) = 0;

  Point2F Pos() { return {m_fXI, m_fYI}; }
  Point2F Size() { return {m_fWidth, m_fHeight}; }
  Point2F Center() { return Pos() + Size() / 2.f; }

protected:
  HWND m_hWnd;
  CD2DDriver* m_d2dDriver;
  ID2D1HwndRenderTargetPtr m_spRT;

  // Internal top, left coordinates of object (Real inertia values)
  float m_fXI = 0.f;
  float m_fYI = 0.f;
  
  // Width and height of the object
  float m_fWidth = 0.f;
  float m_fHeight = 0.f;
};


class CTransformableDrawingObject: public CDrawingObject
{
public:
  CTransformableDrawingObject(HWND hWnd, CD2DDriver* d2dDriver) : CDrawingObject(hWnd, d2dDriver) {}
  void ResetState(Point2F start, Point2F clientArea, Point2F initialSize);

protected:
  void RestoreRealPosition();
  void SetManipulationOrigin(Point2F origin);
  void Translate(Point2F delta, bool bInertia);
  void Scale(const float fFactor);
  void Rotate(const float fAngle);

  void RotateVector(float* vector, float* tVector, float fAngle);
  void ComputeElasticPoint(float fIPt, float* fRPt, float fBorderSize);
  void UpdateBorders(); 
  void EnsureVisible();

  // Keeps the last matrix used to perform the rotate transform
  D2D_MATRIX_3X2_F m_lastMatrix;

  // Coordinates of where manipulation started
  float m_fOX;
  float m_fOY;
  
  // Rendered top, left coordinates of object
  float m_fXR;
  float m_fYR;

  // Scaling factor applied to the object
  float m_fFactor;

  // Cumulative angular rotation applied to the object
  float m_fAngleCumulative;

  // Current angular rotation applied to object
  float m_fAngleApplied;

  // Delta x, y values
  float m_fdX;
  float m_fdY;

  // Right and bottom borders relative to the object's size
  float m_fBorderX;
  float m_fBorderY;

  // Client width and height
  float m_ClientAreaWidth;
  float m_ClientAreaHeight;
};
