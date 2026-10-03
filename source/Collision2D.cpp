#include "Collision2D.hpp"

namespace {

// intervalos fechados [a_lo, a_hi] e [b_lo, b_hi] se tocam ou cruzam
bool ranges_overlap(float a_lo, float a_hi, float b_lo, float b_hi) noexcept {
    return a_lo <= b_hi && b_lo <= a_hi;
}

}  // namespace

bool AABB::intersects(const AABB& other) const noexcept {
    // caixas colidem quando há sobreposição nos dois eixos
    return ranges_overlap(min.x, max.x, other.min.x, other.max.x)
        && ranges_overlap(min.y, max.y, other.min.y, other.max.y);
}

AABB Collision2D::bounds(const Vector2D& position) const noexcept {
    // a posição é o centro da caixa
    return {position - halfExtents, position + halfExtents};
}
