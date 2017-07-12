// Copyright (c) v1ne

#include "Geometry.h"

#include <math.h>

Point2F rotateRad(const Point2F p, const float radAngle) {
  auto fSin = float(sin(radAngle));
  auto fCos = float(cos(radAngle));

  return {
      p.x * fCos + p.y * fSin,
    - p.x * fSin + p.y * fCos};
}
