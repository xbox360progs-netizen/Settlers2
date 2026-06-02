#pragma once
#include "Unit.h"

namespace World {

    enum SoldierRank {
        Private,
        PrivateFirstClass,
        Sergeant,
        Officer,
        General
    };

    class Soldier : public Unit {
    public:
        SoldierRank rank;
        int attack;
        int defense;
        int health;

        Soldier(int x, int y, uint8_t o, SoldierRank r)
            : Unit(UnitType::Soldier, x, y, o), rank(r), 
              attack(1), defense(1), health(10) {
            // Set stats based on rank
        }

        void Update() override {
            // Logic
        }
    };
}
