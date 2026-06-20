#pragma once
#include <string.h>
#include <stdint.h>

namespace World {

    static const size_t PATH_MAX_FLAGS = 256;
    static const uint8_t PATH_NO_ROUTE = 0xFF;

    class Pathfinding {
    private:
        // Next-hop routing table: routingTable[srcIdx][dstIdx] = next hop index
        uint8_t m_routingTable[PATH_MAX_FLAGS][PATH_MAX_FLAGS];
        // Distance table for Floyd-Warshall (128KB, used only during rebuild)
        uint8_t m_distanceTable[PATH_MAX_FLAGS][PATH_MAX_FLAGS];

    public:
        Pathfinding() {
            Clear();
        }

        void Clear() {
            memset(m_routingTable, PATH_NO_ROUTE, sizeof(m_routingTable));
            memset(m_distanceTable, 0x7F, sizeof(m_distanceTable));

            for (size_t i = 0; i < PATH_MAX_FLAGS; ++i) {
                m_distanceTable[i][i] = 0;
                m_routingTable[i][i] = (uint8_t)i;
            }
        }

        // O(1) next-hop query — replaces BFS GetNextHop
        inline uint8_t GetNextFlagIdx(uint32_t startIdx, uint32_t endIdx) const {
            if (startIdx >= PATH_MAX_FLAGS || endIdx >= PATH_MAX_FLAGS)
                return PATH_NO_ROUTE;
            return m_routingTable[startIdx][endIdx];
        }

        // Full path reconstruction from routing table (no heap allocations)
        // Returns number of flags written to outPath (up to maxLen)
        inline uint32_t ReconstructPath(uint32_t startIdx, uint32_t endIdx,
                                        uint32_t* outPath, uint32_t maxLen) const {
            if (startIdx >= PATH_MAX_FLAGS || endIdx >= PATH_MAX_FLAGS) return 0;
            if (startIdx == endIdx) {
                if (maxLen >= 1) outPath[0] = startIdx;
                return (maxLen >= 1) ? 1 : 0;
            }

            uint32_t count = 0;
            uint32_t cur = startIdx;
            uint32_t guard = PATH_MAX_FLAGS;
            while (cur != endIdx && guard > 0) {
                if (count >= maxLen) return count;
                outPath[count++] = cur;
                cur = m_routingTable[cur][endIdx];
                if (cur == PATH_NO_ROUTE) return 0;
                --guard;
            }
            if (guard == 0 || count >= maxLen) return 0;
            outPath[count++] = endIdx;
            return count;
        }

        // Heavy rebuild — call ONLY when road network changes (not per-frame)
        void RebuildRoutingTable(const void* const* roadGraph, size_t stride) {
            Clear();

            for (size_t a = 0; a < PATH_MAX_FLAGS; ++a) {
                for (size_t b = 0; b < PATH_MAX_FLAGS; ++b) {
                    if (roadGraph[a * stride + b] != NULL) {
                        m_distanceTable[a][b] = 1;
                        m_routingTable[a][b] = (uint8_t)b;
                    }
                }
            }

            for (size_t k = 0; k < PATH_MAX_FLAGS; ++k) {
                for (size_t i = 0; i < PATH_MAX_FLAGS; ++i) {
                    if (m_distanceTable[i][k] == 0x7F) continue;
                    for (size_t j = 0; j < PATH_MAX_FLAGS; ++j) {
                        if (m_distanceTable[k][j] == 0x7F) continue;
                        uint32_t nd = (uint32_t)m_distanceTable[i][k] + (uint32_t)m_distanceTable[k][j];
                        if (nd < (uint32_t)m_distanceTable[i][j]) {
                            m_distanceTable[i][j] = (uint8_t)nd;
                            m_routingTable[i][j] = m_routingTable[i][k];
                        }
                    }
                }
            }
        }
    };

} // namespace World