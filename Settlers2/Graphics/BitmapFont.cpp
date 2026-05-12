#include "stdafx.h"
#include "BitmapFont.h"
#include "Renderer.h"
#include "ShaderManager.h"

// ==========================================================
// Constructor
// ==========================================================
BitmapFont::BitmapFont(LPDIRECT3DDEVICE9 device)
: m_device(device), m_texture(nullptr), m_vb(nullptr), m_vbCapacity(0),
  m_renderer(nullptr), m_shaderManager(nullptr),
  m_lineHeight(0.0f), m_textureWidth(0.0f), m_textureHeight(0.0f), m_baseLine(0.0f)
{
}

// ==========================================================
// Destructor
// ==========================================================
BitmapFont::~BitmapFont()
{
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
    if (m_vb) {
        m_vb->Release();
        m_vb = nullptr;
    }
}

// ==========================================================
// Initialize with renderer and shader manager
// ==========================================================
void BitmapFont::Init(Renderer* renderer, ShaderManager* shaderManager)
{
    m_renderer = renderer;
    m_shaderManager = shaderManager;
}

// ==========================================================
// Legacy Initialize method (deprecated)
// ==========================================================
bool BitmapFont::Initialize()
{
    // No-op - shaders are now managed by ShaderManager
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

        // Two triangles
        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);

        m_vertices.push_back(v1);
        m_vertices.push_back(v3);
        m_vertices.push_back(v2);

        penX += ch.xAdvance;
    }
}

void BitmapFont::DrawText(const std::string& text) { DrawText(text, 0xFFFFFFFF); }

// ==========================================================
// Legacy Render method (deprecated - use TextManager instead)
// ==========================================================
void BitmapFont::Render(const D3DXVECTOR3& origin,
                        const D3DXMATRIX& view,
                        const D3DXMATRIX& proj,
                        float scale,
                        bool mirrorX, bool mirrorY)
{
    // Legacy method - no-op with queue-based rendering
    // Text is now rendered via TextManager which submits to the queue
    OutputDebugStringA("[BitmapFont] WARNING: Render() is deprecated. Use TextManager for queue-based rendering.\n");
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
        D3DUSAGE_WRITEONLY,
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
    char debugBuf[256];
    sprintf(debugBuf, "[BitmapFont::LoadFromFile] Loading font: %ls\n", fontDefinitionFile);
    OutputDebugStringA(debugBuf);
    
    FILE* file = _wfopen(fontDefinitionFile,L"r");
    if (!file) {
        sprintf(debugBuf, "[BitmapFont::LoadFromFile] ERROR: Cannot open font file: %ls\n", fontDefinitionFile);
        OutputDebugStringA(debugBuf);
        return false;
    }
    
    sprintf(debugBuf, "[BitmapFont::LoadFromFile] Font file opened successfully\n");
    OutputDebugStringA(debugBuf);

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
                swprintf(textureFileName,511,L"game:\\Media\\Fonts\\%S",fileName);
                char debugBuf[512];
                sprintf(debugBuf, "[BitmapFont::LoadFromFile] Found texture file in .fnt: %s, full path: %ls\n", fileName, textureFileName);
                OutputDebugStringA(debugBuf);
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
    char debugBuf[256];
    sprintf(debugBuf, "[BitmapFont::LoadTextureFromMemory] Loading texture: %ls\n", texturePath);
    OutputDebugStringA(debugBuf);
    
    FILE* file=_wfopen(texturePath,L"rb");
    if (!file) {
        sprintf(debugBuf, "[BitmapFont::LoadTextureFromMemory] ERROR: Cannot open texture file: %ls\n", texturePath);
        OutputDebugStringA(debugBuf);
        return false;
    }
    fseek(file,0,SEEK_END);
    long size=ftell(file);
    fseek(file,0,SEEK_SET);
    unsigned char* buffer=new unsigned char[size];
    fread(buffer,1,size,file); fclose(file);

    HRESULT hr=D3DXCreateTextureFromFileInMemory(m_device,buffer,size,&m_texture);
    delete[] buffer;
    
    if (SUCCEEDED(hr)) {
        sprintf(debugBuf, "[BitmapFont::LoadTextureFromMemory] SUCCESS: Texture loaded, m_texture=0x%p\n", m_texture);
        OutputDebugStringA(debugBuf);
    } else {
        sprintf(debugBuf, "[BitmapFont::LoadTextureFromMemory] ERROR: Failed to load texture, hr=0x%08X\n", hr);
        OutputDebugStringA(debugBuf);
    }
    
    return SUCCEEDED(hr);
}
