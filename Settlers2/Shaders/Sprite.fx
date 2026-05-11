//--------------------------------------------------------------------------------------
// Sprite.fx - Legacy sprite shader for backward compatibility
// Xbox 360 compatible: uses vs_2_0/ps_2_0 shader profiles
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
float4x4 matOrtho; // Orthographic or ViewProj matrix

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
texture g_texture;

sampler texSampler = sampler_state
{
    Texture = <g_texture>;
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
    float4 Position : POSITION;  // float4 for Xbox 360 alignment (W=1.0f auto-filled)
    float4 Color : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Position : POSITION;
    float4 Color : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS_Main(VS_INPUT Input)
{
    VS_OUTPUT Output;
    // Transform position by matrix (can be ortho for UI or ViewProj for world)
    Output.Position = mul(Input.Position, matOrtho);
    Output.Color = Input.Color;
    Output.TexCoord = Input.TexCoord;
    return Output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS_Main(VS_OUTPUT Input) : COLOR0
{
    return tex2D(texSampler, Input.TexCoord) * Input.Color;
}

//--------------------------------------------------------------------------------------
// Technique - Renamed to SpriteBatchTech for ShaderManager compatibility
// Xbox 360: uses vs_2_0/ps_2_0 profiles (not vs_3_0/ps_3_0)
//--------------------------------------------------------------------------------------
technique SpriteBatchTech
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS_Main();
        PixelShader = compile ps_2_0 PS_Main();
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
    }
}
