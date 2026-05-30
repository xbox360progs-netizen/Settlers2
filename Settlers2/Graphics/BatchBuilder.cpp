#include "stdafx.h"
#include "BatchBuilder.h"
#include "ShaderManager.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

BatchBuilder::BatchBuilder()
    : m_batchCount(0)
    , m_vertexWritePos(0)
    , m_indexWritePos(0)
    , m_currentTexture(0xFFFF)
    , m_currentShader(0xFFFF)
    , m_currentBlend(0xFF)
{
}

BatchBuilder::~BatchBuilder() {
}

void BatchBuilder::BeginFrame() {
    Clear();
}

void BatchBuilder::EndFrame() {
}

void BatchBuilder::BuildBatches(const RenderCommand* commands, uint32_t commandCount) {
    if (commandCount == 0 || !commands) return;

    m_batchCount = 0;
    m_vertexWritePos = 0;
    m_indexWritePos = 0;

    m_currentTexture = 0xFFFF;
    m_currentShader = 0xFFFF;
    m_currentBlend = 0xFF;

    RenderBatch* currentBatch = NULL;

    for (uint32_t i = 0; i < commandCount; i++) {
        const RenderCommand& cmd = commands[i];
        WORD shaderID = cmd.shaderID;
        if (shaderID >= SHADER_COUNT) {
            char buf[256];
            sprintf(buf, "[BatchBuilder] WARNING: invalid shaderID=%u in RenderCommand, clamping to SHADER_SPRITE\n", shaderID);
            OutputDebugStringA(buf);
            shaderID = SHADER_SPRITE;
        }

        bool needsNewBatch = (m_batchCount == 0)
            || (cmd.textureID != m_currentTexture)
            || (shaderID != m_currentShader)
            || (cmd.blendMode != m_currentBlend);

        if (needsNewBatch) {
            if (m_batchCount >= MAX_BATCHES) {
                OutputDebugStringA("[BatchBuilder] WARNING: Max batches exceeded\n");
                break;
            }

            currentBatch = &m_batches[m_batchCount++];
            currentBatch->textureID = cmd.textureID;
            currentBatch->shaderID = shaderID;
            currentBatch->blendMode = cmd.blendMode;
            currentBatch->startIndex = m_indexWritePos;
            currentBatch->indexCount = 0;

            m_currentTexture = cmd.textureID;
            m_currentShader = shaderID;
            m_currentBlend = cmd.blendMode;
        }

        if (m_vertexWritePos + 4 > MAX_VERTICES) {
            OutputDebugStringA("[BatchBuilder] WARNING: Max vertices exceeded\n");
            break;
        }

        SpriteVertex* dst = m_vertexPool + m_vertexWritePos;
        uint32_t baseIdx = m_vertexWritePos;

        dst[0].x = cmd.x;                  dst[0].y = cmd.y;                  dst[0].z = 0.0f;
        dst[0].u = cmd.u0;                 dst[0].v = cmd.v0;
        dst[0].color = cmd.color;
        dst[0].padding[0] = 0; dst[0].padding[1] = 0;

        dst[1].x = cmd.x + cmd.width;      dst[1].y = cmd.y;                  dst[1].z = 0.0f;
        dst[1].u = cmd.u1;                 dst[1].v = cmd.v0;
        dst[1].color = cmd.color;
        dst[1].padding[0] = 0; dst[1].padding[1] = 0;

        dst[2].x = cmd.x;                  dst[2].y = cmd.y + cmd.height;     dst[2].z = 0.0f;
        dst[2].u = cmd.u0;                 dst[2].v = cmd.v1;
        dst[2].color = cmd.color;
        dst[2].padding[0] = 0; dst[2].padding[1] = 0;

        dst[3].x = cmd.x + cmd.width;      dst[3].y = cmd.y + cmd.height;     dst[3].z = 0.0f;
        dst[3].u = cmd.u1;                 dst[3].v = cmd.v1;
        dst[3].color = cmd.color;
        dst[3].padding[0] = 0; dst[3].padding[1] = 0;

        // ÈÑÏÐÀÂËÅÍÎ: èñïîëüçóåì uint16_t* äëÿ èíäåêñîâ
        uint16_t* idx = m_indexPool + m_indexWritePos;
        idx[0] = (uint16_t)(baseIdx + 0);
        idx[1] = (uint16_t)(baseIdx + 1);
        idx[2] = (uint16_t)(baseIdx + 2);
        idx[3] = (uint16_t)(baseIdx + 2);
        idx[4] = (uint16_t)(baseIdx + 1);
        idx[5] = (uint16_t)(baseIdx + 3);

        m_vertexWritePos += 4;
        m_indexWritePos += 6;

        currentBatch->indexCount += 6;
    }

}

void BatchBuilder::Clear() {
    m_batchCount = 0;
    m_vertexWritePos = 0;
    m_indexWritePos = 0;
    m_currentTexture = 0xFFFF;
    m_currentShader = 0xFFFF;
    m_currentBlend = 0xFF;
}

}