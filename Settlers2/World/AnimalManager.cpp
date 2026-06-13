#include "stdafx.h"
#include "AnimalManager.h"
#include <cstdlib>

namespace World {

const float DIAG_SPEED = 0.3f;

static void RandomDiagDir(float& vx, float& vy)
{
    int dir = rand() % 4;
    switch (dir) {
        case 0: vx =  DIAG_SPEED; vy = -DIAG_SPEED; break; // NE
        case 1: vx =  DIAG_SPEED; vy =  DIAG_SPEED; break; // SE
        case 2: vx = -DIAG_SPEED; vy =  DIAG_SPEED; break; // SW
        case 3: vx = -DIAG_SPEED; vy = -DIAG_SPEED; break; // NW
    }
}

AnimalManager::AnimalManager() {
}

void AnimalManager::Spawn(AnimalType type, const Vector2i& pos) {
    Animal a;
    a.type = type;
    a.state = AnimalState_Alive;
    a.x = (float)(pos.x + (rand() % 7) - 3);
    a.y = (float)(pos.y + (rand() % 7) - 3);
    a.spawnerX = pos.x;
    a.spawnerY = pos.y;
    a.stopTimer = 0.0f;
    RandomDiagDir(a.vx, a.vy);
    m_animals.push_back(a);
}

void AnimalManager::AddExisting(const Animal& animal) {
    Animal a = animal;
    a.stopTimer = 0.0f;
    RandomDiagDir(a.vx, a.vy);
    m_animals.push_back(a);
}

int AnimalManager::FindAliveAnimal(int x, int y, int radius, AnimalType type) const {
    for (size_t i = 0; i < m_animals.size(); ++i) {
        const Animal& a = m_animals[i];
        if (a.state == AnimalState_Alive && a.type == type) {
            float dx = a.x - (float)x;
            float dy = a.y - (float)y;
            if (dx * dx + dy * dy <= (float)(radius * radius)) {
                return (int)i;
            }
        }
    }
    return -1;
}

bool AnimalManager::IsAlive(int index) const {
    return index >= 0 && index < (int)m_animals.size()
        && m_animals[index].state == AnimalState_Alive;
}

void AnimalManager::TrapAnimal(int index) {
    if (index >= 0 && index < (int)m_animals.size()) {
        m_animals[index].state = AnimalState_Trapped;
    }
}

void AnimalManager::RemoveAnimal(int index) {
    if (index >= 0 && index < (int)m_animals.size()) {
        m_animals.erase(m_animals.begin() + index);
    }
}

} // namespace World
