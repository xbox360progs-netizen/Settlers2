#pragma once
#include <stdint.h>
#include <vector>
#include "../Component.h"
#include "../Entity.h"
#include "../EntityManager.h"
#include "../AnimalTypes.h"
#include "../Animal.h"
#include "../../Core/Vector2i.h"

namespace World {

class Map;

static const size_t MAX_WORLD_ANIMALS = 128;

struct __declspec(align(16)) PositionComponent : public Component {
    float x, y, z, w;
    PositionComponent() : x(0), y(0), z(0), w(1) {}
    PositionComponent(float px, float py) : x(px), y(py), z(0), w(1) {}
};

struct VelocityComponent : public Component {
    float vx, vy;
    VelocityComponent() : vx(0), vy(0) {}
};

struct AnimalComponent : public Component {
    AnimalType type;
    AnimalState state;
    int spawnerX;
    int spawnerY;
    uint32_t habitatId;
    float stopTimer;
    AnimalComponent()
        : type(AnimalType_Deer), state(AnimalState_Alive)
        , spawnerX(0), spawnerY(0), habitatId(0), stopTimer(0) {}
};

struct RenderComponent : public Component {
    int spriteIndex;
    RenderComponent() : spriteIndex(-1) {}
};

class AnimalSystem {
public:
    AnimalSystem(EntityManager* entityManager, Map* map);
    ~AnimalSystem();

    void Update(float dt);
    Entity CreateAnimal(AnimalType type, const Vector2i& pos, uint32_t habitatId);
    void RemoveAnimal(Entity entity);

    Entity FindAliveAnimal(int x, int y, int radius, AnimalType type) const;
    bool IsAlive(Entity entity) const;

    void GetAllAnimals(std::vector<Animal>& out) const;
    size_t GetActiveCount() const { return (size_t)m_activeCount; }
    Entity GetEntityByActiveIdx(int idx) const;
    bool RegisterEntity(Entity entity);

    void Clear();

private:
    void MoveAnimals(float dt);
    static void RandomDiagDir(float& vx, float& vy);

    EntityManager* m_entities;
    Map* m_map;
    float m_dirTimer;

    Entity m_activeAnimals[MAX_WORLD_ANIMALS];
    int m_activeCount;

    static const float DIAG_SPEED;
    static const float MOVE_RANGE;
    static const float DIR_CHANGE_INTERVAL;
    static const float STOP_DURATION;
};

}