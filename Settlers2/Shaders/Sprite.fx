//--------------------------------------------------------------------------------------
// Sprite.fx - Legacy sprite shader for backward compatibility
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
    float3 Position : POSITION;
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
    Output.Position = mul(float4(Input.Position, 1.0f), matOrtho);
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
// Technique
//--------------------------------------------------------------------------------------
technique Sprite
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Main();
        PixelShader = compile ps_3_0 PS_Main();
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
    }
}
