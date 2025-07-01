#pragma once

#include "gamelib.h"

namespace breakout {

struct GameState {
    UIBox               uiRoot;
    GameObjectManager   goMgr;
};

static GameState g_state;

class Map {
public:
    Map(Vector2 origin, Vector2 tileSize, int width, int height);
    ~Map();

    Map(const Map &other) = delete;
    Map &operator=(const Map &other) = delete;

    inline int GetWidth() const { return m_width; }
    inline int GetHeight() const { return m_height; }
    Rectangle GetBounds() const;
    Vector2 GetOrigin() const;
    Vector2 GetTileSize() const { return m_tileSize; }
    void Load(u8 *data);
    void Update();
    bool IsTileOccupied(int x, int y) const;
    bool IsPositionOccupied(f32 x, f32 y) const;
private:
    int                     m_width = 0;
    int                     m_height = 0;
    u8 *                    m_tiles = nullptr;
    Vector2                 m_tileSize;
    Vector2                 m_origin;
};

Map::Map(Vector2 origin, Vector2 tileSize, int width, int height) :
    m_origin{ origin },
    m_tileSize{ tileSize },
    m_width{ width },
    m_height{ height } {

}

Map::~Map() {
    delete[] m_tiles;
}

Rectangle Map::GetBounds() const {
    Rectangle bounds = {
        m_origin.x,
        m_origin.y,
        m_origin.x + (m_tileSize.x * m_width),
        m_origin.y + (m_tileSize.y * m_height)
    };

    return bounds;
}

Vector2 Map::GetOrigin() const {
    auto bounds = GetBounds();
    Vector2 origin = { bounds.x, bounds.y };

    return origin;
}

bool Map::IsTileOccupied(int x, int y) const {
    assert(x < m_width);
    assert(y < m_height);

    int index = (y * m_width) + x;
    bool result = m_tiles[index] == 0;

    return result;
}

bool Map::IsPositionOccupied(f32 x, f32 y) const {

    int norm_x = static_cast<int>(x / m_tileSize.x);
    int norm_y = static_cast<int>(y / m_tileSize.y);

    if (norm_x < 0 || norm_x >(m_width - 1)) {
        return false;
    }

    if (norm_y < 0 || norm_y >(m_height - 1)) {
        return false;
    }

    bool result = IsTileOccupied(norm_x, norm_y);

    return result;
}

void Map::Load(u8 *data) {
    const size_t mapSize = m_width * m_height;
    m_tiles = new u8[mapSize];
    memcpy(m_tiles, data, mapSize);
}

void Map::Update() {

    static constexpr int LOWEST_Z = -999;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Vector2 pos = {
                m_origin.x + x * m_tileSize.x,
                m_origin.y + y * m_tileSize.y
            };

            if (!IsTileOccupied(x, y)) {
            }
            else {
            }
        }

    }
}
class PlayerComponent : public Component {
public:
    COMPONENT_NAME(PlayerComponent)

    void Init() override {
    }

    void Tick() override {
    }
    void Clear() override {}

    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }

private:
    Vector2 m_position;
    Vector2 m_size;
};

class BallComponent : public Component {
public:
    COMPONENT_NAME(BallComponent)

    void Init() override {
    }

    void Tick() override {
    }
    void Clear() override {}

    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }

private:
    Vector2 m_position;
    Vector2 m_size;
};

class BlockComponent : public Component {
public:
    COMPONENT_NAME(BlockComponent)

    void Init() override {
    }

    void Tick() override {
    }
    void Clear() override {}

    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }

private:
    Vector2 m_position;
    Vector2 m_size;
};

void Initialize() {
    g_state.uiRoot = UIBox::Push(0, 0, globals::appSettings.screenWidth, globals::appSettings.screenHeight);

}

void Update(f32 dt) {}
void Draw() {}
}