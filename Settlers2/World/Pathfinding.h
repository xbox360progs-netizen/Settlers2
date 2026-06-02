#pragma once
#include <vector>
#include <queue>
#include <map>
#include "Flag.h"

namespace World {

    class Pathfinding {
    public:
        static Flag* GetNextFlag(Flag* start, Flag* end) {
            if (start == end) return NULL;

            std::queue<Flag*> q;
            std::map<Flag*, Flag*> parent;

            q.push(start);
            parent[start] = (Flag*)NULL;

            while (!q.empty()) {
                Flag* current = q.front();
                q.pop();

                if (current == end) {
                    Flag* step = end;
                    while (parent[step] != start) {
                        step = parent[step];
                    }
                    return step;
                }

                for (size_t i = 0; i < current->neighbors.size(); ++i) {
                    Flag* neighbor = current->neighbors[i];
                    if (parent.find(neighbor) == parent.end()) {
                        parent[neighbor] = current;
                        q.push(neighbor);
                    }
                }
            }
            return NULL;
        }
    };
}
