#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"
#include "../Core/BuildingTypes.h"
#include "../Core/ResourceTypes.h"

namespace World {

    class EconomySystem;
    class JobManager;
    struct WorldModel;
    struct ProductionBuilding;

    // Identifies which expansion rule fired.
    enum ExpansionRuleId {
        ER_None = 0,
        ER_Bootstrap_Woodcutter,
        ER_First_Sawmill,
        ER_Expand_Sawmill,
        ER_Bootstrap_Storehouse,
        ER_First_Quarry,
        ER_First_Forester,
        ER_Expand_Forester,
        ER_Emergency_Forester,
        ER_First_Toolmaker,
        ER_First_Hunter,
        ER_First_Fisher,
        ER_Mine_Food,
        ER_First_Farm,
        ER_First_Mill,
        ER_First_Bakery,
        ER_First_CoalMine,
        ER_Expand_CoalMine,
        ER_First_IronMine,
        ER_First_IronSmelter,
        ER_First_WeaponSmith,
        ER_First_Barracks,
        ER_Count
    };

    // A single expansion decision record — what was built, when, and why.
    struct ExpansionEvent {
        uint32_t tick;
        ExpansionRuleId ruleId;
        BuildingType buildingType;
        int woodFlow;
        float woodPotential;
        int stoneFlow;
        float stonePotential;
        int availableWood;

        ExpansionEvent()
            : tick(0), ruleId(ER_None), buildingType(BuildingType_None)
            , woodFlow(0), woodPotential(0.0f)
            , stoneFlow(0), stonePotential(0.0f)
            , availableWood(0)
        {
        }
    };

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

        // Expansion event log — observational, no behavioral effect.
        int GetExpansionEventCount() const { return m_eventCount; }
        const ExpansionEvent& GetExpansionEvent(int index) const;

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

        // Record an expansion decision event with current economy snapshot.
        void RecordEvent(ExpansionRuleId ruleId, BuildingType bt, const WorldModel& world);

        bool HasBuilding(BuildingType type, const WorldModel& world) const;
        bool HasConstructionSite(BuildingType type, const WorldModel& world) const;
        bool HasPendingJob(BuildingType type, const WorldModel& world) const;

        JobManager* m_jobManager;
        EconomySystem* m_economySystem;
        uint32_t m_tickCount;
        bool m_hasMineFoodRule;
        ExpansionSignal m_coalSignal;

        static const int kWoodForSawmill = 8;

        // Observational event log — purely diagnostic, no behavior change.
        static const int kMaxEvents = 128;
        ExpansionEvent m_events[kMaxEvents];
        int m_eventCount;
    };

}
