#pragma once

#include "../Logic/MapConstants.h"
#include <cmath>

// Two-layer coordinate system:
// Layer 0 (Ground): Large tiles 238x148
// Layer 1 (Nodes): Dense staggered grid 119x74 (half-tiles)

class CoordinateSystem {
private:
    // Layer 0 (Ground) params
    float m_tileW, m_tileH;
    int m_groundW, m_groundH;
    float m_groundLeft, m_groundTop, m_groundRight, m_groundBottom;

    // Layer 1 (Nodes) params - dense grid
    float m_nodeW, m_nodeH;
    int m_nodesW, m_nodesH;

public:
    static CoordinateSystem& GetInstance() {
        static CoordinateSystem instance;
        return instance;
    }

    void Initialize(int groundWidth, int groundHeight) {
        // Layer 0 (Ground) - full tiles
        m_tileW = 238.0f;
        m_tileH = 148.0f;
        m_groundW = groundWidth;
        m_groundH = groundHeight;

        float mapPixelW = m_groundW * m_tileW;
        float mapPixelH = m_groundH * m_tileH;
        m_groundLeft = -mapPixelW * 0.5f;
        m_groundTop = -mapPixelH * 0.5f;
        m_groundRight = mapPixelW * 0.5f;
        m_groundBottom = mapPixelH * 0.5f;

        // Layer 1 (Nodes) - half tiles, 2x denser
        m_nodeW = m_tileW * 0.5f;  // 119
        m_nodeH = m_tileH * 0.5f; // 74
        m_nodesW = m_groundW * 2;  // 40
        m_nodesH = m_groundH * 2;    // 40
    }

    void Initialize() {
        Initialize(20, 20);
    }

    // === LAYER 0: GROUND TILES ===

    // Ground tile to world coords (top-left corner of tile)
    void GroundTileToWorld(int tx, int ty, float& wx, float& wy) const {
        wx = m_groundLeft + tx * m_tileW;
        wy = m_groundTop + ty * m_tileH;
    }

    // Center of ground tile
    void GroundTileToWorldCenter(int tx, int ty, float& wx, float& wy) const {
        wx = m_groundLeft + tx * m_tileW + m_tileW * 0.5f;
        wy = m_groundTop + ty * m_tileH + m_tileH * 0.5f;
    }

    // World coords to ground tile
    void WorldToGroundTile(float wx, float wy, int& tx, int& ty) const {
        tx = static_cast<int>((wx - m_groundLeft) / m_tileW);
        ty = static_cast<int>((wy - m_groundTop) / m_tileH);
    }

    void GetGroundRectBounds(float& left, float& top, float& right, float& bottom) const {
        left = m_groundLeft;
        top = m_groundTop;
        right = m_groundRight;
        bottom = m_groundBottom;
    }

    int GetGroundWidth() const { return m_groundW; }
    int GetGroundHeight() const { return m_groundH; }

    // === LAYER 1: NODES (staggered grid) ===

    // Node tile to world coords (staggered)
    void NodeTileToWorld(int nx, int ny, float& wx, float& wy) const {
        // Origin at left-top of ground layer
        float originX = m_groundLeft;
        float originY = m_groundTop;

        // Staggered offset for odd rows
        float offsetX = (ny % 2) * (m_nodeW * 0.5f);

        wx = originX + nx * m_nodeW + offsetX;
        wy = originY + ny * m_nodeH;
    }

    // World coords to node tile (staggered)
    void WorldToNodeTile(float wx, float wy, int& nx, int& ny) const {
        float originX = m_groundLeft;
        float originY = m_groundTop;

        float relX = wx - originX;
        float relY = wy - originY;

        // Find row
        ny = static_cast<int>(relY / m_nodeH);

        // Find col with row offset
        float offsetX = (ny % 2) * (m_nodeW * 0.5f);
        nx = static_cast<int>((relX - offsetX) / m_nodeW);
    }

    void GetNodeRectBounds(float& left, float& top, float& right, float& bottom) const {
        left = m_groundLeft;
        top = m_groundTop;
        right = m_groundLeft + m_nodesW * m_nodeW;
        bottom = m_groundTop + m_nodesH * m_nodeH;
    }

    int GetNodesWidth() const { return m_nodesW; }
    int GetNodesHeight() const { return m_nodesH; }

    // Get center of map
    void GetDiamondCenter(float& cx, float& cy) const {
        cx = (m_groundLeft + m_groundRight) * 0.5f;
        cy = (m_groundTop + m_groundBottom) * 0.5f;
    }

    bool IsInBounds(int x, int y) const {
        return x >= 0 && x < 20 && y >= 0 && y < 20;
    }
};