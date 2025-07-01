#pragma once

#include "common.h"
#include "memory_arena.h"
#include <vector>

// This file contains common utils, helpers and core data structures for the actual gameplay code

namespace breakout {

class GameObject;

using ResId = u64;

template <typename T>
void RemoveByIndex(std::vector<T> &data, int index) {
    data[index] = std::move(data.back());
    data.pop_back();
}

struct DrawItem {
    Vector2 position;
    Vector2 size;
    ResId   texture = 0;
    Color   color = WHITE;
    int     z_index = 0;
};

class DrawList {
public:
    static DrawList &Instance();

    void Add(const DrawItem &item);
    void Dispatch();

    DrawList(const DrawList &other) = delete;
    DrawList &operator=(const DrawList &other) = delete;

private:
    DrawList() = default;

    std::vector<DrawItem> m_items;
};

class Component {
public:
    virtual void Init() {}
    virtual void Destroy() {}
    virtual void Tick() = 0;
    virtual void SetOwner(GameObject *go) { go = m_go; }
    virtual void Clear() = 0;
    virtual const char *ComponentName() const = 0;
    Component *GetNext() const { return m_next; }
protected:
    Component *     m_next = nullptr;
    GameObject *    m_go = nullptr;
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
    ~GameObject();

    void Init();
    void Clear();

    GameObject(const GameObject &other) = delete;
    GameObject &operator=(const GameObject &other) = delete;

    u32 GetId() const { return m_id; }
    GameObject *GetNext() const { return m_next; }
    void SetId(u32 id) { m_id = id; }
    void SetNext(GameObject *next) { m_next = next; }
    void Tick();

    template <typename T>
    void AddComponent();

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
    void    Tick();
    GameObject *Create();
    void    Destroy(GameObject *go);

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


struct UIBox {
    f32 xpos = 0.0f;
    f32 ypos = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;

    static UIBox Push(f32 xpos, f32 ypos, f32 w, f32 h);
    static UIBox PushFrom(UIBox parent, f32 xpos, f32 ypos, f32 w, f32 h);
    static UIBox PushText(UIBox parent, f32 xpos, f32 ypos);
    static UIBox PushCentered(UIBox parent, f32 w, f32 h);
    void AddPadding(f32 padding, f32 alignFactor);
};

inline
Rectangle UIBox2Rectangle(UIBox box) {
    Rectangle rect{ box.xpos, box.ypos, box.width, box.height };

    return rect;
}

GameObject::~GameObject() {
    free(m_memory);
}

void GameObject::Init() {
    const size_t capacity = 1024;
    m_memory = (u8 *)malloc(capacity);
    m_arena.Init(capacity * 2, m_memory);
    m_components.reserve(capacity);
}

void GameObject::Tick() {
    for (auto *comp : m_components) {
        comp->Tick();
    }
}

template <typename T>
void GameObject::AddComponent() {
    Component *comp = m_arena.Push<T>();

    comp->Init();
    comp->SetOwner(this);

    m_components.push_back(comp);
}

template<typename T>
T *GameObject::GetComponent() const {
    for (const auto &comp : m_components) {
        if (strcmp(comp->ComponentName(), T::ClassName()) == 0) {
            return static_cast<T *>(comp);
        }
    }

    return nullptr;
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

void GameObjectManager::Tick() {
    for (auto *go : m_gos) {
        go->Tick();
    }
}

void GameObjectManager::Destroy(GameObject *go) {
    go->SetNext(m_firstFree);
    m_firstFree = go;

    u32 goId = go->GetId();

    int destroyIdx = 0;
    for (int i = 0; i < m_gos.size(); ++i) {
        if (go->GetId() == m_gos[i]->GetId()) {
            destroyIdx = i;
        }
    }

    RemoveByIndex(m_gos, destroyIdx);
}

DrawList &DrawList::Instance() {
    static DrawList instance;

    return instance;
}

void DrawList::Add(const DrawItem &item) {
    m_items.push_back(item);
}

void DrawList::Dispatch() {
    std::sort(m_items.begin(), m_items.end(), [](auto &lhs, auto &rhs) {
        return lhs.z_index < rhs.z_index;
    });

    for (const auto &item : m_items) {
       // DrawTexturePro()
    }

    m_items.clear();
}


UIBox UIBox::Push(f32 xpos, f32 ypos, f32 w, f32 h) {
    UIBox box{ xpos, ypos, w, h };

    return box;
}

void UIBox::AddPadding(f32 padding, f32 alignFactor) {
    xpos += padding;
    ypos += padding;
    width = width - padding - alignFactor;
    height = height - padding - alignFactor;
}

UIBox UIBox::PushFrom(UIBox parent, f32 xpos, f32 ypos, f32 w, f32 h) {
    UIBox box{
        parent.xpos + xpos,
        parent.ypos + ypos,
        w > 0 ? w : parent.width,
        h > 0 ? h : parent.height };

    box.width = std::min(box.width, parent.width);
    box.height = std::min(box.height, parent.height);

    return box;
}

UIBox UIBox::PushText(UIBox parent, f32 xpos, f32 ypos) {
    UIBox box{ parent.xpos + xpos, parent.ypos + ypos, 0, 0 };

    box.width = std::min(box.width, parent.width);
    box.height = std::min(box.height, parent.height);

    return box;
}

UIBox UIBox::PushCentered(UIBox parent, f32 w, f32 h) {
    f32 xoffset = parent.width * 0.5f;
    f32 yoffset = parent.height * 0.5f;

    UIBox box{
        parent.xpos + xoffset - w * 0.5f,
        parent.ypos + yoffset - h * 0.5f,
        w, h };

    return box;
}
}