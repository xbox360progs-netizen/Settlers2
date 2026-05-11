// Graphics/BitmapFont.cpp
#include "stdafx.h"
#include "BitmapFont.h"
#include "TextVertex.h"
#include <cstdio>
#include <cstring>

#define COLOR_WHITE 0xFFFFFFFF

// ==========================================================
// SHADERS
// ==========================================================

static const char* VS_FONT =
"float4x4 gWVP : register(c0);"
"struct VS_IN  { float3 pos:POSITION; float4 col:COLOR0; float2 uv:TEXCOORD0; };"
"struct VS_OUT { float4 pos:POSITION; float4 col:COLOR0; float2 uv:TEXCOORD0; };"
"VS_OUT main(VS_IN v){"
" VS_OUT o;"
" o.pos = mul(gWVP, float4(v.pos,1));"
" o.col = v.col;"
" o.uv  = v.uv;"
" return o;"
"}";


static const char* PS_FONT =
"sampler2D s0;"
"float4 main(float4 c:COLOR0, float2 uv:TEXCOORD0):COLOR {"
" return tex2D(s0, uv) * c;"
"}";

// ==========================================================
// CTOR / DTOR
// ==========================================================

BitmapFont::BitmapFont(LPDIRECT3DDEVICE9 device)
    : m_device(device), m_texture(nullptr), m_vb(nullptr), m_vbCapacity(0),
      m_vs(nullptr), m_ps(nullptr), m_decl(nullptr), m_lineHeight(0),
      m_textureWidth(0), m_textureHeight(0)
{}

BitmapFont::~BitmapFont()
{
    if (m_vb)      m_vb->Release();
    if (m_texture) m_texture->Release();
    if (m_vs)      m_vs->Release();
	if (m_vsWorld) m_vsWorld->Release();
    if (m_ps)      m_ps->Release();
    if (m_decl)    m_decl->Release();
}

// ==========================================================
// INIT
// ==========================================================

bool BitmapFont::Initialize()
{
    if (!m_device) return false;

    D3DVERTEXELEMENT9 decl[] =
    {
        {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
        {0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    if (FAILED(m_device->CreateVertexDeclaration(decl, &m_decl))) return false;

    // ===== 2D VERTEX SHADER =====
	LPD3DXBUFFER code = nullptr;

	if (FAILED(D3DXCompileShader(VS_FONT, strlen(VS_FONT),
		0, 0, "main", "vs_2_0", 0, &code, 0, 0)))
		return false;

	m_device->CreateVertexShader(
		(DWORD*)code->GetBufferPointer(), &m_vs);

	code->Release();    

    // Пиксельный шейдер (общий)
    if (FAILED(D3DXCompileShader(PS_FONT, strlen(PS_FONT), 0, 0, "main", "ps_2_0", 0, &code, 0, 0))) return false;
    m_device->CreatePixelShader((DWORD*)code->GetBufferPointer(), &m_ps);
    code->Release();

//    Logger::Print("[BitmapFont] Shaders initialized.\n");
    return true;
}

// ==========================================================
// CPU: BUILD GLYPHS (LOCAL SPACE)
// ==========================================================

void BitmapFont::Begin()
{
    m_vertices.clear();
}

void BitmapFont::DrawText(const std::string& text, DWORD color)
{
    float penX = 0.0f;
    float penY = 0.0f;

    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = (unsigned char)text[i];
        // New line handling to avoid bogus glyphs
        if (c == '\n') { penX = 0.0f; penY += m_lineHeight; continue; }
        if (c == '\r') continue;
        if (c >= m_chars.size()) {
            continue;
        }
        const FontChar& ch = m_chars[c];
        if (ch.width <= 0.0f || ch.height <= 0.0f) { penX += ch.xAdvance; continue; }

        float x0 = penX + ch.xOffset;
        float y0 = penY + ch.yOffset;
        float x1 = x0 + ch.width;
        float y1 = y0 + ch.height;

        TextVertex v0; v0.pos = D3DXVECTOR3(x0, y0, 0); v0.col = color; v0.uv = D3DXVECTOR2(ch.u0, ch.v0);
        TextVertex v1; v1.pos = D3DXVECTOR3(x1, y0, 0); v1.col = color; v1.uv = D3DXVECTOR2(ch.u1, ch.v0);
        TextVertex v2; v2.pos = D3DXVECTOR3(x0, y1, 0); v2.col = color; v2.uv = D3DXVECTOR2(ch.u0, ch.v1);
        TextVertex v3; v3.pos = D3DXVECTOR3(x1, y1, 0); v3.col = color; v3.uv = D3DXVECTOR2(ch.u1, ch.v1);

        // Два треугольника
        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);

        m_vertices.push_back(v1);
        m_vertices.push_back(v3);
        m_vertices.push_back(v2);

        penX += ch.xAdvance;
    }
}

void BitmapFont::DrawText(const std::string& text) { DrawText(text, COLOR_WHITE); }

// ==========================================================
// GPU DRAW
// ==========================================================

void BitmapFont::Render(const D3DXVECTOR3& origin,
                        const D3DXMATRIX& view,
                        const D3DXMATRIX& proj,
                        float scale,
                        bool mirrorX, bool mirrorY)
{
    if (!m_device || m_vertices.empty())
        return;

    if (!m_texture || !m_decl || !m_vs || !m_ps)
    {
        return;
    }

    EnsureVB(m_vertices.size());

    TextVertex* vb = nullptr;
    if (FAILED(m_vb->Lock(0,
        sizeof(TextVertex) * m_vertices.size(),
        (void**)&vb, 0)))
        return;

    memcpy(vb, m_vertices.data(),
        sizeof(TextVertex) * m_vertices.size());

    m_vb->Unlock();

    // ==============================
    // WORLD MATRIX (как у спрайта)
    // ==============================

    D3DXMATRIX matScale, matTrans, matWorld;

    float sx = mirrorX ? -scale : scale;
    float sy = mirrorY ? -scale : scale;

    D3DXMatrixScaling(&matScale, sx, sy, 1.0f);
    D3DXMatrixTranslation(&matTrans,
        origin.x,
        origin.y,
        origin.z);

    matWorld = matScale;
	matWorld *= matTrans;

    // ==============================
    // WVP как в Renderer
    // ==============================

    D3DXMATRIX matWVP = matWorld * view * proj;

    m_device->SetVertexShaderConstantF(0,
        (float*)&matWVP, 4);

    // ==============================
    // GPU STATE
    // ==============================

    m_device->SetVertexDeclaration(m_decl);
    m_device->SetVertexShader(m_vs);
    m_device->SetPixelShader(m_ps);

    m_device->SetTexture(0, m_texture);
    m_device->SetStreamSource(0, m_vb, 0, sizeof(TextVertex));

    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    m_device->DrawPrimitive(
        D3DPT_TRIANGLELIST,
        0,
        m_vertices.size() / 3);

    
}

// ==========================================================
// VB MANAGEMENT
// ==========================================================

void BitmapFont::EnsureVB(size_t vertexCount)
{
    if (!m_device)
    {
        return;
    }

    if (vertexCount <= m_vbCapacity) return;
    if (m_vb) m_vb->Release();

    m_vbCapacity = (UINT)vertexCount;
    
    HRESULT hr = m_device->CreateVertexBuffer(
        sizeof(TextVertex) * m_vbCapacity,
        D3DUSAGE_WRITEONLY,  // Только WRITEONLY
        0,
        D3DPOOL_DEFAULT,
        &m_vb,
        nullptr
    );
    
    if (FAILED(hr)) {
        m_vb = nullptr;
        m_vbCapacity = 0;
    }
}

// ==========================================================
// LOAD FONT FILE
// ==========================================================

bool BitmapFont::LoadFromFile(const wchar_t* fontDefinitionFile)
{
    FILE* file = _wfopen(fontDefinitionFile,L"r");
    if (!file) return false;

    char line[512];
    wchar_t textureFileName[512]={0};

    m_chars.clear();
    m_chars.resize(256);
    m_lineHeight = 120.0f;
    int charCount=0;

    while (fgets(line,sizeof(line),file))
    {
        line[strcspn(line,"\r\n")]=0;
        if (strlen(line)==0 || line[0]=='#') continue;

        if (strstr(line,"page ")!=nullptr)
        {
            char fileName[256];
            if (sscanf(line,"page id=%*d file=\"%255[^\"]\"",&fileName)==1)
            {
                swprintf(textureFileName,511,L"game:\\Media\\fonts\\%S",fileName);
            }
        }
		else if (strstr(line,"common ")!=nullptr)
		{
			int lh,base,sw,sh;
			if (sscanf(line,"common lineHeight=%d base=%d scaleW=%d scaleH=%d",&lh,&base,&sw,&sh)>=4)
			{
				m_lineHeight=(float)lh;
				m_baseLine=(float)base; 
				m_textureWidth=(float)sw;
				m_textureHeight=(float)sh;
			}
		}
        else if (strstr(line,"char ")==line)
		{
			int id,x,y,w,h,xo,yo,xa;

			if (sscanf(line,"char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d",
				&id,&x,&y,&w,&h,&xo,&yo,&xa)>=8 && id>=0 && id<256)
			{
				FontChar& ch=m_chars[id];
				ch.u0=(float)x/m_textureWidth;
				ch.v0=(float)y/m_textureHeight;
				ch.u1=(float)(x+w)/m_textureWidth;
				ch.v1=(float)(y+h)/m_textureHeight;

				ch.width=(float)w; ch.height=(float)h;
				ch.xOffset=(float)xo; ch.yOffset=(float)yo;
				ch.xAdvance=(float)xa;
				charCount++;
			}
		}
    }
    fclose(file);
    if (wcslen(textureFileName)>0) return LoadTextureFromMemory(textureFileName);
    return false;
}

bool BitmapFont::LoadTextureFromMemory(const wchar_t* texturePath)
{
    FILE* file=_wfopen(texturePath,L"rb");
    if (!file) return false;
    fseek(file,0,SEEK_END);
    long size=ftell(file);
    fseek(file,0,SEEK_SET);
    unsigned char* buffer=new unsigned char[size];
    fread(buffer,1,size,file); fclose(file);

    HRESULT hr=D3DXCreateTextureFromFileInMemory(m_device,buffer,size,&m_texture);
    delete[] buffer;
    return SUCCEEDED(hr);
}


