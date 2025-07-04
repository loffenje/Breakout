#pragma once

#include <raymath.h>
#include "gamelib.h"

namespace breakout {

class Map;

enum class Collision {
    None,
    Top,
    Bottom,
    Left,
    Right,
};

class CollisionManager {
    struct Collidable {
        GameObject *    go;
        Rectangle       bounds;
    };

    struct Hit {
        GameObject *ball;
        GameObject *block;
        CollisionManifold manifold;
    };

public:
    void AddBall(GameObject *go);
    void AddBlock(GameObject *go, Rectangle bounds);
    void RemoveBlock(GameObject *go);
    void Tick();
    void DebugDraw();
private:
    std::vector<Collidable> m_balls;
    std::vector<Collidable> m_blocks;
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
    Vector2 GetCenter() const { return { m_position.x + (m_size.x * 0.5f), m_position.y + (m_size.y * 0.5f) }; }

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

    BallComponent(f32 x, f32 y, f32 w, f32 h, f32 r);

    void Launch();
    void Tick(f32 dt) override;
    void OnCollision(const CollisionManifold &manifold, GameObject *collidedObject) override;
    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }
    f32 GetRadius() const { return m_radius; }
    Vector2 GetCenter() const { return { m_position.x + m_radius, m_position.y + m_radius }; }
    bool IsLaunched() const { return m_state == State::Launched; }
private:
    State   m_state;
    Vector2 m_position;
    Vector2 m_size;
    Vector2 m_velocity;
    f32     m_radius;
};

class BlockComponent : public Component {
public:
    COMPONENT_NAME(BlockComponent)

    BlockComponent(f32 x, f32 y, f32 width, f32 height);
    void Init() override;
    void Tick(f32) override;
    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }
    Vector2 GetCenter() const { return { m_position.x + (m_size.x * 0.5f), m_position.y + (m_size.y * 0.5f) }; }
    void OnCollision(const CollisionManifold &manifold, GameObject *go);
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

BallComponent::BallComponent(f32 x, f32 y, f32 w, f32 h, f32 r) {
    m_position = { x, y };
    m_size = { w, h };
    m_velocity = { x + 100.0f, -y };
    m_radius = r;
    m_state = State::Attached;
}

void BallComponent::Launch() {
    if (m_state == State::Attached) {
        m_state = State::Launched;
        g_state.collisionMgr.AddBall(m_go);
    }
}

void BallComponent::Tick(f32 dt) {
    PlayerComponent *playerComp = g_state.player->GetComponent<PlayerComponent>();
    Vector2 playerPosition = { 0, 0 };
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
        else if (m_position.x + m_size.x >= g_state.worldDim.width) {
            m_velocity.x = -m_velocity.x;
            m_position.x = g_state.worldDim.width - m_size.x;
        }

        if (m_position.y <= g_state.worldDim.y) {
            m_velocity.y = -m_velocity.y;
            m_position.y = g_state.worldDim.y + m_size.y;
        }
    }

    DrawItem drawItem = {};
    drawItem.position = m_position;
    drawItem.texture = g_state.ballTexture;
    drawItem.src = Rectangle{ 0, 0, (f32)g_state.ballTexture.width, (f32)g_state.ballTexture.height };
    drawItem.size = m_size;
    drawItem.z_index = 0;
    DrawManager::Instance().Add(drawItem);
}

void BallComponent::OnCollision(const CollisionManifold &manifold, GameObject *collidedObject) {
    Vector2 norm = Vector2Normalize(manifold.diff);
   
    Collision collision = Collision::None;
    if (Vector2DotProduct(norm, Vector2{ 0.0f, 1.0f }) > 0.0f) {
        collision = Collision::Top;
    }
    else if (Vector2DotProduct(norm, Vector2{ 0.0f, -1.0f }) > 0.0f) {
        collision = Collision::Bottom;
    }
    else if (Vector2DotProduct(norm, Vector2{ 1.0f, 0.0f }) > 0.0f) {
        collision = Collision::Right;
    }
    else if (Vector2DotProduct(norm, Vector2{ -1.0f, 0.0f }) > 0.0f) {
        collision = Collision::Left;
    }

    switch (collision) {
    case Collision::Top:
        m_position.y -= manifold.penetration;
        m_velocity.y = -m_velocity.y;
        break;
    case Collision::Bottom:
        m_position.y += manifold.penetration;
        m_velocity.y = -m_velocity.y;
        break;
    case Collision::Left:
        m_position.x -= manifold.penetration;
        m_velocity.x -= m_velocity.x;
        break;
    case Collision::Right:
        m_position.x += manifold.penetration;
        m_velocity.x -= m_velocity.x;
        break;
    case Collision::None:
    default:
        break;
    }

}

BlockComponent::BlockComponent(f32 x, f32 y, f32 width, f32 height) {
    m_position = { x, y };
    m_size = { width, height };
}

void BlockComponent::Init() {
    Vector2 center = GetCenter();
    Vector2 halfSize = { m_size.x * 0.5f, m_size.y * 0.5f };
    g_state.collisionMgr.AddBlock(m_go, Rectangle{ center.x, center.y, halfSize.x, halfSize.y });
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

void BlockComponent::OnCollision(const CollisionManifold &manifold, GameObject *go) {
    g_state.collisionMgr.RemoveBlock(m_go);
}

void CollisionManager::AddBlock(GameObject *go, Rectangle bounds) {
    m_blocks.emplace_back(Collidable{go, bounds});
}

void CollisionManager::AddBall(GameObject *go) {
    m_balls.push_back(Collidable{ go, Rectangle{} });
}

void CollisionManager::RemoveBlock(GameObject *go) {
    if (m_blocks.empty()) {
        return;
    }

    // don't care about sorting, should be faster than erase/remove
    int destroyIdx = 0;
    for (int i = 0; i < m_blocks.size(); ++i) {
        if (go->GetId() == m_blocks[i].go->GetId()) {
            destroyIdx = i;
            break;
        }
    }

    RemoveByIndex(m_blocks, destroyIdx);
}

void CollisionManager::Tick() {
// NOTE: reallisticly there is always one ball. But if I decide to add some powerup that adds multiple balls, then this setup already works.
// TODO: consider to have quad tree. For now this is fine since the number of game objects is small.

    //1st test - dynamic bounds vs dynamic bounds
    PlayerComponent *playerComp = g_state.player->GetComponent<PlayerComponent>();

    if (playerComp) {
        Vector2 pos = playerComp->GetPosition();
        Vector2 size = playerComp->GetSize();
        Vector2 halfSize = { size.x * 0.5f, size.y * 0.5f };
        AABB aabbPlayer = { playerComp->GetCenter(), halfSize };

        Rectangle bounds = { pos.x, pos.y, size.x, size.y };
        for (auto &ball : m_balls) {
            BallComponent *ballComp = ball.go->GetComponent<BallComponent>();
            Vector2 center = ballComp->GetCenter();
            f32 radius = ballComp->GetRadius();
            ball.bounds = Rectangle{ center.x, center.y, radius, radius };

            Circle circle = { center, radius };
            CollisionManifold manifold = AABBvsCircle(aabbPlayer, circle);
            if (manifold.collides) {
                ballComp->OnCollision(manifold, g_state.player);
            }
        }
    }

    std::vector<Hit> hitInfo;
    //2nd test - dynamic bounds vs static bounds
    for (auto &ball : m_balls) {
        BallComponent *ballComp = ball.go->GetComponent<BallComponent>();
        Vector2 center = { ball.bounds.x, ball.bounds.y };
        f32 radius = ball.bounds.width;
        Circle circle = { center, radius };
        for (auto block : m_blocks) {
            AABB aabb = {};
            aabb.center = { block.bounds.x, block.bounds.y };
            aabb.halfExtents = { block.bounds.width, block.bounds.height };
            CollisionManifold manifold = AABBvsCircle(aabb, circle);

            if (manifold.collides) {
                ballComp->OnCollision(manifold, block.go);
                hitInfo.emplace_back(Hit{ball.go, block.go, manifold});
            }
        }
    }

    // post processing
    for (auto &hit : hitInfo) {
        BlockComponent *blockComp = hit.block->GetComponent<BlockComponent>();
        if (blockComp) {
            blockComp->OnCollision(hit.manifold, hit.ball);
            g_state.goMgr.Destroy(hit.block);
        }
    }
}

void CollisionManager::DebugDraw() {
    PlayerComponent *playerComp = g_state.player->GetComponent<PlayerComponent>();
    Vector2 pos = playerComp->GetPosition();
    Vector2 size = playerComp->GetSize();
    Rectangle bounds = { pos.x, pos.y, size.x, size.y };
    
    DrawRectangleLinesEx(bounds, 2.0f, RED);

    for (const auto &ball : m_balls) {
        DrawCircleLinesV(Vector2{ ball.bounds.x, ball.bounds.y }, ball.bounds.width, RED);
    }

    for (const auto &block : m_blocks) {
        auto bounds = ScaleAABB(block.bounds);
        DrawRectangleLinesEx(bounds, 2.0f, RED);
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
    const int padding = 5.0f;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int index = (y * m_width) + x;
            bool isTileEmpty = data[index] == 0;
            if (!isTileEmpty) {
                GameObject *tile = g_state.goMgr.Create();
                tile->AddComponent<BlockComponent>(xoffset, yoffset, m_tileSize.x, m_tileSize.y);
           }
           xoffset += m_tileSize.x + padding;
        }
        yoffset -= m_tileSize.y + padding;
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
    g_state.ball->AddComponent<BallComponent>(0.0f, globals::appSettings.screenHeight - 100.0f, 64.0f, 64.0f, 32.0f);

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

#ifdef DEVELOPER
    g_state.collisionMgr.DebugDraw();
#endif
    EndMode2D();

}

}