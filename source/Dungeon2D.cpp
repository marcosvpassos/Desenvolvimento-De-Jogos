#include "Dungeon2D.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <stdexcept>

namespace {

constexpr int MIN_ROOM_SIZE = 5;  // menor sala aceita (5 x 5)
constexpr int ROOM_MARGIN   = 1;  // parede entre a sala e a borda da região
constexpr int MIN_REGION    = MIN_ROOM_SIZE + 2 * ROOM_MARGIN;  // menor região onde cabe uma sala
constexpr int HALF_CORRIDOR = 1;  // corredor de 3 tiles: linha central e uma de cada lado

// retângulos são inclusivos: left == right já ocupa 1 tile
int spanX(const Rect& r) noexcept { return r.right - r.left + 1; }
int spanY(const Rect& r) noexcept { return r.bottom - r.top + 1; }

int centerX(const Rect& r) noexcept { return (r.left + r.right) / 2; }
int centerY(const Rect& r) noexcept { return (r.top + r.bottom) / 2; }

// inteiro em [lo, hi]; usa só o mt19937, que dá a mesma sequência em qualquer compilador
int randomInt(std::mt19937& rng, int lo, int hi) {
    assert(lo <= hi);
    const auto range = static_cast<std::uint32_t>(hi - lo) + 1u;
    return lo + static_cast<int>(rng() % range);
}

// sorteia tamanho e posição da sala dentro da região, deixando a margem de parede
void carveRoom(BSPNode& leaf, std::mt19937& rng) {
    const Rect& area = leaf.region;
    const int w = randomInt(rng, MIN_ROOM_SIZE, spanX(area) - 2 * ROOM_MARGIN);
    const int h = randomInt(rng, MIN_ROOM_SIZE, spanY(area) - 2 * ROOM_MARGIN);
    const int left = randomInt(rng, area.left + ROOM_MARGIN, area.right - ROOM_MARGIN - w + 1);
    const int top  = randomInt(rng, area.top + ROOM_MARGIN, area.bottom - ROOM_MARGIN - h + 1);
    leaf.room = {left, top, left + w - 1, top + h - 1};
}

// corta a região em duas; quando não dá mais, o nó vira folha com uma sala
void split(BSPNode& node, std::mt19937& rng) {
    const Rect& area = node.region;
    const bool canCutX = spanX(area) >= 2 * MIN_REGION;  // corte vertical: esquerda e direita
    const bool canCutY = spanY(area) >= 2 * MIN_REGION;  // corte horizontal: cima e baixo

    if (!canCutX && !canCutY) {
        carveRoom(node, rng);
        return;
    }

    // corta o lado mais comprido para não gerar regiões finas
    bool cutX = canCutX;
    if (canCutX && canCutY) {
        if (spanX(area) * 4 > spanY(area) * 5) {
            cutX = true;
        } else if (spanY(area) * 4 > spanX(area) * 5) {
            cutX = false;
        } else {
            cutX = randomInt(rng, 0, 1) == 0;
        }
    }

    node.left = std::make_unique<BSPNode>();
    node.right = std::make_unique<BSPNode>();
    node.left->region = area;
    node.right->region = area;

    // a linha do corte fica com o primeiro filho; os dois têm pelo menos MIN_REGION
    if (cutX) {
        const int cut = randomInt(rng, area.left + MIN_REGION - 1, area.right - MIN_REGION);
        node.left->region.right = cut;
        node.right->region.left = cut + 1;
    } else {
        const int cut = randomInt(rng, area.top + MIN_REGION - 1, area.bottom - MIN_REGION);
        node.left->region.bottom = cut;
        node.right->region.top = cut + 1;
    }

    split(*node.left, rng);
    split(*node.right, rng);
}

void collectLeaves(const BSPNode& node, std::vector<const BSPNode*>& leaves) {
    if (node.isLeaf()) {
        leaves.push_back(&node);
        return;
    }
    collectLeaves(*node.left, leaves);
    collectLeaves(*node.right, leaves);
}

// faixa de 3 tiles de altura na linha y, da coluna x0 até x1
Corridor horizontalCorridor(int x0, int x1, int y) noexcept {
    return {std::min(x0, x1) - HALF_CORRIDOR, y - HALF_CORRIDOR,
            std::max(x0, x1) + HALF_CORRIDOR, y + HALF_CORRIDOR};
}

// faixa de 3 tiles de largura na coluna x, da linha y0 até y1
Corridor verticalCorridor(int x, int y0, int y1) noexcept {
    return {x - HALF_CORRIDOR, std::min(y0, y1) - HALF_CORRIDOR,
            x + HALF_CORRIDOR, std::max(y0, y1) + HALF_CORRIDOR};
}

// liga as duas metades de cada nó interno, das folhas até a raiz
void connect(const BSPNode& node, std::vector<Corridor>& corridors, std::mt19937& rng) {
    if (node.isLeaf()) {
        return;
    }
    connect(*node.left, corridors, rng);
    connect(*node.right, corridors, rng);

    std::vector<const BSPNode*> leftLeaves;
    std::vector<const BSPNode*> rightLeaves;
    collectLeaves(*node.left, leftLeaves);
    collectLeaves(*node.right, rightLeaves);

    // escolhe o par de salas mais próximo entre as duas metades
    const Rect* from = nullptr;
    const Rect* to = nullptr;
    int bestDistance = INT_MAX;
    for (const BSPNode* a : leftLeaves) {
        for (const BSPNode* b : rightLeaves) {
            const int distance = std::abs(centerX(a->room) - centerX(b->room))
                               + std::abs(centerY(a->room) - centerY(b->room));
            if (distance < bestDistance) {
                bestDistance = distance;
                from = &a->room;
                to = &b->room;
            }
        }
    }

    const int ax = centerX(*from);
    const int ay = centerY(*from);
    const int bx = centerX(*to);
    const int by = centerY(*to);

    // corredor em l entre os centros; sorteia qual trecho vem primeiro
    if (randomInt(rng, 0, 1) == 0) {
        if (ax != bx) corridors.push_back(horizontalCorridor(ax, bx, ay));
        if (ay != by) corridors.push_back(verticalCorridor(bx, ay, by));
    } else {
        if (ay != by) corridors.push_back(verticalCorridor(ax, ay, by));
        if (ax != bx) corridors.push_back(horizontalCorridor(ax, bx, by));
    }
}

}  // namespace

Dungeon2D::Dungeon2D(int widthTiles, int heightTiles)
    : width(widthTiles), height(heightTiles) {
    assert(widthTiles > 0 && heightTiles > 0 && "dimensões precisam ser positivas");
}

void Dungeon2D::generate(unsigned seed) {
    // sala 5 x 5 mais uma parede de cada lado
    if (width < MIN_REGION || height < MIN_REGION) {
        throw std::invalid_argument("Dungeon2D::generate: mapa menor que 7 x 7 tiles");
    }

    std::mt19937 rng(seed);
    corridors.clear();  // permite gerar de novo no mesmo objeto
    root = std::make_unique<BSPNode>();
    root->region = {0, 0, width - 1, height - 1};

    split(*root, rng);
    connect(*root, corridors, rng);
}
