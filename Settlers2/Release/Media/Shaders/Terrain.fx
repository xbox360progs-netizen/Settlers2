// Terrain.fx
// Shader for terrain layer (isometric tilemap)

sampler2D g_TileMap : register(s0);
sampler2D g_Atlas   : register(s1);
float4x4  matVP     : register(c0);
float2    mapSize   : register(c4);
static const float2 TILE_SIZE = float2(119.0f, 72.0f);

struct VS_OUT {
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

VS_OUT MainVS(int vertexID : INDEX) {
    VS_OUT Out;
    int tileID = vertexID / 4;
    int x = tileID % (int)mapSize.x;
    int y = tileID / (int)mapSize.x;

    float2 uv = (float2(x, y) + 0.5f) / mapSize;
    float4 tileData = tex2Dlod(g_TileMap, float4(uv.x, uv.y, 0.0f, 0.0f));
    float tileType = tileData.r * 255.0f;
    float height = tileData.g * 255.0f;

    float isoX = (x - y) * TILE_SIZE.x * 0.5f;
    float isoY = (x + y) * TILE_SIZE.y * 0.5f - height;
    int cornerID = vertexID % 4;
    float2 corner = float2((cornerID % 2) - 0.5f, (cornerID < 2) - 0.5f);
    float2 worldPos = float2(isoX, isoY) + (corner * TILE_SIZE);

    float depth = (x + y) / (mapSize.x + mapSize.y);
    Out.pos = mul(float4(worldPos, depth, 1.0f), matVP);
    Out.uv = float2(tileType * 0.125f, 0.5f);
    Out.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return Out;
}

float4 MainPS(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR {
    return tex2D(g_Atlas, uv) * color;
}

technique TerrainTech {
    pass P0 {
        VertexShader = compile vs_3_0 MainVS();
        PixelShader  = compile ps_3_0 MainPS();
    }
}
