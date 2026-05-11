//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
float4x4 matWVP;
float4 TileUV; // x=u0, y=v0, z=u1, w=v1

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
texture AtlasTexture;

sampler AtlasSampler = sampler_state
{
    Texture = <AtlasTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(VS_INPUT Input)
{
    VS_OUTPUT Output;
    // Screen-space rendering: no ViewProj transformation for UI
    Output.Position = float4(Input.Position, 1.0);
    Output.TexCoord = Input.TexCoord;
    return Output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT Input) : COLOR0
{
    // Преобразуем UV координаты ячейки в UV атласа
    float2 atlasUV;
    atlasUV.x = lerp(TileUV.x, TileUV.z, Input.TexCoord.x);
    atlasUV.y = lerp(TileUV.y, TileUV.w, Input.TexCoord.y);
    
    // Возвращаем с альфой, чтобы видеть настоящие иконки
    return tex2D(AtlasSampler, atlasUV);
}