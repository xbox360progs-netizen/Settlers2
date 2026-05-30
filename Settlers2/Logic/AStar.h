#pragma once
#include <vector>
#include <algorithm>
#include <cmath>

namespace Logic {

// Simple priority queue for A* (binary heap-based, no C++11 features needed)
class AStarOpenList {
public:
    struct Entry {
        int x, y;
        float f;
    };

    void push(int x, int y, float f) {
        Entry e = {x, y, f};
        m_heap.push_back(e);
        int i = (int)m_heap.size() - 1;
        while (i > 0) {
            int p = (i - 1) / 2;
            if (m_heap[p].f <= m_heap[i].f) break;
            Entry tmp = m_heap[p]; m_heap[p] = m_heap[i]; m_heap[i] = tmp;
            i = p;
        }
    }

    Entry pop() {
        Entry top = m_heap.front();
        m_heap.front() = m_heap.back();
        m_heap.pop_back();
        if (!m_heap.empty()) {
            int i = 0;
            int n = (int)m_heap.size();
            for (;;) {
                int smallest = i;
                int left = 2 * i + 1;
                int right = 2 * i + 2;
                if (left < n && m_heap[left].f < m_heap[smallest].f) smallest = left;
                if (right < n && m_heap[right].f < m_heap[smallest].f) smallest = right;
                if (smallest == i) break;
                Entry tmp = m_heap[i]; m_heap[i] = m_heap[smallest]; m_heap[smallest] = tmp;
                i = smallest;
            }
        }
        return top;
    }

    bool empty() const { return m_heap.empty(); }
    void clear() { m_heap.clear(); }

private:
    std::vector<Entry> m_heap;
};

// Neighbor functor: 4-directional grid (left/right/up/down)
struct FourDirNeighbors {
    void operator()(int x, int y, int* nxs, int* nys, int& count) const {
        nxs[0]=x+1; nys[0]=y;
        nxs[1]=x-1; nys[1]=y;
        nxs[2]=x;   nys[2]=y+1;
        nxs[3]=x;   nys[3]=y-1;
        count = 4;
    }
};

// Neighbor functor: isometric staggered grid (NE, SE, SW, NW)
struct IsoNeighbors {
    void operator()(int x, int y, int* nxs, int* nys, int& count) const {
        bool evenRow = (y % 2 == 0);
        nxs[0] = evenRow ? x+1 : x;   nys[0] = y-1;  // NE
        nxs[1] = evenRow ? x+1 : x;   nys[1] = y+1;  // SE
        nxs[2] = evenRow ? x : x-1;   nys[2] = y+1;  // SW
        nxs[3] = evenRow ? x : x-1;   nys[3] = y-1;  // NW
        count = 4;
    }
};

class AStar {
public:
    // IsPassable: return true if (x,y) can be traversed
    // GetCost: return movement cost (>0) for (x,y), or 0 if impassable
    // GetNeighbors: fills arrays with up to 4 neighbor coordinates
    template<typename IsPassableFn, typename GetCostFn, typename GetNeighborsFn>
    static bool FindPath(
        int startX, int startY,
        int endX, int endY,
        int gridW, int gridH,
        IsPassableFn isPassable,
        GetCostFn getCost,
        GetNeighborsFn getNeighbors,
        std::vector<std::pair<int,int>>& outPath
    ) {
        outPath.clear();

        if (!isPassable(startX, startY) || !isPassable(endX, endY))
            return false;

        int size = gridW * gridH;
        std::vector<int> parent(size, -1);
        std::vector<float> gScore(size, 1e9f);
        std::vector<float> fScore(size, 1e9f);
        std::vector<bool> closed(size, false);

        AStarOpenList open;

        gScore[startY * gridW + startX] = 0.0f;
        fScore[startY * gridW + startX] = Heuristic(startX, startY, endX, endY);
        open.push(startX, startY, fScore[startY * gridW + startX]);

        while (!open.empty()) {
            AStarOpenList::Entry cur = open.pop();
            int curIdx = cur.y * gridW + cur.x;

            if (cur.f > fScore[curIdx] + 0.001f)
                continue;

            if (cur.x == endX && cur.y == endY) {
                ReconstructPath(parent, gridW, startX, startY, endX, endY, outPath);
                return true;
            }

            closed[curIdx] = true;

            int nxs[4], nys[4], nCount;
            getNeighbors(cur.x, cur.y, nxs, nys, nCount);

            for (int d = 0; d < nCount; ++d) {
                int nx = nxs[d];
                int ny = nys[d];
                if (nx < 0 || nx >= gridW || ny < 0 || ny >= gridH)
                    continue;

                int nIdx = ny * gridW + nx;
                if (closed[nIdx])
                    continue;

                if (!isPassable(nx, ny))
                    continue;

                float moveCost = getCost(nx, ny);
                if (moveCost <= 0.0f)
                    continue;

                float tentativeG = gScore[curIdx] + moveCost;
                if (tentativeG < gScore[nIdx]) {
                    parent[nIdx] = curIdx;
                    gScore[nIdx] = tentativeG;
                    fScore[nIdx] = tentativeG + Heuristic(nx, ny, endX, endY);
                    open.push(nx, ny, fScore[nIdx]);
                }
            }
        }

        return false;
    }

private:
    static float Heuristic(int x1, int y1, int x2, int y2) {
        return (float)(abs(x1 - x2) + abs(y1 - y2));
    }

    static void ReconstructPath(
        const std::vector<int>& parent, int gridW,
        int startX, int startY, int endX, int endY,
        std::vector<std::pair<int,int>>& outPath
    ) {
        std::vector<std::pair<int,int> > rev;
        int cx = endX, cy = endY;
        while (!(cx == startX && cy == startY)) {
            rev.push_back(std::make_pair(cx, cy));
            int pIdx = parent[cy * gridW + cx];
            if (pIdx < 0) break;
            cx = pIdx % gridW;
            cy = pIdx / gridW;
        }
        rev.push_back(std::make_pair(startX, startY));
        outPath.assign(rev.rbegin(), rev.rend());
    }
};

} // namespace Logic
