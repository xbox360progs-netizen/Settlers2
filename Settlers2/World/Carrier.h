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
        Cargo cargo;
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
                    if (currentJob) {
                        cargo = currentJob->cargo;
                        currentFlag->CommitPickup(cargo.type, cargo.amount);
                    }
                    state = WalkingToDestination;
                    break;
                case Drop:
                    if (currentJob) {
                        currentFlag->AddResource(cargo.type, cargo.amount);
                    }
                    delete currentJob;
                    currentJob = NULL;
                    cargo = Cargo();
                    state = Idle;
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
