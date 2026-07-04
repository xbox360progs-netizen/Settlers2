#include "SimpleTransportDriver.h"
#include "TransportController.h"
#include "Carrier.h"
#include "Cargo.h"
#include "../World/WorldModel.h"

namespace World {

    SimpleTransportDriver::SimpleTransportDriver(TransportController& controller)
        : m_controller(controller)
        , m_cargoCount(0)
        , m_nextCargoId(1)
        , m_activeCarrierCount(0)
    {
    }

    SimpleTransportDriver::~SimpleTransportDriver()
    {
        for (int i = 0; i < m_cargoCount; ++i)
            delete m_cargoPool[i];
        m_cargoCount = 0;
    }

    void SimpleTransportDriver::Tick(WorldModel& world)
    {
        // Compact and free consumed cargo
        int writeIdx = 0;
        for (int i = 0; i < m_cargoCount; ++i) {
            Cargo* c = m_cargoPool[i];
            if (IsCargoConsumed(c)) {
                delete c;
            } else {
                m_cargoPool[writeIdx++] = c;
            }
        }
        m_cargoCount = writeIdx;

        // Drive transport for each flag with waiting task
        for (FlagId f = 0; f < kMaxFlags; ++f) {
            while (m_controller.GetWaitingCount(f) > 0 && m_activeCarrierCount < 64) {
                DriveOne(f);
            }
        }
    }

void SimpleTransportDriver::DriveOne(FlagId sourceFlag)
{
    Carrier* c = new Carrier(NULL);
    c->m_phase7Task = NULL;
    c->m_phase7Controller = NULL;
    m_activeCarrierCount++;

    Cargo* cargo = NULL;
    TransportTask* cargoTask = NULL;

    m_controller.NotifyCarrierIdle(c, sourceFlag);

    while (c->m_phase7Task != NULL) {
        TransportTask* task = c->m_phase7Task;

        if (task->state == TTS_Assigned) {
            if (cargo == NULL) {
                if (m_cargoCount < kMaxCargo) {
                    cargo = new Cargo();
                    cargo->id = m_nextCargoId++;
                    cargo->type = task->resource;
                    cargo->amount = 1;
                    cargo->ownerTask = task;
                    cargoTask = task;
                    m_cargoPool[m_cargoCount++] = cargo;
                }
            }
            Cargo* cargoToPick = (cargo != NULL) ? cargo : task->cargo;
            if (cargoToPick != NULL) {
                m_controller.NotifyCarrierPickedUp(c, cargoToPick);
            } else {
                break;
            }
        } else if (task->state == TTS_Moving) {
            m_controller.NotifyCarrierArrived(c, c->m_phase7TargetFlag);
        } else {
            break;
        }
    }

    delete c;
    m_activeCarrierCount--;
}

bool SimpleTransportDriver::IsCargoConsumed(Cargo* cargo) const
{
    if (!cargo->ownerTask) return false;
    return cargo->ownerTask->state == TTS_Delivered
        && cargo->ownerTask->cargo == NULL;
}

} // namespace World
