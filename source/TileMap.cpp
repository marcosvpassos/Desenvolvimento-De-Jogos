#include "TileMap.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <random>

#include "Dungeon2D.hpp"

namespace {

// posição da célula no vetor linha a linha
std::size_t cellIndex(int col, int row, int width) noexcept {
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(width)
         + static_cast<std::size_t>(col);
}

// abre um retângulo inclusivo, ignorando o que passar da borda
void carve(std::vector<int>& cells, int width, int height, const Rect& r) {
    const int left   = std::max(r.left, 0);
    const int right  = std::min(r.right, width - 1);
    const int top    = std::max(r.top, 0);
    const int bottom = std::min(r.bottom, height - 1);

    for (int row = top; row <= bottom; ++row) {
        for (int col = left; col <= right; ++col) {
            cells[cellIndex(col, row, width)] = EMPTY_TILE;
        }
    }
}

// aceita só sinal opcional e até 9 dígitos, sem espaços
bool parseInt(const std::string& text, int& value) {
    const bool negative = !text.empty() && text[0] == '-';
    const std::size_t first = negative ? 1 : 0;
    const std::size_t digits = text.size() - first;
    if (digits == 0 || digits > 9) {
        return false;
    }

    int result = 0;
    for (std::size_t i = first; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
        result = result * 10 + (text[i] - '0');
    }
    value = negative ? -result : result;
    return true;
}

// quebra uma linha nas vírgulas; campo vazio (como vírgula no fim) é erro
bool parseRow(const std::string& line, std::vector<int>& row) {
    row.clear();
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = line.find(',', start);
        int value = 0;
        if (!parseInt(line.substr(start, comma - start), value)) {
            return false;
        }
        row.push_back(value);
        if (comma == std::string::npos) {
            return true;
        }
        start = comma + 1;
    }
}

// mistura o relógio porque o random_device do mingw antigo repete os valores
std::mt19937& sharedRng() {
    static std::mt19937 rng(
        std::random_device{}() ^
        static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    return rng;
}

}  // namespace

void TileMap::loadFromDungeon(const Dungeon2D& dungeon) {
    m_width = dungeon.width;
    m_height = dungeon.height;
    m_cells.assign(cellIndex(0, m_height, m_width), WALL_TILE);  // tudo parede

    // percorre a árvore com uma pilha; só as folhas têm sala
    std::vector<const BSPNode*> pending;
    if (dungeon.root) {
        pending.push_back(dungeon.root.get());
    }
    while (!pending.empty()) {
        const BSPNode* node = pending.back();
        pending.pop_back();

        if (node->isLeaf()) {
            carve(m_cells, m_width, m_height, node->room);
            continue;
        }
        if (node->left)  pending.push_back(node->left.get());
        if (node->right) pending.push_back(node->right.get());
    }

    for (const Corridor& corridor : dungeon.corridors) {
        carve(m_cells, m_width, m_height, corridor);
    }
}

bool TileMap::saveCSV(const std::string& path) const {
    if (m_cells.empty()) {
        return false;  // nada para salvar
    }

    // binário para gravar só '\n', igual em qualquer sistema
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    for (int row = 0; row < m_height; ++row) {
        for (int col = 0; col < m_width; ++col) {
            if (col > 0) {
                out << ',';  // vírgula só entre valores
            }
            out << m_cells[cellIndex(col, row, m_width)];
        }
        out << '\n';
    }

    out.close();
    return !out.fail();
}

bool TileMap::loadCSV(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    // lê tudo em variáveis locais; o mapa só muda se o arquivo inteiro for válido
    std::vector<int> cells;
    std::vector<int> row;
    int width = 0;
    int height = 0;
    std::string line;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();  // tolera fim de linha do windows
        }
        if (!parseRow(line, row)) {
            return false;
        }
        if (height == 0) {
            width = static_cast<int>(row.size());
        } else if (static_cast<int>(row.size()) != width) {
            return false;  // linhas com tamanhos diferentes
        }
        cells.insert(cells.end(), row.begin(), row.end());
        ++height;
    }

    if (in.bad() || height == 0) {
        return false;
    }

    m_width = width;
    m_height = height;
    m_cells = std::move(cells);
    return true;
}

int TileMap::width() const noexcept {
    return m_width;
}

int TileMap::height() const noexcept {
    return m_height;
}

int TileMap::at(int col, int row) const {
    assert(col >= 0 && col < m_width && row >= 0 && row < m_height && "célula fora do mapa");
    return m_cells[cellIndex(col, row, m_width)];
}

bool TileMap::isWall(int col, int row) const {
    return at(col, row) == WALL_TILE;
}

const int* TileMap::data() const noexcept {
    return m_cells.data();
}

void TileMap::toIndices(const Vector2D& world, int& col, int& row) const noexcept {
    col = static_cast<int>(std::floor(world.x / TILE_SIZE));
    row = static_cast<int>(std::floor(world.y / TILE_SIZE));
}

Vector2D TileMap::toWorld(int col, int row) const noexcept {
    // centro da célula: meio tile depois do canto
    return {(static_cast<float>(col) + 0.5f) * TILE_SIZE,
            (static_cast<float>(row) + 0.5f) * TILE_SIZE};
}

void TileMap::randomEmptyCell(int& col, int& row) const {
    const auto emptyCount = std::count(m_cells.begin(), m_cells.end(), EMPTY_TILE);
    assert(emptyCount > 0 && "nenhuma célula vazia no mapa");
    if (emptyCount == 0) {
        col = row = -1;  // sem assert (modo release), devolve índice inválido
        return;
    }

    // sorteia a posição entre as vazias e anda até ela; todas têm a mesma chance
    std::uniform_int_distribution<long long> pick(0, static_cast<long long>(emptyCount) - 1);
    long long target = pick(sharedRng());

    for (std::size_t i = 0; i < m_cells.size(); ++i) {
        if (m_cells[i] == EMPTY_TILE && target-- == 0) {
            col = static_cast<int>(i % static_cast<std::size_t>(m_width));
            row = static_cast<int>(i / static_cast<std::size_t>(m_width));
            return;
        }
    }
}
