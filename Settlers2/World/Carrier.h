#pragma once
#include "TransportJob.h"
#include "Flag.h"
#include "Pathfinding.h"
#include "../Core/Vector2i.h"

namespace World {

    enum CarrierState {
        Idle,
        WalkingToPickup,
        Pickup,
        WalkingToDestination,
        Drop
    };

    class Carrier {
    public:
        CarrierState state;
        TransportJob* currentJob;
        Flag* currentFlag;
        Flag* nextFlag;

        float progress;

        Carrier(Flag* startFlag)
            : state(Idle), currentJob(NULL),
              currentFlag(startFlag), nextFlag(NULL), progress(0.0f) {}

        void Update(float deltaTime) {
            switch (state) {
                case Idle:
                    break;
                case WalkingToPickup:
                    if (!nextFlag) {
                        nextFlag = Pathfinding::GetNextFlag(currentFlag, currentJob->source);
                    }
                    Move(deltaTime);
                    break;
                case WalkingToDestination:
                    if (!nextFlag) {
                        nextFlag = Pathfinding::GetNextFlag(currentFlag, currentJob->destination);
                    }
                    Move(deltaTime);
                    break;
                case Pickup:
                case Drop:
                    state = (state == Pickup) ? WalkingToDestination : Idle;
                    break;
            }
        }

    private:
        void Move(float deltaTime) {
            if (nextFlag) {
                progress += deltaTime * 0.5f;
                if (progress >= 1.0f) {
                    currentFlag = nextFlag;
                    nextFlag = NULL;
                    progress = 0.0f;

                    if (currentFlag == currentJob->source && state == WalkingToPickup) {
                        state = Pickup;
                    } else if (currentFlag == currentJob->destination && state == WalkingToDestination) {
                        state = Drop;
                    }
                }
            }
        }
    };
}
