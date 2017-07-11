// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "D2DDriver.h"
#include <windows.h>

class CDrawingObject {
public:
  virtual ~CDrawingObject() {};

  virtual void Paint() = 0;
  virtual void Translate(float fdx, float fdy, bool bInertia) = 0;
  virtual void Scale(const float fFactor) = 0;
  virtual void Rotate(const float fAngle) = 0;
  virtual bool InRegion(long lX, long lY) = 0;
  virtual void RestoreRealPosition() = 0;

  // Public set method
  virtual void SetManipulationOrigin(float x, float y) = 0;

  // Public get methods
  virtual float GetPosY() = 0;
  virtual float GetPosX() = 0;
  virtual float GetWidth() = 0;
  virtual float GetHeight() = 0;
  virtual float GetCenterX() = 0;
  virtual float GetCenterY() = 0;
};
