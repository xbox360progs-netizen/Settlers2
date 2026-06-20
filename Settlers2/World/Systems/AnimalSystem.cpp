#include "stdafx.h"
#include "AnimalSystem.h"
#include "../Map.h"
#include <cstdlib>

namespace World {

const float AnimalSystem::DIAG_SPEED = 0.3f;
const float AnimalSystem::MOVE_RANGE = 6.0f;
const float AnimalSystem::DIR_CHANGE_INTERVAL = 1.2f;
const float AnimalSystem::STOP_DURATION = 0.1f;

AnimalSystem::AnimalSystem(EntityManager* entityManager, Map* map)
    : m_entities(entityManager)
    , m_map(map)
    , m_dirTimer(0.0f)
    , m_activeCount(0)
{
}

AnimalSystem::~AnimalSystem() {
}

Entity AnimalSystem::CreateAnimal(AnimalType type, const Vector2i& pos, uint32_t habitatId) {
    if (m_activeCount >= (int)MAX_WORLD_ANIMALS) return INVALID_ENTITY;

    Entity entity = m_entities->CreateEntity();

    int ox = pos.x + (rand() % 7) - 3;
    int oy = pos.y + (rand() % 7) - 3;

    PositionComponent& pc = m_entities->AddComponent<PositionComponent>(entity);
    pc.x = (float)ox;
    pc.y = (float)oy;

    VelocityComponent& vc = m_entities->AddComponent<VelocityComponent>(entity);
    RandomDiagDir(vc.vx, vc.vy);

    AnimalComponent& ac = m_entities->AddComponent<AnimalComponent>(entity);
    ac.type = type;
    ac.state = AnimalState_Alive;
    ac.spawnerX = pos.x;
    ac.spawnerY = pos.y;
    ac.habitatId = habitatId;
    ac.stopTimer = 0.0f;

    m_activeAnimals[m_activeCount++] = entity;

    return entity;
}

void AnimalSystem::RemoveAnimal(Entity entity) {
    int found = -1;
    for (int i = 0; i < m_activeCount; ++i) {
        if (m_activeAnimals[i] == entity) {
            found = i;
            break;
        }
    }

    if (found >= 0) {
        m_activeAnimals[found] = m_activeAnimals[m_activeCount - 1];
        m_activeCount--;
    }

    m_entities->DestroyEntity(entity);
}

Entity AnimalSystem::FindAliveAnimal(int x, int y, int radius, AnimalType type) const {
    float maxDistSq = (float)(radius * radius);

    for (int i = 0; i < m_activeCount; ++i) {
        Entity e = m_activeAnimals[i];
        const AnimalComponent* ac = m_entities->GetComponent<AnimalComponent>(e);
        if (!ac || ac->state != AnimalState_Alive || ac->type != type)
            continue;
        const PositionComponent* pc = m_entities->GetComponent<PositionComponent>(e);
        if (!pc) continue;

        float dx = pc->x - (float)x;
        float dy = pc->y - (float)y;
        if (dx * dx + dy * dy <= maxDistSq)
            return e;
    }
    return INVALID_ENTITY;
}

bool AnimalSystem::IsAlive(Entity entity) const {
    const AnimalComponent* ac = m_entities->GetComponent<AnimalComponent>(entity);
    return ac && ac->state == AnimalState_Alive;
}

void AnimalSystem::GetAllAnimals(std::vector<Animal>& out) const {
    out.clear();
    out.reserve(m_activeCount);

    for (int i = 0; i < m_activeCount; ++i) {
        Entity e = m_activeAnimals[i];
        const PositionComponent* pc = m_entities->GetComponent<PositionComponent>(e);
        const VelocityComponent* vc = m_entities->GetComponent<VelocityComponent>(e);
        const AnimalComponent* ac = m_entities->GetComponent<AnimalComponent>(e);
        if (!pc || !vc || !ac) continue;

        Animal a;
        a.x = pc->x;
        a.y = pc->y;
        a.vx = vc->vx;
        a.vy = vc->vy;
        a.type = ac->type;
        a.state = ac->state;
        a.spawnerX = ac->spawnerX;
        a.spawnerY = ac->spawnerY;
        a.stopTimer = ac->stopTimer;
        a.habitatId = ac->habitatId;
        out.push_back(a);
    }
}

Entity AnimalSystem::GetEntityByActiveIdx(int idx) const {
    if (idx < 0 || idx >= m_activeCount) return INVALID_ENTITY;
    return m_activeAnimals[idx];
}

bool AnimalSystem::RegisterEntity(Entity entity) {
    if (m_activeCount >= (int)MAX_WORLD_ANIMALS) return false;
    m_activeAnimals[m_activeCount++] = entity;
    return true;
}

void AnimalSystem::Clear() {
    m_activeCount = 0;
    m_dirTimer = 0.0f;
}

void AnimalSystem::RandomDiagDir(float& vx, float& vy) {
    int dir = rand() % 4;
    switch (dir) {
        case 0: vx =  DIAG_SPEED; vy = -DIAG_SPEED; break;
        case 1: vx =  DIAG_SPEED; vy =  DIAG_SPEED; break;
        case 2: vx = -DIAG_SPEED; vy =  DIAG_SPEED; break;
        case 3: vx = -DIAG_SPEED; vy = -DIAG_SPEED; break;
    }
}

void AnimalSystem::MoveAnimals(float dt) {
    m_dirTimer += dt;
    bool dirChangeTick = false;
    if (m_dirTimer >= DIR_CHANGE_INTERVAL) {
        m_dirTimer = 0.0f;
        dirChangeTick = true;
    }

    for (int i = 0; i < m_activeCount; ++i) {
        Entity e = m_activeAnimals[i];
        AnimalComponent* ac = m_entities->GetComponent<AnimalComponent>(e);
        if (!ac || ac->state != AnimalState_Alive) continue;

        PositionComponent* pc = m_entities->GetComponent<PositionComponent>(e);
        VelocityComponent* vc = m_entities->GetComponent<VelocityComponent>(e);
        if (!pc || !vc) continue;

        if (ac->stopTimer > 0.0f) {
            vc->vx = 0.0f; vc->vy = 0.0f;
            ac->stopTimer -= dt;
            if (ac->stopTimer <= 0.0f) {
                ac->stopTimer = 0.0f;
                RandomDiagDir(vc->vx, vc->vy);
            }
            continue;
        }

        if (dirChangeTick && (rand() % 3) == 0) {
            vc->vx = 0.0f; vc->vy = 0.0f;
            ac->stopTimer = STOP_DURATION;
            continue;
        }

        float nx = pc->x + vc->vx * dt;
        float ny = pc->y + vc->vy * dt;

        if (m_map) {
            int destX = (int)(nx + 0.5f);
            int destY = (int)(ny + 0.5f);
            BYTE weight = m_map->GetNodeWeight(destX, destY);
            if (weight == Weight_Deep || weight == Weight_Shallow) {
                vc->vx = -vc->vx;
                vc->vy = -vc->vy;
                continue;
            }
        }

        float dxs = nx - (float)ac->spawnerX;
        float dys = ny - (float)ac->spawnerY;
        if (dxs * dxs + dys * dys <= MOVE_RANGE * MOVE_RANGE) {
            pc->x = nx;
            pc->y = ny;
        } else {
            vc->vx = -vc->vx;
            vc->vy = -vc->vy;
        }
    }
}

void AnimalSystem::Update(float dt) {
    MoveAnimals(dt);
}

}