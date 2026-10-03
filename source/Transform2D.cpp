#include "Transform2D.hpp"

#include <cmath>

namespace {

// monta uma afim: parte linear [a b; c d] e translação na terceira linha
Transform2D make_affine(float a, float b, float c, float d, float tx, float ty) noexcept {
    Transform2D affine;
    affine.m[0][0] = a;   affine.m[0][1] = b;
    affine.m[1][0] = c;   affine.m[1][1] = d;
    affine.m[2][0] = tx;  affine.m[2][1] = ty;
    return affine;
}

}  // namespace

Transform2D Transform2D::translation(float tx, float ty) noexcept {
    return make_affine(1.0f, 0.0f, 0.0f, 1.0f, tx, ty);
}

Transform2D Transform2D::rotation(float angle_rad) noexcept {
    const float cos_a = std::cos(angle_rad);
    const float sin_a = std::sin(angle_rad);
    return make_affine(cos_a, sin_a, -sin_a, cos_a, 0.0f, 0.0f);
}

Transform2D Transform2D::scale(float sx, float sy) noexcept {
    return make_affine(sx, 0.0f, 0.0f, sy, 0.0f, 0.0f);
}

Transform2D Transform2D::operator*(const Transform2D& rhs) const noexcept {
    // linha de this vezes coluna de rhs; com vetores-linha this vem primeiro
    Transform2D product;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            product.m[row][col] = m[row][0] * rhs.m[0][col]
                                + m[row][1] * rhs.m[1][col]
                                + m[row][2] * rhs.m[2][col];
        }
    }
    return product;
}

Transform2D& Transform2D::operator*=(const Transform2D& rhs) noexcept {
    return *this = *this * rhs;
}

Vector2D Transform2D::transform_vector(const Vector2D& direction) const noexcept {
    // w = 0: só as duas primeiras linhas entram
    return {
        direction.x * m[0][0] + direction.y * m[1][0],
        direction.x * m[0][1] + direction.y * m[1][1]
    };
}

Vector2D Transform2D::transform_point(const Vector2D& point) const noexcept {
    // w = 1: mesma conta da direção mais a translação
    const Vector2D offset{m[2][0], m[2][1]};
    return transform_vector(point) + offset;
}
