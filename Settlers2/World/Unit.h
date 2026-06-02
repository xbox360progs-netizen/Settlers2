#pragma once
#include "../../Core/Vector2i.h"

namespace World {

    enum UnitType {
        Worker,
        Soldier
    };

    class Unit {
    public:
        UnitType type;
        Vector2i pos;
        uint8_t owner;

        Unit(UnitType t, int x, int y, uint8_t o) 
            : type(t), pos({x, y}), owner(o) {}
        virtual ~Unit() {}
        virtual void Update() = 0;
    };
}
