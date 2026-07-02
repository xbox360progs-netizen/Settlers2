#pragma once
#include "RenderCursor.h"

namespace Scene {

struct FrameContext;

// Reads cursor tile position from FrameContext and produces a RenderCursor DTO
// with world coords. ProjectionSystem later transforms to screen coords.
class CursorPresentationSystem {
public:
    CursorPresentationSystem() : m_suppressed(false) {}

    // Suppress cursor rendering (e.g. when placement preview is active).
    void SetSuppressed(bool suppressed) { m_suppressed = suppressed; }

    // Fill cursor DTO from current frame input state.
    // cursorTileX/Y must be set in frame.input before calling.
    void BuildRenderFrame(const FrameContext& frame, RenderCursor& outCursor);

private:
    bool m_suppressed;
};

} // namespace Scene
