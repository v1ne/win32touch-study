// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "Square.h"
#include <math.h>

#define INITIAL_OBJ_WIDTH	200
#define INITIAL_OBJ_HEIGHT	200
#define DEFAULT_DIRECTION	0

CSquare::CSquare(HWND hWnd, CD2DDriver* d2dDriver,  const DrawingColor colorChoice)
  : CTransformableDrawingObject(hWnd, d2dDriver)
{
  // Determines what brush to use for drawing this object and
  // gets the brush from the D2DDriver class

  switch (colorChoice){
      case Blue:
          m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Blue);
          break;
      case Orange:
          m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Orange);
          break;
      case Green:
          m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Green);
          break;
      case Red:
          m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Red);
          break;
      default:
          m_currBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Blue);
  }

}

CSquare::~CSquare()
{
}


void CSquare::ManipulationStarted(Point2F)
{
    RestoreRealPosition();
}


void CSquare::ManipulationDelta(CDrawingObject::ManipDeltaParams params)
{
    float rads = 180.0f / 3.14159f;

    SetManipulationOrigin(params.pos);

    Rotate(params.dRotation * rads);
    Scale(params.dScale);
    Translate(params.dTranslation, params.isExtrapolated);
}

void CSquare::ManipulationCompleted(CDrawingObject::ManipCompletedParams)
{
}

void CSquare::Paint()
{
    if(!(m_spRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        float fGlOffset = 2.5f;

        // Setup our matrices for performing transforms

        D2D_MATRIX_3X2_F rotateMatrix;
        D2D_MATRIX_3X2_F identityMatrix;
        identityMatrix = D2D1::Matrix3x2F::Identity();

        // Apply rotate transform

        rotateMatrix = D2D1::Matrix3x2F::Rotation(
            m_fAngleCumulative,
            (mRenderPos + mSize / 2.f).to<D2D1_POINT_2F>());

        m_spRT->SetTransform(&rotateMatrix);

        // Store the rotate matrix to be used in hit testing
        m_lastMatrix = rotateMatrix;

        // Get glossy brush
        m_pGlBrush = m_d2dDriver->get_GradBrush(CD2DDriver::GRB_Glossy);

        // Set positions of gradients based on the new coordinates of the objecs
        m_currBrush->SetStartPoint(mRenderPos.to<D2D1_POINT_2F>());
        m_currBrush->SetEndPoint(D2D1::Point2F(
            mRenderPos.x,
            mRenderPos.y + mSize.y));

        m_pGlBrush->SetStartPoint(mRenderPos.to<D2D1_POINT_2F>());  
        m_pGlBrush->SetEndPoint(D2D1::Point2F(
            mRenderPos.x + mSize.x/15.0f,
            mRenderPos.y + mSize.y/2.0f));

        // Create rectangle to draw

        D2D1_RECT_F rectangle = D2D1::RectF(
            mRenderPos.x,
            mRenderPos.y,
            mRenderPos.x+mSize.x,
            mRenderPos.y+mSize.y
        );

        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
            rectangle,
            10.0f, 10.0f
        );

        // Create glossy effect

        D2D1_RECT_F glossyRect = D2D1::RectF(
            mRenderPos.x+fGlOffset,
            mRenderPos.y+fGlOffset,
            mRenderPos.x+mSize.x-fGlOffset,
            mRenderPos.y+mSize.y/2.0f
        );

        D2D1_ROUNDED_RECT glossyRoundedRect = D2D1::RoundedRect(
            glossyRect,
            10.0f,
            10.0f
        );

        // D2D requires that a geometry is created for the rectangle
        m_d2dDriver->m_spD2DFactory->CreateRoundedRectangleGeometry(
            roundedRect,
            &m_spRoundedRectGeometry
        );

        // Fill the geometry that was created
        m_spRT->FillGeometry(
            m_spRoundedRectGeometry,
            m_currBrush
        );

        // Draw glossy effect
        m_spRT->FillRoundedRectangle(
            &glossyRoundedRect,
            m_pGlBrush
        );

        // Restore our transform to nothing
        m_spRT->SetTransform(&identityMatrix);
    }
}

// Hit testing method handled with Direct2D
bool CSquare::InRegion(Point2F pos)
{
    BOOL b = FALSE;
    m_spRoundedRectGeometry->FillContainsPoint(pos.to<D2D1_POINT_2F>(), &m_lastMatrix, &b);
    return b;
}
