#pragma once
#include "ResourceNode.h"
#include "Flag.h"
#include "Road.h"
#include "RoadManager.h"
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

        // Position (node coords)
        float wx, wy;
        int wdir;       // 0=SE, 1=SW
        float wspeed;

        // Route walking (used during MovingToJob)
        std::vector<Flag*> route;
        uint32_t routeIndex;
        float ep;
        float walkDir;

        Worker(Building* h, float sx, float sy)
            : state(WorkerState_MovingToJob)
            , home(h)
            , wx(sx), wy(sy)
            , wdir(0)
            , wspeed(2.5f)
            , routeIndex(0)
            , ep(0.0f)
            , walkDir(1.0f)
        {}

        virtual ~Worker() {}

        // Initialize the leg starting at routeIndex (fromFlag) toward routeIndex+1 (toFlag).
        // Sets walkDir and ep correctly regardless of road tile orientation.
        void InitLeg(RoadManager* roadManager) {
            if (routeIndex >= route.size() - 1) return;
            Flag* f = route[routeIndex];
            Flag* t = route[routeIndex + 1];
            Road* r = roadManager ? roadManager->GetRoadBetween(f, t) : NULL;
            if (r && r->tiles.size() >= 2) {
                float plen = (float)(r->tiles.size() - 1);
                if (f->pos.x == r->tiles[0].x && f->pos.y == r->tiles[0].y) {
                    walkDir = 1.0f;
                    ep = 0.0f;
                } else {
                    walkDir = -1.0f;
                    ep = plen;
                }
            }
        }

        // Returns true while still moving, false when arrived
        bool Update(float dt, RoadManager* roadManager) {
            if (state != WorkerState_MovingToJob) return false;
            if (!home || !home->connectedFlag) return false;

            // No road route — fall back to direct walk
            if (route.size() < 2) {
                float dx = (float)home->connectedFlag->pos.x - wx;
                float dy = (float)home->connectedFlag->pos.y - wy;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < 0.5f) {
                    wx = (float)home->connectedFlag->pos.x;
                    wy = (float)home->connectedFlag->pos.y;
                    state = WorkerState_Idle;
                    return false;
                }
                wdir = (dx >= 0.0f) ? 0 : 1;
                wx += (dx / d) * wspeed * dt;
                wy += (dy / d) * wspeed * dt;
                return true;
            }

            if (routeIndex >= route.size() - 1) {
                // At final flag
                wx = (float)home->connectedFlag->pos.x;
                wy = (float)home->connectedFlag->pos.y;
                state = WorkerState_Idle;
                return false;
            }

            Flag* fromFlag = route[routeIndex];
            Flag* toFlag = route[routeIndex + 1];
            Road* road = roadManager ? roadManager->GetRoadBetween(fromFlag, toFlag) : NULL;
            if (!road || road->tiles.size() < 2) {
                routeIndex++;
                InitLeg(roadManager);
                return true;
            }

            float pathLen = (float)(road->tiles.size() - 1);

            // Set direction based on road tile order
            if (fromFlag->pos.x == road->tiles[0].x && fromFlag->pos.y == road->tiles[0].y) {
                walkDir = 1.0f;
            } else {
                walkDir = -1.0f;
            }

            ep += walkDir * wspeed * dt;

            // Compute position for this frame
            int tileCount = (int)road->tiles.size();
            float pos = ep;
            if (pos < 0.0f) pos = 0.0f;
            if (pos > pathLen) pos = pathLen;
            int tileIdx = (int)pos;
            float frac = pos - (float)tileIdx;
            if (tileIdx >= tileCount - 1) { tileIdx = tileCount - 2; frac = 1.0f; }
            if (tileIdx < 0) { tileIdx = 0; frac = 0.0f; }
            const Vector2i& tileA = road->tiles[tileIdx];
            const Vector2i& tileB = road->tiles[tileIdx + 1];
            wx = (float)tileA.x + ((float)tileB.x - (float)tileA.x) * frac;
            wy = (float)tileA.y + ((float)tileB.y - (float)tileA.y) * frac;

            // Direction based on road tile delta
            int bdx = tileB.x - tileA.x;
            wdir = (bdx >= 0) ? 0 : 1;

            // Check arrival at next flag
            bool arrivedAtFlag = false;
            if (walkDir > 0.0f && ep >= pathLen) {
                arrivedAtFlag = true;
            } else if (walkDir < 0.0f && ep <= 0.0f) {
                arrivedAtFlag = true;
            }

            if (arrivedAtFlag) {
                routeIndex++;
                if (routeIndex < route.size() - 1) {
                    InitLeg(roadManager);
                }
            }

            return true;
        }
    };
}
