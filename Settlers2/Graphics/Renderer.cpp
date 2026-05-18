#include "stdafx.h"
#include "Renderer.h"
#include "Texture.h"
#include "SpriteRenderer.h"
#include <stdio.h>
#include <d3dx9.h>

Renderer::Renderer()  
    : m_pD3D(NULL), m_pDevice(NULL), m_pBackBuffer(NULL),
      m_pVertexDecl(NULL), m_pVertexShader(NULL), m_pPixelShader(NULL),
      m_pShaderManager(NULL), m_pSpriteRenderer(NULL) {
    ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
    ZeroMemory(m_projMatrix, sizeof(m_projMatrix));
}

Renderer::~Renderer() {
    Shutdown();
}

HRESULT Renderer::Initialize() {
    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D) { OutputDebugStringA("D3D Create Failed!\n"); return E_FAIL; }

    m_d3dpp.BackBufferWidth = 1280;
    m_d3dpp.BackBufferHeight = 720;
    m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    m_d3dpp.BackBufferCount = 1;
    m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.EnableAutoDepthStencil = FALSE;
    OutputDebugStringA("[R] Setting IMMEDIATE mode\n");
    
    m_d3dpp.BackBufferWidth = 1280;
    m_d3dpp.BackBufferHeight = 720;
    m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    m_d3dpp.BackBufferCount = 1;
    m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.EnableAutoDepthStencil = FALSE;
    m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    
    DWORD flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    HRESULT hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, flags, &m_d3dpp, &m_pDevice);
    if (FAILED(hr)) { OutputDebugStringA("Device Create Failed!\n"); return hr; }
    
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    // ShaderManager is now set externally via SetShaderManager()
    if (!m_pShaderManager) {
        OutputDebugStringA("[Renderer] WARNING: m_pShaderManager is NULL - call SetShaderManager() before Initialize()\n");
    }

    // Shaders are now loaded by GameEngine, not here
    
    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        D3DDECL_END()
    };
    HRESULT hrDecl = m_pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);
    if (FAILED(hrDecl)) {
        OutputDebugStringA("[Renderer] ERROR: CreateVertexDeclaration failed!\n");
        char errorMsg[256];
        sprintf_s(errorMsg, "[Renderer] CreateVertexDeclaration failed with HRESULT: 0x%08x\n", hrDecl);
        OutputDebugStringA(errorMsg);
    } else {
        OutputDebugStringA("Vertex declaration created OK\n");
    }

    // Shaders are now loaded by ShaderManager::Init() - no duplicate loading here

    SetProjectionMatrix(1280.0f, 720.0f);

    // Get the back buffer surface for dimensions
    hr = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    if (FAILED(hr)) {
        OutputDebugStringA("[Renderer] ERROR: GetBackBuffer failed!\n");
        return hr;
    }

    D3DSURFACE_DESC desc;
    m_pBackBuffer->GetDesc(&desc);

    // Position buffer (R32G32B32A32_FLOAT)
    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A32B32G32R32F, // Xbox 360: A32B32G32R32F ��� �������
        D3DMULTISAMPLE_NONE,
        0,
        TRUE,
        &m_pGBufferPos,
        NULL
    );
    if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Pos failed!\n"); goto cleanup; }

    // Normal buffer (A16B16G16R16F)
    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A16B16G16R16F,
        D3DMULTISAMPLE_NONE,
        0,
        TRUE,
        &m_pGBufferNormal,
        NULL
    );
    if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Normal failed!\n"); goto cleanup; }

    // Albedo buffer (A8R8G8B8)
    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        0,
        TRUE,
        &m_pGBufferAlbedo,
        NULL
    );
    if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Albedo failed!\n"); goto cleanup; }

    // Specular buffer (A8R8G8B8) - A = gloss, RGB = specular color
    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        0,
        TRUE,
        &m_pGBufferSpec,
        NULL
    );
    if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Specular failed!\n"); goto cleanup; }

    // Depth buffer (D24S8)
    hr = m_pDevice->CreateDepthStencilSurface(
        desc.Width, desc.Height,
        D3DFMT_D24S8,
        D3DMULTISAMPLE_NONE,
        0,
        TRUE,
        &m_pGBufferDepth,
        NULL
    );
    if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateDepthStencilSurface failed!\n"); goto cleanup; }

    // If we got here, all succeeded
    hr = S_OK;

    cleanup:
    if (FAILED(hr)) {
        // Clean up any successfully created resources
        if (m_pGBufferPos) { m_pGBufferPos->Release(); m_pGBufferPos = NULL; }
        if (m_pGBufferNormal) { m_pGBufferNormal->Release(); m_pGBufferNormal = NULL; }
        if (m_pGBufferAlbedo) { m_pGBufferAlbedo->Release(); m_pGBufferAlbedo = NULL; }
        if (m_pGBufferSpec) { m_pGBufferSpec->Release(); m_pGBufferSpec = NULL; }
        if (m_pGBufferDepth) { m_pGBufferDepth->Release(); m_pGBufferDepth = NULL; }
        if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
    }

m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

return S_OK;
}

void Renderer::SetSpriteRenderer(SpriteRenderer* pSpriteRenderer) {
    m_pSpriteRenderer = pSpriteRenderer;
	char buf[256];
    sprintf(buf, "[Renderer] SetSpriteRenderer: %p\n", pSpriteRenderer);
    OutputDebugStringA(buf);
}

HRESULT Renderer::LoadShader(ShaderID id, const char* filepath, const char* techniqueName) {
    if (!m_pShaderManager) return E_FAIL;
    return m_pShaderManager->LoadShader(id, filepath, techniqueName);
}

bool Renderer::SetShader(ShaderID id) {
    if (!m_pShaderManager) return false;
    return m_pShaderManager->SetActiveShader(id);
}

void Renderer::ResetToDefaultShader() {
    if (m_pShaderManager) {
        m_pShaderManager->SetActiveShader(SHADER_SPRITE_CONSTANT_INSTANCED);
    }
}

void Renderer::Setup2DRenderStates() {
    if (!m_pDevice) return;

    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

void Renderer::PrepareForUI() {
    if (!m_pDevice) return;

    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

void Renderer::RestoreFromUI() {
    if (!m_pDevice) return;

    m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
}

void Renderer::Shutdown() {
    // ShaderManager is now owned by GameEngine, don't shut it down here
    
    if (m_pVertexShader) { m_pVertexShader->Release(); m_pVertexShader = NULL; }
    if (m_pPixelShader) { m_pPixelShader->Release(); m_pPixelShader = NULL; }
    if (m_pVertexDecl) { m_pVertexDecl->Release(); m_pVertexDecl = NULL; }
    if (m_pGBufferPos) { m_pGBufferPos->Release(); m_pGBufferPos = NULL; }
    if (m_pGBufferNormal) { m_pGBufferNormal->Release(); m_pGBufferNormal = NULL; }
    if (m_pGBufferAlbedo) { m_pGBufferAlbedo->Release(); m_pGBufferAlbedo = NULL; }
    if (m_pGBufferSpec) { m_pGBufferSpec->Release(); m_pGBufferSpec = NULL; }
    if (m_pGBufferDepth) { m_pGBufferDepth->Release(); m_pGBufferDepth = NULL; }
    if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = NULL; }
    if (m_pD3D) { m_pD3D->Release(); m_pD3D = NULL; }
}

void Renderer::BeginFrame() {
    if (!m_pDevice) return;
    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    m_pDevice->BeginScene();
}

void Renderer::EndSceneOnly() {
    if (!m_pDevice) return;

    m_pDevice->EndScene();
    // Xbox 360: Present() is called in render thread, not here
}

void Renderer::EndFrame() {
    if (!m_pDevice) return;
    m_pDevice->EndScene();
    OutputDebugStringA("[Renderer::EndFrame] Calling Present...\n");
    m_pDevice->Present(NULL, NULL, NULL, NULL);
    OutputDebugStringA("[Renderer::EndFrame] Present done\n");
    // REMOVED: ResetVertexCount() is already called in ResetBatchState() inside SceneManager::Render()
    // Double reset causes issues with frame timing
}
        

void Renderer::Clear(D3DCOLOR color) {
    if (m_pDevice) m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
}

void Renderer::OnLostDevice() {
    if (m_pShaderManager) {
        m_pShaderManager->OnLostDevice();
    }
}

void Renderer::OnResetDevice() {
    if (m_pShaderManager) {
        m_pShaderManager->OnResetDevice();
    }

    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    SetProjectionMatrix(1280.0f, 720.0f);

        HRESULT hr;

        // Get the back buffer surface for dimensions
        hr = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
        if (FAILED(hr)) {
            OutputDebugStringA("[Renderer] ERROR: GetBackBuffer failed!\n");
            return;
        }

        D3DSURFACE_DESC desc;
        m_pBackBuffer->GetDesc(&desc);

        // Position buffer (R32G32B32A32_FLOAT)
        hr = m_pDevice->CreateRenderTarget(
            desc.Width, desc.Height,
            D3DFMT_A32B32G32R32F, // Xbox 360: A32B32G32R32F ��� �������
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            &m_pGBufferPos,
            NULL
        );
        if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Pos failed!\n"); goto cleanup; }

        // Normal buffer (A16B16G16R16F)
        hr = m_pDevice->CreateRenderTarget(
            desc.Width, desc.Height,
            D3DFMT_A16B16G16R16F,
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            &m_pGBufferNormal,
            NULL
        );
        if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Normal failed!\n"); goto cleanup; }

        // Albedo buffer (A8R8G8B8)
        hr = m_pDevice->CreateRenderTarget(
            desc.Width, desc.Height,
            D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            &m_pGBufferAlbedo,
            NULL
        );
        if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Albedo failed!\n"); goto cleanup; }

        // Specular buffer (A8R8G8B8) - A = gloss, RGB = specular color
        hr = m_pDevice->CreateRenderTarget(
            desc.Width, desc.Height,
            D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            &m_pGBufferSpec,
            NULL
        );
        if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateRenderTarget for Specular failed!\n"); goto cleanup; }

        // Depth buffer (D24S8)
        hr = m_pDevice->CreateDepthStencilSurface(
            desc.Width, desc.Height,
            D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            &m_pGBufferDepth,
            NULL
        );
        if (FAILED(hr)) { OutputDebugStringA("[Renderer] ERROR: CreateDepthStencilSurface failed!\n"); goto cleanup; }

        // If we got here, all succeeded
        hr = S_OK;

        cleanup:
        if (FAILED(hr)) {
            // Clean up any successfully created resources
            if (m_pGBufferPos) { m_pGBufferPos->Release(); m_pGBufferPos = NULL; }
            if (m_pGBufferNormal) { m_pGBufferNormal->Release(); m_pGBufferNormal = NULL; }
            if (m_pGBufferAlbedo) { m_pGBufferAlbedo->Release(); m_pGBufferAlbedo = NULL; }
            if (m_pGBufferSpec) { m_pGBufferSpec->Release(); m_pGBufferSpec = NULL; }
            if (m_pGBufferDepth) { m_pGBufferDepth->Release(); m_pGBufferDepth = NULL; }
            if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
        }
}

void Renderer::SetProjectionMatrix(float width, float height) {
    D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)m_projMatrix, 0.0f, width, height, 0.0f, 0.0f, 1.0f);
}

void Renderer::DrawSingleSprite(Texture* texture, float x, float y, float width, float height, D3DCOLOR color) {
    DrawSingleSprite(texture, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

void Renderer::DrawSingleSprite(Texture* texture, float x, float y, float width, float height,
                                float u0, float v0, float u1, float v1, D3DCOLOR color) {
    if (!m_pDevice || !texture || !texture->GetTexture()) {
        OutputDebugStringA("[Renderer] DrawSingleSprite: Invalid inputs\n");
        return;
    }

    static_assert(sizeof(SpriteVertex) == 32, "SpriteVertex size mismatch");

    LPDIRECT3DTEXTURE9 tex = texture->GetTexture();
    if (!tex) {
        OutputDebugStringA("[Renderer] ERROR: Texture is NULL\n");
        return;
    }

    SpriteVertex vertices[4];

    vertices[0].x = x; vertices[0].y = y; vertices[0].z = 0.0f;
    vertices[0].color = color; vertices[0].u = u0; vertices[0].v = v0;
    vertices[0].padding[0] = 0; vertices[0].padding[1] = 0;

    vertices[1].x = x + width; vertices[1].y = y; vertices[1].z = 0.0f;
    vertices[1].color = color; vertices[1].u = u1; vertices[1].v = v0;
    vertices[1].padding[0] = 0; vertices[1].padding[1] = 0;

    vertices[2].x = x + width; vertices[2].y = y + height; vertices[2].z = 0.0f;
    vertices[2].color = color; vertices[2].u = u1; vertices[2].v = v1;
    vertices[2].padding[0] = 0; vertices[2].padding[1] = 0;

    vertices[3].x = x; vertices[3].y = y + height; vertices[3].z = 0.0f;
    vertices[3].color = color; vertices[3].u = u0; vertices[3].v = v1;
    vertices[3].padding[0] = 0; vertices[3].padding[1] = 0;

    if (!m_pShaderManager) {
        OutputDebugStringA("[Renderer] DrawSingleSprite: m_pShaderManager is NULL\n");
        return;
    }

    ShaderManager::Shader* pShader = m_pShaderManager->GetShader(SHADER_SPRITE);
    if (!pShader || !pShader->pEffect || !m_pVertexDecl) {
        OutputDebugStringA("[Renderer] DrawSingleSprite: Shader/vertex decl is NULL\n");
        return;
    }

    m_pShaderManager->SetActiveShader(SHADER_SPRITE);
    m_pShaderManager->SetTexture("g_texture", tex);
    m_pShaderManager->BeginShader();
    m_pShaderManager->BeginPass(0);

    // Set WVP matrix
    D3DXMATRIX ortho;
    D3DXMatrixOrthoOffCenterLH(&ortho, 0, 1280, 720, 0, 0, 1);
    
    D3DXHANDLE hWVP = pShader->pEffect->GetParameterByName(NULL, "WVP");
    if (hWVP) {
        pShader->pEffect->SetMatrix(hWVP, &ortho);
    }
    
    D3DXHANDLE hMatOrtho = pShader->pEffect->GetParameterByName(NULL, "matOrtho");
    if (hMatOrtho) {
        pShader->pEffect->SetMatrix(hMatOrtho, &ortho);
    }

    pShader->pEffect->CommitChanges();

    m_pDevice->SetVertexDeclaration(m_pVertexDecl);
    m_pDevice->SetTexture(0, tex);
    m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(SpriteVertex));

    m_pShaderManager->EndPass();
    m_pShaderManager->EndShader();
}

void Renderer::BindGBuffer()
{
    if (!m_pDevice) return;

    m_pDevice->SetRenderTarget(0, m_pGBufferPos);
    m_pDevice->SetRenderTarget(1, m_pGBufferNormal);
    m_pDevice->SetRenderTarget(2, m_pGBufferAlbedo);
    m_pDevice->SetRenderTarget(3, m_pGBufferSpec);

    m_pDevice->SetDepthStencilSurface(m_pGBufferDepth);
}

void Renderer::UnbindGBuffer()
{
    if (!m_pDevice) return;

    m_pDevice->SetRenderTarget(0, NULL);
    m_pDevice->SetRenderTarget(1, NULL);
    m_pDevice->SetRenderTarget(2, NULL);
    m_pDevice->SetRenderTarget(3, NULL);

    m_pDevice->SetDepthStencilSurface(NULL);
}

void Renderer::ClearGBuffers()
{
    if (!m_pDevice) return;

    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

void Renderer::ApplyDeferredLighting()
{
    if (!m_pDevice) return;

    m_pDevice->SetRenderTarget(0, m_pBackBuffer);
    m_pDevice->SetRenderTarget(1, NULL);
    m_pDevice->SetRenderTarget(2, NULL);
    m_pDevice->SetRenderTarget(3, NULL);

    m_pDevice->SetDepthStencilSurface(NULL);

    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}


