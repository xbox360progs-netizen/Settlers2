#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"
#include "../Core/BuildingTypes.h"

namespace World {

    class EconomySystem;
    class JobManager;
    struct WorldModel;

    // Signal accumulator for expansion decisions.
    // Tracks how many consecutive full windows a condition has been true.
    // Used to add hysteresis: expand only after sustained saturation.
    struct ExpansionSignal {
        ExpansionSignal() : m_ticksConditionMet(0), m_consecutiveWindows(0) {}

        // Call every tick. Resets on condition=false.
        // Returns true when thresholdInWindows consecutive full windows
        // have been observed (each window = ticksPerWindow ticks).
        bool Update(bool condition, int thresholdInWindows, int ticksPerWindow)
        {
            if (condition) {
                m_ticksConditionMet++;
            } else {
                m_ticksConditionMet = 0;
                m_consecutiveWindows = 0;
                return false;
            }

            if (m_ticksConditionMet >= ticksPerWindow) {
                m_consecutiveWindows++;
                m_ticksConditionMet = 0;
            }

            return (m_consecutiveWindows >= thresholdInWindows);
        }

        void Reset() { m_ticksConditionMet = 0; m_consecutiveWindows = 0; }

        int GetConsecutiveWindows() const { return m_consecutiveWindows; }

    private:
        int m_ticksConditionMet;
        int m_consecutiveWindows;
    };

    class SettlementSystem : public ISimulationSystem {
    public:
        SettlementSystem();
        ~SettlementSystem();

        void SetJobManager(JobManager* jm) { m_jobManager = jm; }
        void SetEconomySystem(EconomySystem* es) { m_economySystem = es; }
        virtual void Tick(WorldModel& world);

    private:
        void BootstrapProduction(WorldModel& world);
        void BootstrapIndustry(WorldModel& world);
        void BootstrapInfrastructure(WorldModel& world);
        void BootstrapMining(WorldModel& world);
        void BootstrapForestry(WorldModel& world);
        void BootstrapToolProduction(WorldModel& world);
        void BootstrapHunter(WorldModel& world);
        void BootstrapFisher(WorldModel& world);
        void BootstrapMiningExpanded(WorldModel& world);
        void BootstrapAgriculture(WorldModel& world);
        void BootstrapMetallurgy(WorldModel& world);
        void BootstrapMilitary(WorldModel& world);

        bool HasBuilding(BuildingType type, const WorldModel& world) const;
        bool HasConstructionSite(BuildingType type, const WorldModel& world) const;
        bool HasPendingJob(BuildingType type, const WorldModel& world) const;

        JobManager* m_jobManager;
        EconomySystem* m_economySystem;
        uint32_t m_tickCount;
        bool m_hasMineFoodRule;
        ExpansionSignal m_coalSignal;

        static const int kWoodForSawmill = 8;
    };

}
