#pragma once
#include "ResourceNode.h"
#include "Flag.h"
#include <math.h>

namespace World {

    enum WorkerState {
        WorkerState_Idle,
        WorkerState_Working,
        WorkerState_ReturningHome,
        WorkerState_MovingToJob
    };

    class Worker {
    public:
        WorkerState state;
        Building* home;

        // Movement data (used during MovingToJob)
        float wx, wy;
        float wvx, wvy;
        int wdir;       // 0=SE, 1=SW
        float wspeed;

        Worker(Building* h, float sx, float sy)
            : state(WorkerState_MovingToJob)
            , home(h)
            , wx(sx), wy(sy)
            , wvx(0.0f), wvy(0.0f)
            , wdir(0)
            , wspeed(1.0f)
        {}

        virtual ~Worker() {}

        // Returns true while still moving, false when arrived
        bool Update(float dt) {
            if (state != WorkerState_MovingToJob) return false;
            if (!home || !home->connectedFlag) return false;

            float dx = (float)home->connectedFlag->pos.x - wx;
            float dy = (float)home->connectedFlag->pos.y - wy;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < 0.5f) {
                wx = (float)home->connectedFlag->pos.x;
                wy = (float)home->connectedFlag->pos.y;
                wvx = 0.0f; wvy = 0.0f;
                state = WorkerState_Idle;
                return false; // Arrived
            }
            wvx = dx / d * wspeed;
            wvy = dy / d * wspeed;
            wdir = (wvx >= 0.0f) ? 0 : 1;
            wx += wvx * dt;
            wy += wvy * dt;
            return true; // Still moving
        }
    };
}
