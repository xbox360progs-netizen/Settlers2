#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace Graphics {

class LightOptimizer {
public:
    LightOptimizer();
    ~LightOptimizer();

    void SetDevice(LPDIRECT3DDEVICE9 pDevice) { m_pDevice = pDevice; }

    void ApplyScissorRectForPointLight(const D3DXVECTOR3& lightPos, float radius, int screenWidth, int screenHeight);
    void ApplyScissorRectForSpotLight(const D3DXVECTOR3& lightPos, const D3DXVECTOR3& lightDir, float radius, float angle, int screenWidth, int screenHeight);

    void DisableScissor();
    void RestorePreviousScissor();

    void DrawLightVolumeSphere(const D3DXVECTOR3& pos, float radius);
    void DrawLightVolumeCone(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, float length, float angle);
    void DrawFullscreenQuad();

    void EnableOptimizations(bool enable) { m_optimizationsEnabled = enable; }
    bool AreOptimizationsEnabled() const { return m_optimizationsEnabled; }

    void SetDebugDraw(bool debug) { m_debugDraw = debug; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    bool m_optimizationsEnabled;
    bool m_debugDraw;

    RECT m_savedScissorRect;
    bool m_scissorWasEnabled;

    void SaveViewportState();
    void RestoreViewportState();

    void ProjectSphereToScreen(const D3DXVECTOR3& center, float radius, int screenW, int screenH, RECT& outRect);
    void ProjectConeToScreen(const D3DXVECTOR3& origin, const D3DXVECTOR3& direction, float length, float angle, int screenW, int screenH, RECT& outRect);

    void CreateSphereMesh(float radius, int segments);
    void CreateConeMesh(float angle, int segments);
    void CreateQuadMesh();

    struct VolumeMesh {
        LPDIRECT3DVERTEXBUFFER9 vb;
        LPDIRECT3DINDEXBUFFER9 ib;
        int vertexCount;
        int indexCount;
    };

    VolumeMesh m_sphereMesh;
    VolumeMesh m_coneMesh;
    VolumeMesh m_quadMesh;

    LPDIRECT3DVERTEXDECLARATION9 m_volumeDecl;

    void ReleaseMesh(VolumeMesh& mesh);
};

}