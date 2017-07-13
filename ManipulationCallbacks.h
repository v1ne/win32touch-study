// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) v1ne

#pragma once

#include "Geometry.h"

class CManipulationEventSink;
struct IManipulationProcessor;
struct IInertiaProcessor;

class IManipulationCallbacks {
public:
  struct ManipDeltaParams {
    Point2F pos;
    Point2F dTranslation;
    float dScale;
    float dExtension;
    float dRotation;
    Point2F sumTranslation;
    float sumScale;
    float sumExpansion;
    float sumRotation;
    bool isExtrapolated;
  };

  struct ManipCompletedParams {
    Point2F pos;
    Point2F sumTranslation;
    float sumScale;
    float sumExpansion;
    float sumRotation;
  };

  virtual void ManipulationStarted(Point2F pos) = 0;
  virtual void ManipulationDelta(ManipDeltaParams params) = 0;
  virtual void ManipulationCompleted(ManipCompletedParams params) = 0;
    
  virtual Point2F PivotPoint() = 0;
  virtual float PivotRadius() = 0;

  // This flag is set by the mManipulation event sink
  // when the ManipulationCompleted method is invoked
  bool mIsInertiaActive = false;
};
