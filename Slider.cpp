// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) v1ne 
#include "Slider.h"
#include <math.h>

#define INITIAL_OBJ_WIDTH	50
#define INITIAL_OBJ_HEIGHT	200
#define DEFAULT_DIRECTION	0


CSlider::CSlider(HWND hwnd, CD2DDriver* d2dDriver) :
  m_hWnd(hwnd),
  m_d2dDriver(d2dDriver)
{
  // Get the render target for drawing to
  m_spRT = m_d2dDriver->GetRenderTarget();
}

CSlider::~CSlider()
{
}


void CSlider::ManipulationStarted(FLOAT x, FLOAT y) {}


void CSlider::ManipulationDelta(
    FLOAT x,
    FLOAT y,
    FLOAT translationDeltaX,
    FLOAT translationDeltaY,
    FLOAT scaleDelta,
    FLOAT expansionDelta,
    FLOAT rotationDelta,
    FLOAT cumulativeTranslationX,
    FLOAT cumulativeTranslationY,
    FLOAT cumulativeScale,
    FLOAT cumulativeExpansion,
    FLOAT cumulativeRotation,
    bool isExtrapolated) {}

void CSlider::ManipulationCompleted(
    FLOAT x,
    FLOAT y,
    FLOAT cumulativeTranslationX,
    FLOAT cumulativeTranslationY,
    FLOAT cumulativeScale,
    FLOAT cumulativeExpansion,
    FLOAT cumulativeRotation) {}


void CSlider::ResetState(const float startX, const float startY, 
  const int ixClient, const int iyClient,
  const int iScaledWidth, const int iScaledHeight)
{
  // Set width and height of the client area
  // must adjust for dpi aware
  m_iCWidth = iScaledWidth;
  m_iCHeight = iScaledHeight;

  // Initialize width height of object
  m_fWidth   = INITIAL_OBJ_WIDTH;
  m_fHeight  = INITIAL_OBJ_HEIGHT;

  // Set outer elastic border
  UpdateBorders();

  m_value = ::rand() / float(RAND_MAX);

  // Set cooredinates given by processor
  m_fXI = startX;
  m_fYI = startY;

  // Set coordinates used for rendering
  m_fXR = startX;
  m_fYR = startY;

  // Set touch origin to 0
  m_fOX = 0.0f;
  m_fOY = 0.0f;

  // Initialize scaling factor
  m_fFactor = 1.0f;

  // Initialize angle
  m_fAngleCumulative = 0.0f;

  if(!m_spRT) return;
    
  D2D1_SIZE_U  size = {UINT32(ixClient), UINT32(iyClient)};
  if (FAILED(m_spRT->Resize(size)))
  {
    m_d2dDriver->DiscardDeviceResources();
    InvalidateRect(m_hWnd, NULL, FALSE);
  }

  m_pBgBrush = m_d2dDriver->get_SolidBrush(CD2DDriver::SB_LightGrey);
  m_pFgBrush = m_d2dDriver->get_SolidBrush(CD2DDriver::SB_Cornflower);
}

void CSlider::Paint()
{
  if(!(m_spRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
  {
    const auto rotateMatrix = D2D1::Matrix3x2F::Rotation(
      m_fAngleCumulative, 
      D2D1::Point2F(
        m_fXR + m_fWidth/2.0f, 
        m_fYR + m_fHeight/2.0f 
      )
    );
    
    m_spRT->SetTransform(&rotateMatrix);

    // Store the rotate matrix to be used in hit testing
    m_lastMatrix = rotateMatrix;

    const auto bgRect = D2D1::RectF(m_fXR, m_fYR, m_fXR+m_fWidth, m_fYR+m_fHeight);
    m_d2dDriver->CreateGeometryRect(bgRect, &m_spRectGeometry);
    m_spRT->FillGeometry(m_spRectGeometry, m_pBgBrush);

    const auto borderWidth = m_fWidth / 4;
    const auto topBorder = m_fHeight * 10 / 100;
    const auto topEnd = m_fYR + topBorder;
    const auto bottomPos = m_fYR+m_fHeight;
    const auto sliderHeight = bottomPos - topEnd;
    const auto topPos = bottomPos - m_value * sliderHeight;

    const auto fgRect = D2D1::RectF(m_fXR + borderWidth, topPos, m_fXR+m_fWidth - borderWidth, bottomPos);
    ID2D1RectangleGeometryPtr fgGeometry;
    m_d2dDriver->CreateGeometryRect(fgRect, &fgGeometry);
    m_spRT->FillGeometry(fgGeometry, m_pFgBrush);

    wchar_t buf[16];
    wsprintf(buf, L"%d%%", int(m_value*100));
    m_d2dDriver->RenderText({m_fXR, m_fYR, m_fXR + m_fWidth, m_fYR + topBorder}, buf, wcslen(buf));

    // Restore our transform to nothing
    const auto identityMatrix = D2D1::Matrix3x2F::Identity();
    m_spRT->SetTransform(&identityMatrix);
  }
}

void CSlider::Translate(float fdx, float fdy, bool bInertia)
{
  auto offsetX = m_fOX - fdx;

  return;
  m_fdX = fdx;
  m_fdY = fdy;

  float fOffset[2];
  fOffset[0] = m_fOX - m_fdX;
  fOffset[1] = m_fOY - m_fdY;

  // Translate based on the offset caused by rotating 
  // and scaling in order to vary rotational behavior depending 
  // on where the manipulation started
  
  if(m_fAngleApplied != 0.0f)
  {
    float v1[2];
    v1[0] = GetCenterX() - fOffset[0];
    v1[1] = GetCenterY() - fOffset[1];

    float v2[2];
    RotateVector(v1, v2, m_fAngleApplied);

    m_fdX += v2[0] - v1[0];
    m_fdY += v2[1] - v1[1];
  }

  if(m_fFactor != 1.0f)
  {
    float v1[2];
    v1[0] = GetCenterX() - fOffset[0];
    v1[1] = GetCenterY() - fOffset[1];

    float v2[2];
    v2[0] = v1[0] * m_fFactor;
    v2[1] = v1[1] * m_fFactor;
    
    m_fdX += v2[0] - v1[0];
    m_fdY += v2[1] - v1[1];
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
    ComputeElasticPoint(m_fXI, &m_fXR, m_iBorderX);
    ComputeElasticPoint(m_fYI, &m_fYR, m_iBorderY);
  }
  else
  {
    m_fXR = m_fXI;
    m_fYR = m_fYI;

    // Make sure it stays on screen
    EnsureVisible();
  }
}

void CSlider::EnsureVisible()
{
  m_fXR = max(0,min(m_fXI, (float)m_iCWidth-m_fWidth));
  m_fYR = max(0,min(m_fYI, (float)m_iCHeight-m_fHeight));
  RestoreRealPosition();
}

void CSlider::Scale(const float dFactor)
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

    m_fWidth = min(min(m_iCWidth, m_iCHeight), m_fWidth);
    m_fHeight = min(min(m_iCWidth, m_iCHeight), m_fHeight);
  }

  // Readjust borders for the objects new size
  UpdateBorders();
}

void CSlider::Rotate(const float fAngle)
{
  m_fAngleCumulative += fAngle;
  m_fAngleApplied = fAngle;
}

void CSlider::SetManipulationOrigin(float x, float y)
{
  m_fOX = x;
  m_fOY = y;
}

// Helper method that rotates a vector using basic math transforms
void CSlider::RotateVector(float *vector, float *tVector, float fAngle)
{
  auto fAngleRads = fAngle / 180.0f * 3.14159f;
  auto fSin = float(sin(fAngleRads));
  auto fCos = float(cos(fAngleRads));

  auto fNewX = (vector[0]*fCos) - (vector[1]*fSin);
  auto fNewY = (vector[0]*fSin) + (vector[1]*fCos);

  tVector[0] = fNewX;
  tVector[1] = fNewY;
}


// Hit testing method handled with Direct2D
bool CSlider::InRegion(LONG x, LONG y)
{
  BOOL b = FALSE;
  
  m_spRectGeometry->FillContainsPoint(
    D2D1::Point2F((float)x, (float)y),
    &m_lastMatrix, 
    &b
  );
  return b;
}

// Sets the internal coordinates to render coordinates
void CSlider::RestoreRealPosition()
{
  m_fXI = m_fXR;
  m_fYI = m_fYR;
}

void CSlider::UpdateBorders()
{
  m_iBorderX = m_iCWidth  - (int)m_fWidth;
  m_iBorderY = m_iCHeight - (int)m_fHeight;
}

// Computes the the elastic point and sets the render coordinates
void CSlider::ComputeElasticPoint(float fIPt, float *fRPt, int iBSize)
{
  // If the border size is 0 then do not attempt
  // to calculate the render point for elasticity
  if(iBSize == 0)
    return;

  // Calculate render coordinate for elastic border effect

  // Divide the cumulative translation vector by the max border size
  auto q = int(fabsf(fIPt) / iBSize);
  int direction = q % 2;
  
  // Calculate the remainder this is the new render coordinate
  float newPt = fabsf(fIPt) - (float)(iBSize*q);
  
  if (direction == DEFAULT_DIRECTION)
  {
    *fRPt = newPt;
  }
  else
  {
    *fRPt = (float)iBSize - newPt;
  }
}

float CSlider::GetPosY() { return m_fYI; }
float CSlider::GetPosX() { return m_fXI; }
float CSlider::GetWidth() { return m_fWidth; }
float CSlider::GetHeight() { return m_fHeight; }
float CSlider::GetCenterX() { return m_fXI + m_fWidth/2.0f; }
float CSlider::GetCenterY() { return m_fYI + m_fHeight/2.0f; }
