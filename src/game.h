#pragma once

#include <raymath.h>
#include "gamelib.h"

namespace breakout {

class Map;

class CollisionManager {
public:
    void Add(Rectangle rect);
    void Tick();
private:
    std::vector<Rectangle> m_boundsList;
};

struct GameState {
    UIBox               uiRoot;
    Rectangle           worldDim;
    GameObjectManager   goMgr;
    CollisionManager    collisionMgr;
    Camera2D            camera;
    Map *               map;
    Texture2D           texture;
    Texture2D           ballTexture;
    GameObject *        player;
    GameObject *        ball;
};

static GameState g_state;

class Map {
public:
    Map(Vector2 origin, Vector2 tileSize, int width, int height);

    inline int GetWidth() const { return m_width; }
    inline int GetHeight() const { return m_height; }
    Rectangle GetBounds() const;
    Vector2 GetOrigin() const;
    Vector2 GetTileSize() const { return m_tileSize; }
    void Load(u8 *data);
private:
    int                     m_width = 0;
    int                     m_height = 0;
    Vector2                 m_tileSize;
    Vector2                 m_origin;
};

class PlayerComponent : public Component {
public:
    COMPONENT_NAME(PlayerComponent)

    static constexpr f32 SPEED = 480.0f;

    PlayerComponent(f32 x, f32 y, f32 w, f32 h);
    void Tick(f32 dt) override;
    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }

private:
    Vector2 m_position;
    Vector2 m_size;
};

class BallComponent : public Component {
    enum class State {
        Attached,
        Launched
    };

    static constexpr f32 SPEED = 200.0f;

public:
    COMPONENT_NAME(BallComponent)

    BallComponent(f32 x, f32 y, f32 r);

    void Launch();
    void Tick(f32 dt) override;
    Vector2 GetPosition() const { return m_position; }
    f32 GetRadius() const { return m_radius; }

private:
    State   m_state;
    Vector2 m_position;
    Vector2 m_velocity;
    f32     m_radius;
};

class BlockComponent : public Component {
public:
    COMPONENT_NAME(BlockComponent)

    BlockComponent(f32 x, f32 y, f32 width, f32 height);

    void Tick(f32) override;
    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }

private:
    Vector2 m_position;
    Vector2 m_size;
};


PlayerComponent::PlayerComponent(f32 x, f32 y, f32 w, f32 h) {
    m_position = { x, y };
    m_size = { w, h };
}

void PlayerComponent::Tick(f32 dt) {

    Vector2 newPosition = m_position;
    if (IsKeyDown(KEY_LEFT)) {
        newPosition.x -= SPEED * dt;
    }

    if (IsKeyDown(KEY_RIGHT)) {
        newPosition.x += SPEED * dt;
    }

    if (newPosition.x >= g_state.worldDim.x && newPosition.x + m_size.x <= g_state.worldDim.width) {
        m_position = newPosition;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        g_state.collisionMgr.Add(Rectangle{ m_position.x, m_position.y, m_size.x, m_size.y });
        
        BallComponent *ballComp = g_state.ball->GetComponent<BallComponent>();
        ballComp->Launch();
    }

    DrawItem drawItem = {};
    drawItem.position = m_position;
    drawItem.texture = g_state.texture;
    drawItem.src = Rectangle{ 96,64, 16, 16 };
    drawItem.size = m_size;
    drawItem.z_index = 0;
    DrawManager::Instance().Add(drawItem);
}

BallComponent::BallComponent(f32 x, f32 y, f32 r) {
    m_position = { x, y };
    m_velocity = { x - 100.0f, -y };
    m_radius = r;
    m_state = State::Attached;
}

void BallComponent::Launch() {
    if (m_state == State::Attached) {
        m_state = State::Launched;
    }
}

void BallComponent::Tick(f32 dt) {
    PlayerComponent *playerComp = g_state.player->GetComponent<PlayerComponent>();
    Vector2 playerPosition = { 0, 0 };
    Vector2 size = { m_radius, m_radius };
    if (playerComp && m_state == State::Attached) {
        playerPosition = playerComp->GetPosition();
        m_position = { playerPosition.x + 32.0f, m_position.y };
    }
    else if (playerComp && m_state == State::Launched) {
        m_position += m_velocity * dt;

        if (m_position.x <= g_state.worldDim.x) {
            m_velocity.x = -m_velocity.x;
            m_position.x = g_state.worldDim.x;
        }
        else if (m_position.x + size.x <= g_state.worldDim.x) {
            m_velocity.x = -m_velocity.x;
            m_position.x = g_state.worldDim.width - m_position.x;
        }

        if (m_position.y <= g_state.worldDim.y) {
            m_velocity.y = -m_velocity.y;
            m_position.y = g_state.worldDim.y + size.y;
        }
    }

    DrawItem drawItem = {};
    drawItem.position = m_position;
    drawItem.texture = g_state.ballTexture;
    drawItem.src = Rectangle{ 0, 0, (f32)g_state.ballTexture.width, (f32)g_state.ballTexture.height };
    drawItem.size = size;
    drawItem.z_index = 0;
    DrawManager::Instance().Add(drawItem);
}

BlockComponent::BlockComponent(f32 x, f32 y, f32 width, f32 height) {
    m_position = { x, y };
    m_size = { width, height };

    g_state.collisionMgr.Add(Rectangle{ m_position.x, m_position.y, m_size.x, m_size.y });
}

void BlockComponent::Tick(f32) {
    DrawItem drawItem = {};
    drawItem.position = m_position;
    drawItem.texture = g_state.texture;
    drawItem.size = m_size;
    drawItem.src = Rectangle{ 0, 0, 25, 25 };
    drawItem.z_index = -999;
    DrawManager::Instance().Add(drawItem);
}

void CollisionManager::Add(Rectangle bounds) {
    m_boundsList.push_back(bounds);
}

void CollisionManager::Tick() {
    BallComponent *ballComp = g_state.ball->GetComponent<BallComponent>();
    if (ballComp) {
        for (auto bounds : m_boundsList) {
            if (CheckCollisionCircleRec(ballComp->GetPosition(), ballComp->GetRadius(), bounds)) {
                volatile int x = 5;
                (void)x;
                //
            }
        }
    }
}

Map::Map(Vector2 origin, Vector2 tileSize, int width, int height) :
    m_origin{ origin },
    m_tileSize{ tileSize },
    m_width{ width },
    m_height{ height } {

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

void Map::Load(u8 *data) {
    f32 xoffset = m_origin.x;
    f32 yoffset = m_origin.y;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int index = (y * m_width) + x;
            bool isTileEmpty = data[index] == 0;
            if (!isTileEmpty) {
                GameObject *tile = g_state.goMgr.Create();
                tile->AddComponent<BlockComponent>(xoffset, yoffset, m_tileSize.x, m_tileSize.y);
           }
           xoffset += m_tileSize.x;
        }
        yoffset -= m_tileSize.y;
        xoffset = m_origin.x;
    }
}

void Initialize() {
    g_state.uiRoot = UIBox::Push(0, 0, globals::appSettings.screenWidth, globals::appSettings.screenHeight);
    g_state.worldDim.x = globals::appSettings.screenWidth * -0.5f;
    g_state.worldDim.y = globals::appSettings.screenHeight * -0.5f;
    g_state.worldDim.width = globals::appSettings.screenWidth * 0.5f;
    g_state.worldDim.height = globals::appSettings.screenHeight * 0.5f;
    g_state.goMgr.Init();

    g_state.player = g_state.goMgr.Create();
    g_state.player->AddComponent<PlayerComponent>(-16.0f, globals::appSettings.screenHeight - 40.0f, 128.0f, 32.0f);

    g_state.ball = g_state.goMgr.Create();
    g_state.ball->AddComponent<BallComponent>(0.0f, globals::appSettings.screenHeight - 100.0f, 64.0f);

    g_state.texture = LoadTexture("assets/tiles.png");
    g_state.ballTexture = LoadTexture("assets/doge.png");

    const int width = 9;
    const int height = 3;
    u8 tiles[width * height] = {
        1, 1, 1, 1, 1, 1, 0, 1, 1,
        1, 0, 0, 0, 1, 1, 0, 1, 1,
        0, 0, 1, 1, 0, 0, 1, 1, 1,
    };

    g_state.camera.offset = {globals::appSettings.screenWidth * 0.5f, globals::appSettings.screenHeight * 0.5f};
    g_state.camera.target = {0,0};
    g_state.camera.rotation = 0.0f;
    g_state.camera.zoom = 1.0f;

    Vector2 originMap = { -globals::appSettings.screenWidth * 0.3f, 500.0f };
    g_state.map = new Map(originMap, Vector2{ 128, 64 }, width, height);
    g_state.map->Load(tiles);
}

void Update(f32 dt) {

    g_state.goMgr.Tick(dt);
    g_state.collisionMgr.Tick();
}

void Draw() {
    BeginMode2D(g_state.camera);

    DrawManager::Instance().Dispatch();

    EndMode2D();

}

}