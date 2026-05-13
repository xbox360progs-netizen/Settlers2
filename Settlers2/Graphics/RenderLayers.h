#pragma once

// Render layer enumeration for proper depth sorting
// Lower numbers = rendered later (on top)
// Higher numbers = rendered earlier (behind)
enum RenderLayer {
    LAYER_BACKGROUND = 0,       // Far background elements (draw first)
    LAYER_TERRAIN = 100,       // Ground/terrain tiles
    LAYER_WORLD = 500,          // World objects (buildings, units)
    LAYER_EFFECTS = 800,        // Visual effects, particles
    LAYER_UI = 900,             // UI elements, menus
    LAYER_FOREGROUND = 1000     // Text, cursors, tooltips (always on top, draw last)
};

// Helper functions for depth calculation
namespace LayerUtils {
    // Convert layer and Y position to composite depth
    inline float CalculateDepth(RenderLayer layer, float yPosition = 0.0f, float yScale = 0.001f) {
        return static_cast<float>(layer) + (yPosition * yScale);
    }
    
    // Get depth for UI elements (no Y sorting needed)
    inline float GetUIDepth(RenderLayer layer = LAYER_UI) {
        return static_cast<float>(layer);
    }
    
    // Get depth for world objects with Y sorting
    inline float GetWorldDepth(RenderLayer layer, float worldY, float yScale = 0.001f) {
        return CalculateDepth(layer, worldY, yScale);
    }
}
