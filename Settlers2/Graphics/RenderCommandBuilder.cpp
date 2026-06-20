#include "stdafx.h"
#include "RenderCommandBuilder.h"
#include "RenderQueue.h"

namespace Graphics {

void RenderCommandBuilder::Submit(RenderQueue* queue) const {
    if (queue) {
        queue->Submit(m_cmd);
    }
}

}
