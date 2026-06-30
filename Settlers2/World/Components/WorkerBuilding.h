#pragma once
#include "Building.h"
#include "../Map.h"
#include "../Flag.h"
#include <cmath>

namespace World {

class WorkerBuilding : public Building {
protected:
    enum WorkerState {
        WState_Idle,
        WState_Leaving,
        WState_WalkingToTarget,
        WState_Working,
        WState_Returning
    };

    WorkerState m_wState;
    float m_wX, m_wY;
    float m_wVx, m_wVy;
    float m_wTimer;
    int   m_wDir;        // 0=SE, 1=SW
    int   m_workFrame;   // Animation toggle

    Vector2i m_targetPos;
    bool m_hasTarget;

    // Shared constants (can be overridden by derived)
    float m_idleDuration;
    float m_workDuration;
    float m_workerSpeed;
    float m_searchCooldown;

    void StartWalking(float dx, float dy) {
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 0.5f) return;
        m_wVx = dx / d * m_workerSpeed;
        m_wVy = dy / d * m_workerSpeed;
        m_wDir = (m_wVx >= 0.0f) ? 0 : 1;
    }

    void StartWalkingToTarget() {
        float dx = (float)m_targetPos.x - m_wX;
        float dy = (float)m_targetPos.y - m_wY;
        float d = sqrtf(dx * dx + dy * dy);
        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg),
                "[WorkerBuilding] StartWalkingToTarget ENTER worker=(%.1f,%.1f) target=(%d,%d) d=%.1f\n",
                m_wX, m_wY, m_targetPos.x, m_targetPos.y, d);
            OutputDebugStringA(dbg);
        }
        if (d < 0.5f) {
            m_wState = WState_Working;
            m_wTimer = 0.0f;
            return;
        }
        StartWalking(dx, dy);
        m_wState = WState_WalkingToTarget;
    }

    void StartWalkingHome() {
        if (!connectedFlag) { OnArriveHome(); return; }
        float dx = (float)connectedFlag->pos.x - m_wX;
        float dy = (float)connectedFlag->pos.y - m_wY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 0.5f) {
            OnArriveHome();
            return;
        }
        StartWalking(dx, dy);
        m_wState = WState_Returning;
    }

    virtual void GoIdle() {
        m_wState = WState_Idle;
        m_wTimer = 0.0f;
        m_wVx = 0.0f;
        m_wVy = 0.0f;
        m_hasTarget = false;
        if (connectedFlag) {
            m_wX = (float)connectedFlag->pos.x;
            m_wY = (float)connectedFlag->pos.y;
        }
    }

    // Override to run custom logic when worker arrives home (e.g. deposit resource)
    virtual void OnArriveHome() { GoIdle(); }

    // To be implemented by derived buildings
    virtual bool FindTarget() = 0;
    virtual void Produce() = 0;
    virtual bool ValidateTarget() const = 0;

public:
    WorkerBuilding(BuildingType type, int x, int y, uint8_t o, Map* m)
        : Building(type, x, y, o, m)
        , m_wState(WState_Idle)
        , m_wX((float)x), m_wY((float)y)
        , m_wVx(0.0f), m_wVy(0.0f)
        , m_wTimer(0.0f)
        , m_wDir(0)
        , m_workFrame(0)
        , m_targetPos(0, 0)
        , m_hasTarget(false)
        , m_idleDuration(5.0f)
        , m_workDuration(2.0f)
        , m_workerSpeed(1.0f)
        , m_searchCooldown(5.0f)
    {}

    void Update(float dt) override {
        if (m_population <= 0) return;

        // Clamp worker position to node-layer bounds (otherWidth × otherHeight)
        int mapW = map ? map->GetWidth() * 2 : 1000;
        int mapH = map ? map->GetHeight() * 4 : 1000;
        if (m_wX < 0.0f) m_wX = 0.0f;
        if (m_wY < 0.0f) m_wY = 0.0f;
        if (m_wX >= (float)mapW) m_wX = (float)mapW - 0.1f;
        if (m_wY >= (float)mapH) m_wY = (float)mapH - 0.1f;

        // Refresh resource cache periodically for O(1) FindTarget
        m_cacheTimer += dt;
        if (m_cacheTimer >= 2.0f) {
            m_cacheTimer = 0.0f;
            RefreshResourceCache();
        }

        switch (m_wState) {
            case WState_Idle: {
                m_wTimer += dt;
                if (m_wTimer >= m_idleDuration) {
                    m_wTimer = 0.0f;
                    if (FindTarget()) {
                        m_wX = (float)connectedFlag->pos.x;
                        m_wY = (float)connectedFlag->pos.y;
                        m_hasTarget = true;
                        m_wState = WState_Leaving;
                    } else {
                        m_wTimer = m_idleDuration - m_searchCooldown;
                    }
                }
                break;
            }
            case WState_Leaving:
                StartWalkingToTarget();
                break;
            case WState_WalkingToTarget: {
                if (!ValidateTarget()) { GoIdle(); return; }
                m_wX += m_wVx * dt;
                m_wY += m_wVy * dt;
                float dx = (float)m_targetPos.x - m_wX;
                float dy = (float)m_targetPos.y - m_wY;
                if (dx * dx + dy * dy <= 0.25f) {
                    m_wX = (float)m_targetPos.x;
                    m_wY = (float)m_targetPos.y;
                    m_wVx = 0.0f; m_wVy = 0.0f;
                    m_wState = WState_Working;
                    m_wTimer = 0.0f;
                }
                break;
            }
            case WState_Working: {
                if (!ValidateTarget()) { GoIdle(); return; }
                m_wTimer += dt;
                m_workFrame = ((int)(m_wTimer * 4.0f)) & 1;
                if (m_wTimer >= m_workDuration) {
                    Produce();
                    StartWalkingHome();
                }
                break;
            }
            case WState_Returning: {
                if (!connectedFlag) { GoIdle(); return; }
                m_wX += m_wVx * dt;
                m_wY += m_wVy * dt;
                float dx = (float)connectedFlag->pos.x - m_wX;
                float dy = (float)connectedFlag->pos.y - m_wY;
                if (dx * dx + dy * dy <= 0.25f) {
                    m_wX = (float)connectedFlag->pos.x;
                    m_wY = (float)connectedFlag->pos.y;
                    m_wVx = 0.0f; m_wVy = 0.0f;
                    OnArriveHome();
                }
                break;
            }
        }
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (m_wState == WState_Idle) return false;
        outX = m_wX;
        outY = m_wY;
        return true; // Derived must provide outSpriteIdx
    }
};

} // namespace World
