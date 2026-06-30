#ifndef WORLD_COMPONENTS_PRODUCTIONBUILDING_H
#define WORLD_COMPONENTS_PRODUCTIONBUILDING_H

#include "Building.h"

namespace World {

class ProductionBuilding : public Building {
public:
    ProductionBuilding(BuildingType t, int x, int y, uint8_t o, Map* m)
        : Building(t, x, y, o, m) {
        m_productionInterval = 3.0f;
    }

    void SyncIOFromRules() {
        for (int r = 0; r < m_numRules; ++r) {
            for (int i = 0; i < m_rules[r].numInputs; ++i)
                inputResources.push_back(m_rules[r].input[i]);
            for (int o = 0; o < m_rules[r].numOutputs; ++o)
                outputResources.push_back(m_rules[r].output[o]);
        }
    }

    void Update(float dt) override {
        m_productionTimer += dt;

        while (m_productionTimer >= m_productionInterval) {
            m_productionTimer -= m_productionInterval;

            bool produced = false;
            for (int r = 0; r < m_numRules; ++r) {
            ProductionRule& rule = m_rules[r];

            // Check input availability
            bool canProduce = true;
            for (int i = 0; i < rule.numInputs; ++i) {
                if (m_storage[rule.input[i]] < rule.inputAmount[i]) {
                    canProduce = false;
                    break;
                }
            }
            if (!canProduce) continue;

            // Check output capacity
            bool hasRoom = true;
            for (int o = 0; o < rule.numOutputs; ++o) {
                if (m_storage[rule.output[o]] >= rule.outputCap) {
                    hasRoom = false;
                    break;
                }
            }
            if (!hasRoom) continue;

            // Consume inputs
            for (int i = 0; i < rule.numInputs; ++i) {
                m_storage[rule.input[i]] -= rule.inputAmount[i];
                if (m_storage[rule.input[i]] < 0)
                    m_storage[rule.input[i]] = 0;
            }

            // Produce outputs
            for (int o = 0; o < rule.numOutputs; ++o) {
                m_storage[rule.output[o]] += rule.outputAmount[o];
            }
            produced = true;
        }
        if (!produced) break;  // no rule could fire — avoid infinite loop
    }
    }
};

} // namespace World

#endif
