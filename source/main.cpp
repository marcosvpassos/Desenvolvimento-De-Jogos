#include <cmath>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "Collision2D.hpp"
#include "Global.hpp"
#include "RigidBody2D.hpp"
#include "Transform2D.hpp"
#include "Vector2D.hpp"

// demonstração e testes rápidos de cada módulo

namespace {

int failures = 0;  // verificações que falharam

// mostra o resultado de uma verificação
void check(bool ok, const char* name) {
    std::cout << (ok ? "  [ok]    " : "  [falha] ") << name << '\n';
    if (!ok) {
        ++failures;
    }
}

// imprime um vetor como (x, y)
std::ostream& operator<<(std::ostream& out, const Vector2D& v) {
    return out << '(' << v.x << ", " << v.y << ')';
}

void demo_global() {
    std::cout << "Global\n";

    // tile que contém uma posição do mundo
    const Vector2D pos{100.0f, 200.0f};
    const int col = static_cast<int>(std::floor(pos.x / TILE_SIZE));
    const int row = static_cast<int>(std::floor(pos.y / TILE_SIZE));

    std::cout << "  posição " << pos << " fica no tile (" << col << ", " << row << ")\n";
    check(col == 1 && row == 3, "tile calculado a partir da posição");
    check(PLAYER_ID < INITIAL_ENTITY_ID, "id do jogador fora da faixa gerada");
}

void demo_vector() {
    std::cout << "\nVector2D\n";

    const Vector2D a{3.0f, 4.0f};
    const Vector2D b{1.0f, 2.0f};
    std::cout << "  a = " << a << ", b = " << b << '\n';

    check((a + b).equals({4.0f, 6.0f}), "a + b");
    check((a - b).equals({2.0f, 2.0f}), "a - b");
    check((a * 2.0f).equals({6.0f, 8.0f}), "a * 2");
    check((2.0f * a).equals({6.0f, 8.0f}), "2 * a");
    check((a / 2.0f).equals({1.5f, 2.0f}), "a / 2");
    check(std::abs(a.length() - 5.0f) < EPSILON, "|a| = 5");
    check(std::abs(a.dot(b) - 11.0f) < EPSILON, "a . b = 11");
    check(std::abs(a.cross(b) - 2.0f) < EPSILON, "a x b = 2");
    check(std::abs(a.normalized().length() - 1.0f) < EPSILON, "normalized() tem comprimento 1");

    // ida e volta deve voltar ao valor original
    Vector2D c = a;
    c += b;
    c -= b;
    c *= 2.0f;
    c /= 2.0f;
    check(c.equals(a), "operadores compostos (+=, -=, *=, /=)");
}

void demo_transform() {
    std::cout << "\nTransform2D\n";

    const float pi = 3.14159265f;
    const Vector2D p{1.0f, 1.0f};
    const Transform2D move = Transform2D::translation(10.0f, 5.0f);

    check(move.transform_point(p).equals({11.0f, 6.0f}), "translação move pontos");
    check(move.transform_vector(p).equals(p), "translação não afeta direções");

    // o x sai quase zero, não zero, por erro de float
    const Vector2D r = Transform2D::rotation(pi / 2.0f).transform_vector({1.0f, 0.0f});
    std::cout << "  (1, 0) girado 90 graus = " << r << '\n';
    check(r.equals({0.0f, 1.0f}), "rotação de 90 graus");

    // escala primeiro, depois translada
    const Transform2D combo = Transform2D::scale(2.0f, 2.0f) * move;
    check(combo.transform_point(p).equals({12.0f, 7.0f}), "composição aplica da esquerda para a direita");
}

void demo_rigidbody() {
    std::cout << "\nRigidBody2D\n";

    RigidBody2D body;
    body.velocity = {10.0f, 0.0f};
    body.acceleration = {0.0f, 98.0f};
    body.integrate(0.5f);

    // velocidade vira (10, 49) e a posição anda com ela
    check(body.velocity.equals({10.0f, 49.0f}), "velocidade atualizada antes");
    check(body.position.equals({5.0f, 24.5f}), "posição usa a velocidade nova");

    // um segundo de queda com passo fixo de 60 hz
    RigidBody2D ball;
    ball.acceleration = {0.0f, 98.0f};
    const float dt = 1.0f / 60.0f;
    for (int step = 0; step < 60; ++step) {
        ball.integrate(dt);
    }

    std::cout << "  bola após 1 s: posição " << ball.position
              << ", velocidade " << ball.velocity << '\n';
    check(std::abs(ball.velocity.y - 98.0f) < 0.01f, "velocidade após 1 s = 98");
}

void demo_collision() {
    std::cout << "\nCollision2D\n";

    const Collision2D shape{};  // tile padrão 64 x 64
    const AABB a = shape.bounds({0.0f, 0.0f});

    check(a.min.equals({-32.0f, -32.0f}) && a.max.equals({32.0f, 32.0f}), "caixa centrada na posição");
    check(a.intersects(shape.bounds({32.0f, 32.0f})), "caixas sobrepostas colidem");
    check(a.intersects(shape.bounds({64.0f, 0.0f})), "bordas encostadas colidem");
    check(!a.intersects(shape.bounds({65.0f, 0.0f})), "caixas separadas não colidem");
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // acentos certos no console do windows
#endif

    demo_global();
    demo_vector();
    demo_transform();
    demo_rigidbody();
    demo_collision();

    if (failures == 0) {
        std::cout << "\ntudo certo!\n";
    } else {
        std::cout << '\n' << failures << " verificação(ões) falharam\n";
    }
    return failures == 0 ? 0 : 1;
}
