// SpriteObject.fx - Complete Fixed Version

texture g_DataTex;
texture g_Atlas;
float4x4 matVP : register(c0);
static const float2 TILE_SIZE = float2(119.0f, 72.0f);

sampler DataSampler = sampler_state {
    Texture = <g_DataTex>;
    MinFilter = Point; 
    MagFilter = Point;
    MipFilter = None;
};

sampler AtlasSampler = sampler_state {
    Texture = <g_Atlas>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_OUT {
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

// Updated to use the sampler and removed the sampler2D parameter
float4 FetchReg(int spriteID, int reg) {
    float u = (spriteID * 4 + reg) + 0.5f;
    return tex2Dlod(DataSampler, float4(u / 16384.0f, 0.0f, 0.0f, 0.0f));
}

VS_OUT MainVS(int vertexID : INDEX) {
    VS_OUT Out;
    int spriteID = vertexID / 6;
    int v = vertexID % 6;
    
    // Triangle list corner mapping
    int cornerID = 0;
    if (v == 0) cornerID = 0;
    else if (v == 1) cornerID = 1;
    else if (v == 2) cornerID = 2;
    else if (v == 3) cornerID = 2;
    else if (v == 4) cornerID = 1;
    else cornerID = 3;

    float4 dataPos = FetchReg(spriteID, 0);
    float4 dataUV  = FetchReg(spriteID, 1);
    float4 dataPrp = FetchReg(spriteID, 2);
    float4 dataCol = FetchReg(spriteID, 3);

    float2 gridPos = dataPos.xy;
    float2 size    = dataPos.zw;
    
    float2 isoPos;
    isoPos.x = (gridPos.x - gridPos.y) * TILE_SIZE.x * 0.5f;
    isoPos.y = (gridPos.x + gridPos.y) * TILE_SIZE.y * 0.5f;

    float2 corner = float2((cornerID % 2) - 0.5f, (cornerID < 2) - 0.5f);
    
    float s, c;
    sincos(dataPrp.z /*rot*/, s, c);
    float2 rotatedCorner = float2(corner.x * c - corner.y * s, corner.x * s + corner.y * c);
    float2 worldPos = isoPos + (rotatedCorner * size);

    Out.pos = mul(float4(worldPos, dataPrp.x /*depth*/, 1.0f), matVP);
    Out.uv = dataUV.xy + (corner + 0.5f) * dataUV.zw;
    Out.color = dataCol * dataPrp.w /*opacity*/;
    
    return Out;
}

float4 MainPS(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR {
    return tex2D(AtlasSampler, uv) * color;
}

technique SpriteObjectTech {
    pass P0 {
        VertexShader = compile vs_3_0 MainVS();
        PixelShader  = compile ps_3_0 MainPS();
    }
}