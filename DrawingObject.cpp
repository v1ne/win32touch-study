// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) v1ne

#include "DrawingObject.h"
#include <math.h>

#define DEFAULT_DIRECTION	0


void CTransformableDrawingObject::ResetState(Point2F start, Point2F clientArea, Point2F initialSize)
{
  // Set width and height of the client area
  // must adjust for dpi aware
  m_ClientAreaWidth = clientArea.x;
  m_ClientAreaHeight = clientArea.y;

  // Initialize width height of object
  m_fWidth   = initialSize.x;
  m_fHeight  = initialSize.y;

  // Set outer elastic border
  UpdateBorders();

  // Set cooredinates given by processor
  m_fXI = start.x;
  m_fYI = start.y;

  // Set coordinates used for rendering
  m_fXR = start.x;
  m_fYR = start.y;

  // Set touch origin to 0
  m_fOX = 0.0f;
  m_fOY = 0.0f;

  // Initialize scaling factor
  m_fFactor = 1.0f;

  // Initialize angle
  m_fAngleCumulative = 0.0f;
}


void CTransformableDrawingObject::Translate(Point2F delta, bool bInertia)
{
  m_fdX = delta.x;
  m_fdY = delta.y;

  auto Offset = Point2F(m_fOX - m_fdX, m_fOY - m_fdY);

  // Translate based on the offset caused by rotating
  // and scaling in order to vary rotational behavior depending
  // on where the manipulation started

  auto v1 = Center() - Offset;
  if(m_fAngleApplied != 0.0f)
  {
    Point2F v2;
    RotateVector(v1.raw(), v2.raw(), m_fAngleApplied);
    auto temp = v2 - v1;

    m_fdX += temp.x;
    m_fdY += temp.y;
  }

  if(m_fFactor != 1.0f)
  {
/* ??? TODO
    float v1[2];
    v1[0] = GetCenterX() - fOffset[0];
    v1[1] = GetCenterY() - fOffset[1];
*/
    auto v2 = v1 * m_fFactor;
    auto temp = v2 - v1;

    m_fdX += temp.x;
    m_fdY += temp.y;
  }

  m_fXI += m_fdX;
  m_fYI += m_fdY;

  // The following code handles the effect for
  // bouncing off the edge of the screen.  It takes
  // the x,y coordinates computed by the inertia processor
  // and calculates the appropriate render coordinates
  // in order to achieve the effect.

  if (bInertia)
  {
    ComputeElasticPoint(m_fXI, &m_fXR, m_fBorderX);
    ComputeElasticPoint(m_fYI, &m_fYR, m_fBorderY);
  }
  else
  {
    m_fXR = m_fXI;
    m_fYR = m_fYI;

    // Make sure it stays on screen
    EnsureVisible();
  }
}

void CTransformableDrawingObject::EnsureVisible()
{
  m_fXR = max(0,min(m_fXI, (float)m_ClientAreaWidth-m_fWidth));
  m_fYR = max(0,min(m_fYI, (float)m_ClientAreaHeight-m_fHeight));
  RestoreRealPosition();
}

void CTransformableDrawingObject::Scale(const float dFactor)
{
  m_fFactor = dFactor;

  float scaledW = (dFactor-1) * m_fWidth;
  float scaledH = (dFactor-1) * m_fHeight;
  float scaledX = scaledW/2.0f;
  float scaledY = scaledH/2.0f;

  m_fXI -= scaledX;
  m_fYI -= scaledY;

  m_fWidth  += scaledW;
  m_fHeight += scaledH;

  // Only limit scaling in the case that the factor is not 1.0

  if(dFactor != 1.0f)
  {
    m_fXI = max(0, m_fXI);
    m_fYI = max(0, m_fYI);

    m_fWidth = min(min(m_ClientAreaWidth, m_ClientAreaHeight), m_fWidth);
    m_fHeight = min(min(m_ClientAreaWidth, m_ClientAreaHeight), m_fHeight);
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
  m_fOX = origin.x;
  m_fOY = origin.y;
}

// Helper method that rotates a vector using basic math transforms
void CTransformableDrawingObject::RotateVector(float *vector, float *tVector, float fAngle)
{
  auto fAngleRads = fAngle / 180.0f * 3.14159f;
  auto fSin = float(sin(fAngleRads));
  auto fCos = float(cos(fAngleRads));

  auto fNewX = (vector[0]*fCos) - (vector[1]*fSin);
  auto fNewY = (vector[0]*fSin) + (vector[1]*fCos);

  tVector[0] = fNewX;
  tVector[1] = fNewY;
}


// Sets the internal coordinates to render coordinates
void CTransformableDrawingObject::RestoreRealPosition()
{
  m_fXI = m_fXR;
  m_fYI = m_fYR;
}


void CTransformableDrawingObject::UpdateBorders()
{
  m_fBorderX = m_ClientAreaWidth  - m_fWidth;
  m_fBorderY = m_ClientAreaHeight - m_fHeight;
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

