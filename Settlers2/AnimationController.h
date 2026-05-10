#pragma once
#include "SpriteAtlas.h"

// AnimationController for CPU-side animation playback logic
// Optimized for Xbox 360 multi-threading
class AnimationController
{
private:
    float m_currentTime;
    int   m_direction; // For Ping-Pong playback (1 = forward, -1 = backward)

public:
    AnimationController() : m_currentTime(0.0f), m_direction(1) {}

    // Reset animation to beginning
    void Reset() {
        m_currentTime = 0.0f;
        m_direction = 1;
    }

    // Get current frame index based on elapsed time
    uint32_t GetCurrentFrameIndex(const SpriteAnimation& anim, float deltaTime) {
        m_currentTime += deltaTime * anim.speedMultiplier;

        float frameDuration = 1.0f / (float)anim.frameRate;
        int totalFrames = (int)(anim.endFrame - anim.startFrame + 1);

        if (totalFrames <= 0) return anim.startFrame;

        int currentFrame = (int)(m_currentTime / frameDuration);

        if (anim.loop) {
            // Standard loop
            return anim.startFrame + (currentFrame % totalFrames);
        }
        else if (anim.pingPong) {
            // Ping-pong playback
            int cycleLength = totalFrames * 2 - 2; // e.g., 0,1,2,3,2,1,0 for 4 frames
            int posInCycle = currentFrame % cycleLength;

            if (posInCycle < totalFrames) {
                // Forward direction
                return anim.startFrame + posInCycle;
            } else {
                // Backward direction
                return anim.endFrame - (posInCycle - totalFrames);
            }
        }
        else {
            // One-shot (clamp to last frame)
            if (currentFrame >= totalFrames) {
                return anim.endFrame;
            }
            return anim.startFrame + currentFrame;
        }
    }

    // Check if animation has finished (for non-looping animations)
    bool IsFinished(const SpriteAnimation& anim) const {
        if (anim.loop) return false;

        float frameDuration = 1.0f / (float)anim.frameRate;
        int totalFrames = (int)(anim.endFrame - anim.startFrame + 1);
        int currentFrame = (int)(m_currentTime / frameDuration);

        return currentFrame >= totalFrames;
    }

    // Get current animation time (for debugging or custom logic)
    float GetCurrentTime() const { return m_currentTime; }
};
