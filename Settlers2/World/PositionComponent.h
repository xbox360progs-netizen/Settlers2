#pragma once
#include "Component.h"

namespace World {

struct __declspec(align(16)) PositionComponent : public Component {
    float x, y, z, w;
    PositionComponent() : x(0), y(0), z(0), w(1) {}
    PositionComponent(float px, float py) : x(px), y(py), z(0), w(1) {}
};

}