#pragma once
#include <vector>
#include <queue>
#include <map>
#include "Flag.h"
#include "Road.h"

namespace World {

    class Pathfinding {
    public:
        static Flag* GetNextFlag(Flag* start, Flag* end) {
            if (!start || !end) return NULL;
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

                for (size_t i = 0; i < current->roads.size(); ++i) {
                    Road* r = current->roads[i];
                    Flag* neighbor = (r->a == current) ? r->b : r->a;
                    if (!neighbor) continue;
                    if (parent.find(neighbor) == parent.end()) {
                        parent[neighbor] = current;
                        q.push(neighbor);
                    }
                }
            }
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Path] NO PATH: flag %u -> flag %u (start roads=%u)\n",
                start->id, end->id, (unsigned)start->roads.size());
            OutputDebugStringA(buf);
            return NULL;
        }
    };
}
