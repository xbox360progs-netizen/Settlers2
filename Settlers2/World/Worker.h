#pragma once
#include "WorkerDefs.h"
#include "ResourceNode.h"
#include "Flag.h"
#include "Road.h"
#include "RoadManager.h"
#include <math.h>

namespace World {

    struct Worker {
        // Hot data — fits in 32 bytes (two per 64-byte Xenon cache line)
        float posX, posY;        // 0-7
        float ep;                // 8-11
        float walkDir;           // 12-15
        uint32_t homeBuildingIdx;// 16-19
        uint16_t routeIndex;     // 20-21
        uint16_t routeCount;     // 22-23
        uint16_t stateTimer;     // 24-25
        uint8_t state;           // 26
        uint8_t profession;      // 27
        uint8_t carriedResource; // 28
        uint8_t padding[3];      // 29-31 (explicit pad to 32 bytes)

        // InitLeg now takes the route array as parameter (cold data)
        inline void InitLeg(RoadManager* roadManager, Flag** route) {
            if (routeIndex >= routeCount - 1) return;
            Flag* f = route[routeIndex];
            Flag* t = route[routeIndex + 1];
            Road* r = roadManager ? roadManager->GetRoadBetween(f, t) : NULL;
            if (r && r->tileCount >= 2) {
                float plen = (float)(r->tileCount - 1);
                if (f->pos.x == r->tiles[0].x && f->pos.y == r->tiles[0].y) {
                    walkDir = 1.0f;
                    ep = 0.0f;
                } else {
                    walkDir = -1.0f;
                    ep = plen;
                }
            }
        }

        // Update takes route array as parameter (cold data from WorkerManager)
        inline bool Update(float dt, RoadManager* roadManager, Flag* destFlag, Flag** route) {
            if (state != WorkerState_MovingToJob) return false;
            if (!destFlag) return false;

            // No route at all → can't move, stay idle until route assigned
            if (routeCount == 0) return false;

            if (routeCount < 2) {
                float dx = (float)destFlag->pos.x - posX;
                float dy = (float)destFlag->pos.y - posY;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < 0.5f) {
                    posX = (float)destFlag->pos.x;
                    posY = (float)destFlag->pos.y;
                    state = WorkerState_Idle;
                    return false;
                }
                posX += (dx / d) * 2.5f * dt;
                posY += (dy / d) * 2.5f * dt;
                return true;
            }

            if (routeIndex >= routeCount - 1) {
                posX = (float)destFlag->pos.x;
                posY = (float)destFlag->pos.y;
                state = WorkerState_Idle;
                return false;
            }

            Flag* fromFlag = route[routeIndex];
            Flag* toFlag = route[routeIndex + 1];
            Road* road = roadManager ? roadManager->GetRoadBetween(fromFlag, toFlag) : NULL;
            if (!road || road->tileCount < 2) {
                routeIndex++;
                InitLeg(roadManager, route);
                return true;
            }

            float pathLen = (float)(road->tileCount - 1);

            if (fromFlag->pos.x == road->tiles[0].x && fromFlag->pos.y == road->tiles[0].y) {
                walkDir = 1.0f;
            } else {
                walkDir = -1.0f;
            }

            ep += walkDir * 2.5f * dt;

            int tc = (int)road->tileCount;
            float pos = ep;
            if (pos < 0.0f) pos = 0.0f;
            if (pos > pathLen) pos = pathLen;
            int tileIdx = (int)pos;
            float frac = pos - (float)tileIdx;
            if (tileIdx >= tc - 1) { tileIdx = tc - 2; frac = 1.0f; }
            if (tileIdx < 0) { tileIdx = 0; frac = 0.0f; }
            const Vector2i& tileA = road->tiles[tileIdx];
            const Vector2i& tileB = road->tiles[tileIdx + 1];
            posX = (float)tileA.x + ((float)tileB.x - (float)tileA.x) * frac;
            posY = (float)tileA.y + ((float)tileB.y - (float)tileA.y) * frac;

            bool arrivedAtFlag = false;
            if (walkDir > 0.0f && ep >= pathLen) {
                arrivedAtFlag = true;
            } else if (walkDir < 0.0f && ep <= 0.0f) {
                arrivedAtFlag = true;
            }

            if (arrivedAtFlag) {
                routeIndex++;
                if (routeIndex < routeCount - 1) {
                    InitLeg(roadManager, route);
                }
            }

            return true;
        }
    };

} // namespace World