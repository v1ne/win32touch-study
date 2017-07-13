// Copyright (c) v1ne

#pragma once

template<typename T>
struct Point2 {
// data members:
  T x;
  T y;
// end data members

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
  template<typename U> U* rawAs() const { static_assert(sizeof(U) == sizeof(T), "size mismatch"); return (U*)this; }
  template<typename U> U to() const { return {x, y}; }


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

  Self& operator+=(const Self& point) { x += point.x; y += point.y; return *this; }
  Self& operator-=(const Self& point) { x -= point.x; y -= point.y; return *this; }
  Self& operator*=(T factor) { x *= factor; y *= factor; return *this; }
  Self& operator/=(T factor) { x /= factor; y /= factor; return *this; }


  Self dot(const Self& other) const {
    return {x * other.x, y * other.y};
  }

  friend Self minByComponent(const Self& a, const Self& b) {
    return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y};
  }

  friend Self maxByComponent(const Self& a, const Self& b) {
    return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y};
  }
};

using Point2F = Point2<float>;
using Point2L = Point2<long>;
using Point2I = Point2<int>;

Point2F rotateRad(const Point2F p, const float radAngle);
static Point2F rotateDeg(const Point2F p, const float radAngle) {
  return rotateRad(p, radAngle * (3.14159f / 180.f));
}

template<typename T>
static Point2<T> Vec2Up(T length) { return {0.f, length}; }
template<typename T>
static Point2<T> Vec2Down(T length) { return {0.f, -length}; }
template<typename T>
static Point2<T> Vec2Right(T length) { return {length, 0.f}; }
template<typename T>
static Point2<T> Vec2Left(T length) { return {-length, 0.f}; }
