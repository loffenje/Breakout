#pragma once

#include "common.h"
#include "memory_arena.h"
#include <iterator>
#include <vector>
#include <unordered_map>

// This file contains common utils, helpers and core data structures for the actual gameplay code

namespace breakout {

class GameObject;

template <typename T>
void RemoveByIndex(std::vector<T> &data, int index) {
    data[index] = std::move(data.back());
    data.pop_back();
}

using ResHandle = u32;

enum ResType : int {
    RES_INVALID = 0,
    RES_SOUND,
    RES_FONT,
    RES_TEXTURE
};

inline
ResType ResGetType(ResHandle handle) {
    ResType result = static_cast<ResType>((handle >> 24) & 0xff);
    return result;
}

inline
u32 ResGetIndex(ResHandle handle) {
    u32 index = handle & 0x00ffffff;
    return index;
}

inline
ResHandle ResCreateHandle(u32 index, ResType type) {
    ResHandle result = (static_cast<ResHandle>(type) << 24) | (index & 0x00ffffff);

    return result;
}

ResHandle INVALID_HANDLE = ResCreateHandle(0, RES_INVALID);
struct Resources {
    static constexpr int MAX_RESOURCES = 32;

    Buffer<Sound, MAX_RESOURCES>                    sounds;
    Buffer<Font, MAX_RESOURCES>                     fonts;
    Buffer<Texture2D, MAX_RESOURCES>                textures;
    std::unordered_map<std::string, ResHandle>      handles;

    ResHandle LoadTexture(const char *filename) {
        Texture2D texture = ::LoadTexture(filename);
        int index = textures.Add(texture);
        auto handle = ResCreateHandle(index, RES_TEXTURE);

        handles[filename] = handle;

        return handle;
    }

    ResHandle LoadSound(const char *filename) {
        Sound sound = ::LoadSound(filename);
        int index = sounds.Add(sound);
        auto handle = ResCreateHandle(index, RES_SOUND);

        handles[filename] = handle;

        return handle;
    }

    ResHandle LoadFont(const char *filename, int fontSize) {
        Font font = ::LoadFontEx(filename, fontSize, 0, 256);
        int index = fonts.Add(font);
        auto handle = ResCreateHandle(index, RES_FONT);

        handles[filename] = handle;

        return handle;
    }

    int Acquire(const std::string &name) {
        const auto handleIt = handles.find(name);
        if (handleIt != handles.end()) {
            return Acquire(handleIt->second);
        }

        return INVALID_HANDLE;
    }

    int Acquire(ResHandle handle) {
        assert(handle != INVALID_HANDLE);
        ResType type = ResGetType(handle);
        assert(type != RES_INVALID);

        int result = ResGetIndex(handle);
        assert(result >= 0 && result < MAX_RESOURCES);
        return result;
    }
};

struct CollisionManifold {
    Vector2         diff;
    f32             penetration = 0.0f;
    bool            collides = false;
};

struct AABB {
    Vector2 center;
    Vector2 halfExtents;
};

inline
Rectangle ScaleAABB(Rectangle aabb) {
    Rectangle result = {};
    result.x = aabb.x - aabb.width;
    result.y = aabb.y - aabb.height;
    result.width = aabb.width * 2;
    result.height = aabb.height * 2;

    return result;
}

struct Circle {
    Vector2 center;
    f32     radius;
};

CollisionManifold AABBvsCircle(AABB aabb, Circle circle) {
    CollisionManifold result = {};
    Vector2 diff = Vector2Subtract(circle.center, aabb.center);
    Vector2 clampedDiff = Vector2Clamp(diff, Vector2Negate(aabb.halfExtents), aabb.halfExtents);
    Vector2 closestPoint = aabb.center + clampedDiff;
    Vector2 distToClosest = closestPoint - circle.center;
    f32 diffToClosest = Vector2Length(distToClosest);

    result.diff = distToClosest;
    result.penetration = circle.radius - fabsf(diffToClosest);
    result.collides = diffToClosest <= circle.radius;

    return result;
}

enum class DrawItemType {
    Texture,
    Font
};

struct DrawItem {
    DrawItemType    type = DrawItemType::Texture;
    Vector2         position;
    Vector2         size;
    Texture2D       texture;
    Rectangle       src;
    Font            font;
    f32             spacing = 1.0f;
    std::string     text;
    Color           color = WHITE;
    int             z_index = 0;
};

struct RecordedDrawItems {
    std::vector<DrawItem>    textureItems;
    std::vector<DrawItem>    fontItems;
};

class DrawManager {
public:
    static DrawManager &Instance();

    void Add(const DrawItem &item);
    void Dispatch();
    void Flush();
    void Record(RecordedDrawItems &record);
    void Copy(const RecordedDrawItems &record);
    DrawManager(const DrawManager &other) = delete;
    DrawManager &operator=(const DrawManager &other) = delete;

private:
    DrawManager() = default;

    std::vector<DrawItem>    m_textureItems;
    std::vector<DrawItem>    m_fontItems;
};

class Component {
public:
    virtual void OnInit() {}
    virtual void OnDestroy() {}
    virtual void Tick(f32 dt) = 0;
    virtual void Clear() {}
    virtual const char *ComponentName() const = 0;
    virtual void OnCollision(const CollisionManifold &manifold, GameObject *collidedObject) {}
    virtual void SetOwner(GameObject *go) { m_go = go; }
    Vector2 GetPosition() const { return m_position; }
    Vector2 GetSize() const { return m_size; }
protected:
    GameObject *    m_go = nullptr;
    Vector2         m_position;
    Vector2         m_size;
};

#define COMPONENT_NAME(klass)                                                                                      \
const char *ComponentName() const override {                                                                       \
    return #klass;                                                                                                 \
}                                                                                                                  \
static const char *ClassName() {                                                                                   \
    return #klass;                                                                                                 \
}

class GameObject {
public:
    GameObject() = default;
    ~GameObject() = default;

    void Init();
    void Clear();
    void Destroy();

    GameObject(const GameObject &other) = delete;
    GameObject &operator=(const GameObject &other) = delete;

    u32 GetId() const { return m_id; }
    GameObject *GetNext() const { return m_next; }
    void SetId(u32 id) { m_id = id; }
    void SetNext(GameObject *next) { m_next = next; }
    void Tick(f32 dt);

    template <typename T>
    void AddComponent();

    template <typename T, typename... Args>
    void AddComponent(Args &&...args);

    template<typename T>
    T *GetComponent() const;
private:
    u32                             m_id = 0;
    u8 *                            m_memory = nullptr;
    GameObject *                    m_next = nullptr;
    MemoryArena                     m_arena;
    std::vector<Component *>        m_components;

};

class GameObjectManager {
public:
    GameObjectManager() = default;

    void    Init();
    void    Destroy();
    void    Tick(f32 dt);
    GameObject *Create();
    void    Destroy(GameObject *go);
    int GetIndex(GameObject *go) const;
    GameObjectManager(const GameObjectManager &other) = delete;
    GameObjectManager &operator=(const GameObjectManager &other) = delete;

    ~GameObjectManager();
private:
    u32                                m_genId = 0;
    u8 *                               m_memory = nullptr;
    GameObject *                       m_firstFree = nullptr;
    MemoryArena                        m_arena;
    std::vector<GameObject *>          m_gos;
};


struct View {
    f32 xpos = 0.0f;
    f32 ypos = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;

    static View Push(f32 xpos, f32 ypos, f32 w, f32 h);
    static View PushFrom(View parent, f32 xpos, f32 ypos, f32 w, f32 h);
    static View PushText(View parent, f32 xpos, f32 ypos);
    static View PushCentered(View parent, f32 w, f32 h);
    void AddPadding(f32 padding, f32 alignFactor);
};

inline
Rectangle UIBox2Rectangle(View box) {
    Rectangle rect{ box.xpos, box.ypos, box.width, box.height };

    return rect;
}

void GameObject::Init() {
    const size_t capacity = 1024;
    m_memory = (u8 *)malloc(capacity);
    m_arena.Init(capacity, m_memory);
    m_components.reserve(capacity);
}

void GameObject::Tick(f32 dt) {
    for (auto *comp : m_components) {
        comp->Tick(dt);
    }
}

template <typename T>
void GameObject::AddComponent() {
    Component *comp = m_arena.Push<T>();

    comp->OnInit();
    comp->SetOwner(this);

    m_components.push_back(comp);
}

template <typename T, typename... Args>
void GameObject::AddComponent(Args &&...args) {
    Component *comp = m_arena.Push<T>(std::forward<Args>(args)...);

    comp->SetOwner(this);
    comp->OnInit();

    m_components.push_back(comp);
}

template<typename T>
T *GameObject::GetComponent() const {
    for (const auto &comp : m_components) {
        //NOTE: interned strings would be faster here
        if (strcmp(comp->ComponentName(), T::ClassName()) == 0) {
            return static_cast<T *>(comp);
        }
    }

    return nullptr;
}

void GameObject::Destroy() {
    for (auto *comp : m_components) {
        comp->OnDestroy();
    }

    free(m_memory);

    Clear();
}

void GameObject::Clear() {
    m_id = 0;
    m_next = nullptr;
    m_arena.Clear();
    m_components.clear();
}

void GameObjectManager::Init() {
    const size_t capacity = 1024 * 1024;
    u8 *memory = (u8 *)malloc(capacity);
    m_arena.Init(capacity, memory);

    m_gos.reserve(capacity);
}

void GameObjectManager::Destroy() {
    for (auto *go : m_gos) {
        go->SetNext(m_firstFree);
        m_firstFree = go;
        go->Destroy();
    }

    m_genId = 0;
    m_firstFree = nullptr;
    m_gos.clear();
    m_arena.Clear();
}

GameObjectManager::~GameObjectManager() {
    free(m_memory);
}

GameObject *GameObjectManager::Create() {
    GameObject *go = m_firstFree;
    if (go) {
        m_firstFree = m_firstFree->GetNext();
        go->Clear();
    }
    else {
        go = m_arena.Push<GameObject>();
        go->Init();
    }

    assert(m_genId + 1 < std::numeric_limits<u32>::max());
    go->SetId(m_genId++);

    m_gos.push_back(go);

    return go;
}

void GameObjectManager::Tick(f32 dt) {
    for (auto *go : m_gos) {
        go->Tick(dt);
    }
}

int GameObjectManager::GetIndex(GameObject *go) const {
    int idx = 0;
    for (int i = 0; i < m_gos.size(); ++i) {
        if (go->GetId() == m_gos[i]->GetId()) {
            idx = i;
            break;
        }
    }

    return idx;
}

void GameObjectManager::Destroy(GameObject *go) {
    go->SetNext(m_firstFree);
    m_firstFree = go;

    u32 goId = go->GetId();

    int destroyIdx = GetIndex(go);

    RemoveByIndex(m_gos, destroyIdx);
}

DrawManager &DrawManager::Instance() {
    static DrawManager instance;

    return instance;
}

void DrawManager::Add(const DrawItem &item) {
    switch (item.type) {
    case DrawItemType::Texture:
        m_textureItems.push_back(item);
        break;
    case DrawItemType::Font:
        m_fontItems.push_back(item);
        break;
    }
}

void DrawManager::Dispatch() {
    std::sort(m_textureItems.begin(), m_textureItems.end(), [](auto &lhs, auto &rhs) {
        return lhs.z_index < rhs.z_index;
    });

    for (const auto &item : m_textureItems) {
        DrawTexturePro(item.texture,
            item.src,
            Rectangle{ item.position.x, item.position.y, item.size.x, item.size.y },
            Vector2{ 0, 0 },
            0.0f,
            WHITE);
    }

    for (const auto &item : m_fontItems) {
        DrawTextEx(item.font, item.text.c_str(), item.position, item.size.x, item.spacing, item.color);
    }
}

void DrawManager::Flush() {
    m_textureItems.clear();
    m_fontItems.clear();
}

void DrawManager::Record(RecordedDrawItems &record) {
    std::copy(m_textureItems.begin(), m_textureItems.end(), std::back_inserter(record.textureItems));
    std::copy(m_fontItems.begin(), m_fontItems.end(), std::back_inserter(record.fontItems));
}

void DrawManager::Copy(const RecordedDrawItems &record) {
    std::copy(record.textureItems.begin(), record.textureItems.end(), std::back_inserter(m_textureItems));
    std::copy(record.fontItems.begin(), record.fontItems.end(), std::back_inserter(m_fontItems));
}

View View::Push(f32 xpos, f32 ypos, f32 w, f32 h) {
    View box{ xpos, ypos, w, h };

    return box;
}

void View::AddPadding(f32 padding, f32 alignFactor) {
    xpos += padding;
    ypos += padding;
    width = width - padding - alignFactor;
    height = height - padding - alignFactor;
}

View View::PushFrom(View parent, f32 xpos, f32 ypos, f32 w, f32 h) {
    View box{
        parent.xpos + xpos,
        parent.ypos + ypos,
        w > 0 ? w : parent.width,
        h > 0 ? h : parent.height };

    box.width = std::min(box.width, parent.width);
    box.height = std::min(box.height, parent.height);

    return box;
}

View View::PushText(View parent, f32 xpos, f32 ypos) {
    View box{ parent.xpos + xpos, parent.ypos + ypos, 0, 0 };

    box.width = std::min(box.width, parent.width);
    box.height = std::min(box.height, parent.height);

    return box;
}

View View::PushCentered(View parent, f32 w, f32 h) {
    f32 xoffset = parent.width * 0.5f;
    f32 yoffset = parent.height * 0.5f;

    View box{
        parent.xpos + xoffset - w * 0.5f,
        parent.ypos + yoffset - h * 0.5f,
        w, h };

    return box;
}
}