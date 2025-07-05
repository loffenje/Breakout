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
public:
    void AddBall(GameObject *go);
    void AddBlock(GameObject *go, Rectangle bounds);
    void RemoveBlock(GameObject *go);
    void Tick();
    void DebugDraw();
    void Clear();
private:
    std::vector<Collidable> m_balls;
    std::vector<Collidable> m_blocks;
};

struct HUD {
    View        container;
    View        text;
    int         fontId;

    void Init(View root, ResHandle fontHandle);
    void Draw();
};


enum class GameplayState {
    RunGame,
    RunMenu,
    PreGameWin,
    PreGameOver,
    GameOver,
    GameWin,
    Quit
};

struct Menu {
    enum : int {
        PLAY = 0,
        QUIT,
        MAX_SIZE
    };

    View                                view;
    View                                title;
    Buffer<View, MAX_SIZE>              stack;
    Buffer<const char *, MAX_SIZE>      texts;
    Buffer<int, MAX_SIZE>               options;
   
    int cursor = 0;
    int selectedOption = 0;

    void InitStartMenu(View parent) {

        stack.Clear();
        texts.Clear();
        options.Clear();

        view = View::PushCentered(parent, 200.0f, 200.0f);
        title = View::PushText(parent, 100.0f, 100.0f);

        const f32 offset = 5.0f;

        stack.Add(View::PushFrom(view, 0, offset, 0.0f, 100.0f));
        stack.Add(View::PushFrom(view, 0, offset + 100.0f, 0.0f, 100.0f));

        texts.Add("Play");
        texts.Add("Quit");

        options.Add(PLAY);
        options.Add(QUIT);
    }

    int GetOptionsLen() const {
        return options.len;
    }

    void Up() {
        assert(options.len == stack.len);
        
        cursor--;
        if (cursor < 0) {
            cursor = stack.len - 1;
        }

        selectedOption = options[cursor];
    }

    void Down() {
        assert(options.len == stack.len);
        
        cursor++;
        if (cursor > stack.len - 1) {
            cursor = 0;
        }
        selectedOption = options[cursor];
    }
};

static constexpr f32 GAME_RESET_DIFF = 2.0f;

struct GameState {
    GameplayState       gameplayState;
    Menu                menu;
    HUD                 hud;
    Rectangle           worldDim;
    GameObjectManager   goMgr;
    CollisionManager    collisionMgr;
    Camera2D            camera;
    Map *               map;
    GameObject *        player;
    GameObject *        ball;
    int                 hitScore;
    Resources           res;
    RecordedDrawItems   recordedDrawings;
    f32                 resetTimer;
};

static GameState g_gameState;

class Map {
public:
    Map(Vector2 origin, Vector2 tileSize, int width, int height);

    inline int GetWidth() const { return m_width; }
    inline int GetHeight() const { return m_height; }
    inline int GetBlocksNum() const { return m_blocksNum; }
    Rectangle GetBounds() const;
    Vector2 GetOrigin() const;
    Vector2 GetTileSize() const { return m_tileSize; }
    void Load(u8 *data);
private:
    int                     m_width = 0;
    int                     m_height = 0;
    int                     m_blocksNum = 0;
    Vector2                 m_tileSize;
    Vector2                 m_origin;
};


class PlayerComponent : public Component {
public:
    COMPONENT_NAME(PlayerComponent)

    static constexpr f32 SPEED = 480.0f;

    PlayerComponent(f32 x, f32 y, f32 w, f32 h);

    void Init() override;
    void Tick(f32 dt) override;
    Vector2 GetPosition() const { return m_position; }
    f32 GetVelocity() const { return m_velocity; }
    Vector2 GetSize() const { return m_size; }
    Vector2 GetCenter() const { return { m_position.x + (m_size.x * 0.5f), m_position.y + (m_size.y * 0.5f) }; }

private:
    Vector2     m_position;
    Vector2     m_size;
    f32         m_velocity;
    int         m_textureId = 0;
    Rectangle   m_textureSrc = {};
};

class BallComponent : public Component {
    enum class State {
        Attached,
        Launched
    };

    static constexpr Vector2 INIT_VELOCITY = { 100.0f, -660.0f };

public:
    COMPONENT_NAME(BallComponent)

    BallComponent(f32 x, f32 y, f32 w, f32 h, f32 r);

    void Init() override;
    void Launch();
    void Tick(f32 dt) override;
    void OnCollision(const CollisionManifold &manifold, GameObject *collidedObject) override;
    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }
    f32 GetRadius() const { return m_radius; }
    Vector2 GetCenter() const { return { m_position.x + m_radius, m_position.y + m_radius }; }
    bool IsLaunched() const { return m_state == State::Launched; }
private:
    void ResolvePlayerCollision(PlayerComponent *playerComp);
    void ResolveBlockCollision(const CollisionManifold &manifold);

    State   m_state;
    Vector2 m_position;
    Vector2 m_size;
    Vector2 m_velocity;
    f32     m_radius;
    int     m_textureId = 0;
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
    Vector2     m_position;
    Vector2     m_size;
    int         m_textureId = 0;
    Rectangle   m_textureSrc = {};
};


PlayerComponent::PlayerComponent(f32 x, f32 y, f32 w, f32 h) {
    m_position = { x, y };
    m_size = { w, h };
}


void PlayerComponent::Init() {
    m_textureId = g_gameState.res.Acquire("assets/tiles.png");
    m_textureSrc = Rectangle{ 96, 64, 16, 16 };
}

void PlayerComponent::Tick(f32 dt) {

    Vector2 newPosition = m_position;
    f32 velocity = m_velocity;

    f32 baseSpeed = SPEED;

    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        baseSpeed *= 2.0f;
    }

    if (IsKeyDown(KEY_LEFT)) {
        velocity = baseSpeed * dt;
        newPosition.x -= velocity;
    }

    if (IsKeyDown(KEY_RIGHT)) {
        velocity = baseSpeed * dt;
        newPosition.x += velocity;
    }

    if (newPosition.x >= g_gameState.worldDim.x && newPosition.x + m_size.x <= g_gameState.worldDim.width) {
        m_velocity = velocity;
        m_position = newPosition;
    }
    
    if (IsKeyPressed(KEY_SPACE)) {
        BallComponent *ballComp = g_gameState.ball->GetComponent<BallComponent>();
        ballComp->Launch();
    }

    DrawItem drawItem = {};
    drawItem.position = m_position;
    drawItem.texture = g_gameState.res.textures[m_textureId];
    drawItem.src = m_textureSrc;
    drawItem.size = m_size;
    drawItem.z_index = 0;
    DrawManager::Instance().Add(drawItem);
}

BallComponent::BallComponent(f32 x, f32 y, f32 w, f32 h, f32 r) {
    m_position = { x, y };
    m_size = { w, h };
    m_velocity = INIT_VELOCITY;
    m_radius = r;
    m_state = State::Attached;
}

void BallComponent::Launch() {
    if (m_state == State::Attached) {
        m_state = State::Launched;
        g_gameState.collisionMgr.AddBall(m_go);
    }
}

void BallComponent::Init() {
    m_textureId = g_gameState.res.Acquire("assets/doge.png");
}

void BallComponent::Tick(f32 dt) {
    PlayerComponent *playerComp = g_gameState.player->GetComponent<PlayerComponent>();
    Vector2 playerPosition = { 0, 0 };
    if (playerComp && m_state == State::Attached) {
        playerPosition = playerComp->GetPosition();
        m_position = { playerPosition.x + 32.0f, m_position.y };
    }
    else if (playerComp && m_state == State::Launched) {
        m_position += m_velocity * dt;

        if (m_position.x <= g_gameState.worldDim.x) {
            m_velocity.x = -m_velocity.x;
            m_position.x = g_gameState.worldDim.x;
        }
        else if (m_position.x + m_size.x >= g_gameState.worldDim.width) {
            m_velocity.x = -m_velocity.x;
            m_position.x = g_gameState.worldDim.width - m_size.x;
        }

        if (m_position.y <= g_gameState.worldDim.y) {
            m_velocity.y = -m_velocity.y;
            m_position.y = g_gameState.worldDim.y;
        }

        if (m_position.y > g_gameState.worldDim.height) {
            g_gameState.gameplayState = GameplayState::PreGameOver;
        }
    }

    Texture2D texture = g_gameState.res.textures[m_textureId];
    DrawItem drawItem = {};
    drawItem.position = m_position;
    drawItem.texture = g_gameState.res.textures[m_textureId];
    drawItem.src = Rectangle{ 0, 0, (f32)texture.width, (f32)texture.height };
    drawItem.size = m_size;
    drawItem.z_index = 0;
    DrawManager::Instance().Add(drawItem);
}

void BallComponent::ResolvePlayerCollision(PlayerComponent *playerComp) {
    Vector2 centerPlayer = playerComp->GetCenter();
    Vector2 centerBall = GetCenter();
    f32 diff = centerBall.x - centerPlayer.x;
    const f32 intensity = 2.5f;
    f32 ratio = diff / (playerComp->GetSize().x * 0.5f);
    Vector2 prevVelocity = m_velocity;
    m_velocity.x = INIT_VELOCITY.x * intensity * ratio;
    m_velocity.y = -1.0f * fabsf(m_velocity.y);
    m_velocity = Vector2Normalize(m_velocity) * Vector2Length(prevVelocity);
}

void BallComponent::ResolveBlockCollision(const CollisionManifold &manifold) {
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

void BallComponent::OnCollision(const CollisionManifold &manifold, GameObject *collidedObject) {
    auto *playerComp = collidedObject->GetComponent<PlayerComponent>();
    if (playerComp != nullptr) {
        ResolvePlayerCollision(playerComp);
        return;
    }

    ResolveBlockCollision(manifold);

    g_gameState.hitScore++;
}

BlockComponent::BlockComponent(f32 x, f32 y, f32 width, f32 height) {
    m_position = { x, y };
    m_size = { width, height };
}

void BlockComponent::Init() {
    Vector2 center = GetCenter();
    Vector2 halfSize = { m_size.x * 0.5f, m_size.y * 0.5f };
    m_textureId = g_gameState.res.Acquire("assets/tiles.png");
    m_textureSrc = Rectangle{ 0, 0, 25, 25 };

    g_gameState.collisionMgr.AddBlock(m_go, Rectangle{ center.x, center.y, halfSize.x, halfSize.y });
}

void BlockComponent::Tick(f32) {
    DrawItem drawItem = {};
    drawItem.position = m_position;
    drawItem.texture = g_gameState.res.textures[m_textureId];
    drawItem.size = m_size;
    drawItem.src = m_textureSrc;
    drawItem.z_index = -999;
    DrawManager::Instance().Add(drawItem);
}

void BlockComponent::OnCollision(const CollisionManifold &manifold, GameObject *go) {
    g_gameState.collisionMgr.RemoveBlock(m_go);
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
    PlayerComponent *playerComp = g_gameState.player->GetComponent<PlayerComponent>();

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
                ballComp->OnCollision(manifold, g_gameState.player);
            }
        }
    }

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
                BlockComponent *blockComp = block.go->GetComponent<BlockComponent>();
                ballComp->OnCollision(manifold, block.go);
                blockComp->OnCollision(manifold, ball.go);
                g_gameState.goMgr.Destroy(block.go);
                break;
            }
        }
    }
}

void CollisionManager::DebugDraw() {
    PlayerComponent *playerComp = g_gameState.player->GetComponent<PlayerComponent>();
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

void CollisionManager::Clear() {
    m_balls.clear();
    m_blocks.clear();
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
                GameObject *tile = g_gameState.goMgr.Create();
                tile->AddComponent<BlockComponent>(xoffset, yoffset, m_tileSize.x, m_tileSize.y);
                m_blocksNum++;
           }
           xoffset += m_tileSize.x + padding;
        }
        yoffset -= m_tileSize.y + padding;
        xoffset = m_origin.x;
    }
}

void HUD::Init(View parent, ResHandle fontHandle) {
    container = View::PushFrom(parent, 10.0f, 20.0f, 256.0f, 128.0f);
    text = View::PushText(container, 5.0f, 2.0f);
    fontId = g_gameState.res.Acquire(fontHandle);
}

void HUD::Draw() {
    Font font = g_gameState.res.fonts[fontId];
    DrawTextEx(font,
        TextFormat("Score: %d", g_gameState.hitScore),
        Vector2{ text.xpos, text.ypos }, font.baseSize,
        1.0f, WHITE);

#if DEVELOPER
    DrawRectangleLinesEx(Rectangle{ container.xpos, container.ypos, container.width, container.height }, 2.0f, RED);
#endif

}

static
void DestroyScene() {
    g_gameState.goMgr.Destroy();
    delete g_gameState.map;
    g_gameState.player = nullptr;
    g_gameState.ball = nullptr;
    g_gameState.hitScore = 0;
    g_gameState.recordedDrawings.textureItems.clear();
    g_gameState.recordedDrawings.fontItems.clear();
    g_gameState.resetTimer = 0;
    g_gameState.collisionMgr.Clear();
    g_gameState.gameplayState = GameplayState::RunMenu;
}

static
void InitScene() {

    g_gameState.player = g_gameState.goMgr.Create();
    g_gameState.player->AddComponent<PlayerComponent>(-16.0f, g_gameState.worldDim.height - 40.0f, 128.0f, 32.0f);

    g_gameState.ball = g_gameState.goMgr.Create();
    g_gameState.ball->AddComponent<BallComponent>(0.0f, g_gameState.worldDim.height - 110.0f, 64.0f, 64.0f, 32.0f);

    const int width = 9;
    const int height = 3;
    u8 tiles[width * height] = {
        0, 1, 1, 0, 0, 0, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 1, 0, 0, 1, 1, 0,
    };

    Vector2 originMap = { g_gameState.worldDim.x + 200.0f, g_gameState.worldDim.y + 400.0f };
    g_gameState.map = new Map(originMap, Vector2{ 128, 64 }, width, height);
    g_gameState.map->Load(tiles);
}

void Initialize() {
    View mainView = View::Push(0, 0, globals::appSettings.screenWidth, globals::appSettings.screenHeight);
    g_gameState.worldDim.x = globals::appSettings.screenWidth * -0.5f;
    g_gameState.worldDim.y = globals::appSettings.screenHeight * -0.5f;
    g_gameState.worldDim.width = globals::appSettings.screenWidth * 0.5f;
    g_gameState.worldDim.height = globals::appSettings.screenHeight * 0.5f;
    
    g_gameState.camera.offset = { globals::appSettings.screenWidth * 0.5f, globals::appSettings.screenHeight * 0.5f };
    g_gameState.camera.target = { 0,0 };
    g_gameState.camera.rotation = 0.0f;
    g_gameState.camera.zoom = 1.0f;

    g_gameState.res.LoadTexture("assets/menu_bg.png");
    g_gameState.res.LoadTexture("assets/bg.png");
    g_gameState.res.LoadTexture("assets/tiles.png");
    g_gameState.res.LoadTexture("assets/doge.png");
    
    auto fontHandle = g_gameState.res.LoadFont("assets/nicefont.ttf", 72); // probably not

    g_gameState.hud.Init(mainView, fontHandle);
    g_gameState.goMgr.Init();

    InitScene();

    g_gameState.gameplayState = GameplayState::RunMenu;
    g_gameState.menu.InitStartMenu(mainView);
}

static
void PostGameResultMessage(const std::string &text, Color color) {
    f32 currTime = GetTime();
    if (currTime - g_gameState.resetTimer > GAME_RESET_DIFF) {
        DestroyScene();
        InitScene();
    }

    DrawItem item;
    item.type = DrawItemType::Font;
    item.position = { g_gameState.worldDim.x + 200.0f, g_gameState.worldDim.y + 200.0f };
    item.font = g_gameState.res.fonts[g_gameState.res.Acquire("assets/nicefont.ttf")];
    item.spacing = 1.0f;
    item.size = Vector2{ 144.0f, 144.0f };
    item.text = text;

    item.color = color;

    DrawManager::Instance().Copy(g_gameState.recordedDrawings);
    DrawManager::Instance().Add(item);
}

static
void UpdateGame(f32 dt) {
    switch (g_gameState.gameplayState) {
    case GameplayState::RunGame:
        if (IsKeyPressed(KEY_ESCAPE)) {
            g_gameState.gameplayState = GameplayState::RunMenu;
        }

        g_gameState.goMgr.Tick(dt);
        g_gameState.collisionMgr.Tick();

        if (g_gameState.map->GetBlocksNum() == g_gameState.hitScore) {
            g_gameState.gameplayState = GameplayState::PreGameWin;
        }

        break;
    case GameplayState::GameOver: {

        f32 currTime = GetTime();
        if (currTime - g_gameState.resetTimer > GAME_RESET_DIFF) {
            DestroyScene();
            InitScene();
            break;
        }

        PostGameResultMessage(R"(
                Critical
                failure)", RED);

        break;
    }
    case GameplayState::GameWin: {
        f32 currTime = GetTime();
        if (currTime - g_gameState.resetTimer > GAME_RESET_DIFF) {
            DestroyScene();
            InitScene();
            break;
        }

        PostGameResultMessage(R"(
                Critical
                success)", WHITE);

        break;
    }
    case GameplayState::PreGameOver:
        DrawManager::Instance().Record(g_gameState.recordedDrawings);
        g_gameState.resetTimer = GetTime();
        g_gameState.gameplayState = GameplayState::GameOver;
        break;
    case GameplayState::PreGameWin:
        DrawManager::Instance().Record(g_gameState.recordedDrawings);
        g_gameState.resetTimer = GetTime();
        g_gameState.gameplayState = GameplayState::GameWin;
    }
}

static
void UpdateMenu() {
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        g_gameState.menu.Down();
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        g_gameState.menu.Up();
    }

    if (IsKeyPressed(KEY_ENTER)) {
        switch (g_gameState.menu.selectedOption) {
        case Menu::PLAY: 
            g_gameState.gameplayState = GameplayState::RunGame;
            break;
        case Menu::QUIT:
            g_gameState.gameplayState = GameplayState::Quit;
            break;
        }
    }
}

void Update(f32 dt, bool &exitRequested) {

    if (g_gameState.gameplayState == GameplayState::RunGame || 
        g_gameState.gameplayState == GameplayState::PreGameOver ||
        g_gameState.gameplayState == GameplayState::PreGameWin ||
        g_gameState.gameplayState == GameplayState::GameOver ||
        g_gameState.gameplayState == GameplayState::GameWin) {
        UpdateGame(dt);
    }
    else if (g_gameState.gameplayState == GameplayState::RunMenu) {
        UpdateMenu();
    }
    else {
        exitRequested = true;
    }
    
}

static
void DrawGame() {

    int background = g_gameState.res.Acquire("assets/bg.png");
    DrawTextureEx(g_gameState.res.textures[background], Vector2{ 0, 0 }, 0.0f, 1.0f, WHITE);

    BeginMode2D(g_gameState.camera);

    switch (g_gameState.gameplayState) {
    case GameplayState::PreGameOver:
    case GameplayState::PreGameWin:
        DrawManager::Instance().Dispatch();
        break;
    case GameplayState::RunGame:
    case GameplayState::GameOver:
    case GameplayState::GameWin:
        DrawManager::Instance().Dispatch();
        DrawManager::Instance().Flush();
        break;
    }

#if DEVELOPER
    g_gameState.collisionMgr.DebugDraw();
#endif

    EndMode2D();

    g_gameState.hud.Draw();

}

static
void DrawMenu() {
    int background = g_gameState.res.Acquire("assets/menu_bg.png");
    DrawTextureEx(g_gameState.res.textures[background], Vector2{ 0, 0 }, 0.0f, 1.0f, WHITE);

    int fontId = g_gameState.res.Acquire("assets/nicefont.ttf");
    Font font = g_gameState.res.fonts[fontId];

    
    DrawTextEx(font,
        "Breakout 0.1",
        Vector2{ g_gameState.menu.title.xpos, g_gameState.menu.title.ypos }, font.baseSize,
        1.0f, WHITE);

    for (int i = 0; i < g_gameState.menu.GetOptionsLen(); ++i) {
        Color highlightColor = WHITE;
        if (g_gameState.menu.selectedOption == g_gameState.menu.options[i]) {
            highlightColor = MAGENTA;
        }
        View view = g_gameState.menu.stack[i];
        DrawTextEx(font,
            g_gameState.menu.texts[i],
            Vector2{ view.xpos, view.ypos }, font.baseSize,
            1.0f, highlightColor);
    }

}

void Draw() {

    if (g_gameState.gameplayState == GameplayState::RunGame || 
        g_gameState.gameplayState == GameplayState::PreGameOver ||
        g_gameState.gameplayState == GameplayState::PreGameWin ||
        g_gameState.gameplayState == GameplayState::GameOver ||
        g_gameState.gameplayState == GameplayState::GameWin) {
         DrawGame();
    }
    else {
        DrawMenu();
    }
}

}