#include "RigidBody2D.hpp"

#include <cassert>

void RigidBody2D::integrate(float dt) noexcept {
    assert(dt > 0.0f && "passo de tempo precisa ser positivo");

    // euler semi-implícito: a posição já usa a velocidade nova
    velocity += acceleration * dt;
    position += velocity * dt;
}
