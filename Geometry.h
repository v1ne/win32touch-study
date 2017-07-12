// Copyright (c) v1ne

#pragma once

template<typename T>
struct Point2 {
  using Self = Point2<T>;

  Point2() : x(), y() {}
  Point2(T x_, T y_) : x(x_), y(y_) {}
  explicit Point2(T xy) : x(xy), y(xy) {}
  Point2(const Self& other) = default;
  Point2(Self&&) = default;
  template<typename U>
  Point2(const Point2<U> other) : x((T)other.x), y((T)other.y) {}

  Point2& operator=(const Point2& other) = default;
  Point2& operator=(Point2&&) = default;

  const T* raw() const { return (const T*)this; }
  T* raw() { return (T*)this; }
  template<typename U>
  U to() const { return {x, y}; }

  friend Self operator+(const Self& a, const Self& b) {
    return {a.x + b.x, a.y + b.y};
  }

  friend Self operator-(const Self& a, const Self& b) {
    return {a.x - b.x, a.y - b.y};
  }

  friend Self operator*(const Self& point, T factor) {
    return {point.x * factor, point.y * factor};
  }

  friend Self operator/(const Self& point, T factor) {
    return {point.x / factor, point.y / factor};
  }

  Self dot(const Self& other) const {
    return {x * other.x, y * other.y};
  }

  T x;
  T y;
};

using Point2F = Point2<float>;
using Point2I = Point2<int>;
