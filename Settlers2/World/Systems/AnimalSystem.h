#pragma once
#include "../Component.h"
#include "../Entity.h"
#include "../EntityManager.h"
#include "../AnimalTypes.h"
#include "../Animal.h"
#include "../../Core/Vector2i.h"
#include <vector>

namespace World {

class Map;

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
    float stopTimer;
    AnimalComponent()
        : type(AnimalType_Deer), state(AnimalState_Alive)
        , spawnerX(0), spawnerY(0), stopTimer(0) {}
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
    Entity CreateAnimal(AnimalType type, const Vector2i& pos);
    void RemoveAnimal(Entity entity);

    Entity FindAliveAnimal(int x, int y, int radius, AnimalType type) const;
    bool IsAlive(Entity entity) const;

    void GetAllAnimals(std::vector<Animal>& out) const;
    size_t GetCount() const;

private:
    void MoveAnimals(float dt);
    static void RandomDiagDir(float& vx, float& vy);

    EntityManager* m_entities;
    Map* m_map;
    float m_dirTimer;

    static const float DIAG_SPEED;
    static const float MOVE_RANGE;
    static const float DIR_CHANGE_INTERVAL;
    static const float STOP_DURATION;
};

}
