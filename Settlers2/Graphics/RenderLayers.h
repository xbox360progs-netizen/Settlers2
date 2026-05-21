#pragma once

enum RenderLayer {
    LAYER_BACKGROUND = 0,
    LAYER_TERRAIN = 1,
    LAYER_WORLD = 2,
    LAYER_EFFECTS = 3,
    LAYER_UI = 4,
    LAYER_FOREGROUND = 5
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
