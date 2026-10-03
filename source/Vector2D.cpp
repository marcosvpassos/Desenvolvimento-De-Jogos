#include "Vector2D.hpp"

#include <cassert>
#include <cmath>

// os operadores compostos fazem a conta; os binários copiam e reaproveitam

Vector2D& Vector2D::operator+=(const Vector2D& rhs) noexcept {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

Vector2D& Vector2D::operator-=(const Vector2D& rhs) noexcept {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

Vector2D& Vector2D::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2D& Vector2D::operator/=(float scalar) {
    assert(std::fabs(scalar) > EPSILON && "divisão por escalar quase zero");

    // uma divisão e duas multiplicações em vez de duas divisões
    const float inverse = 1.0f / scalar;
    return *this *= inverse;
}

Vector2D Vector2D::operator+(const Vector2D& rhs) const noexcept {
    Vector2D sum = *this;
    return sum += rhs;
}

Vector2D Vector2D::operator-(const Vector2D& rhs) const noexcept {
    Vector2D difference = *this;
    return difference -= rhs;
}

Vector2D Vector2D::operator*(float scalar) const noexcept {
    Vector2D scaled = *this;
    return scaled *= scalar;
}

Vector2D Vector2D::operator/(float scalar) const {
    Vector2D scaled = *this;
    return scaled /= scalar;
}

Vector2D operator*(float scalar, const Vector2D& vec) noexcept {
    return vec * scalar;
}

void Vector2D::normalize() {
    const float len = length();
    assert(len > EPSILON && "vetor nulo não tem direção");
    *this /= len;
}

Vector2D Vector2D::normalized() const {
    // normaliza uma cópia para não mexer neste vetor
    Vector2D unit = *this;
    unit.normalize();
    return unit;
}

bool Vector2D::equals(const Vector2D& rhs, float tolerance) const noexcept {
    // com tolerância zero vira comparação exata
    const Vector2D delta = *this - rhs;
    return std::fabs(delta.x) <= tolerance && std::fabs(delta.y) <= tolerance;
}
