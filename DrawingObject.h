// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "D2DDriver.h"
#include <windows.h>

extern bool gShiftPressed;

class CDrawingObject {
public:
  virtual ~CDrawingObject() {};

  virtual void ManipulationStarted(FLOAT x, FLOAT y) = 0;

  // Handles event when the manipulation is progress
  virtual void ManipulationDelta(
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
      bool isExtrapolated) = 0;

  // Handles event when the manipulation ends
  virtual void ManipulationCompleted(
      FLOAT x,
      FLOAT y,
      FLOAT cumulativeTranslationX,
      FLOAT cumulativeTranslationY,
      FLOAT cumulativeScale,
      FLOAT cumulativeExpansion,
      FLOAT cumulativeRotation) = 0;
    
  virtual void Paint() = 0;
  virtual bool InRegion(float x, float y) = 0;

  // Public get methods
  virtual float GetPosY() = 0;
  virtual float GetPosX() = 0;
  virtual float GetWidth() = 0;
  virtual float GetHeight() = 0;
  virtual float GetCenterX() = 0;
  virtual float GetCenterY() = 0;
};
