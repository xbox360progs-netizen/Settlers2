//
// UIElement.fx
//
// Note: This effect file works with EffectEdit.
//

float4x3 g_WorldView : WORLDVIEW;
float4x4 g_WorldViewProj : WORLDVIEWPROJ;
float3 g_LightDir = {0.7f, -0.7f, 0.0f};
float4 g_Ambient = {0.2, 0.2, 0.2, 1.0};

struct VS_OUTPUT
{
    float4 Position         : POSITION;
    float4 Diffuse          : COLOR0;
};

VS_OUTPUT ShadeVertex(
    float4 Position         : POSITION,
    float3 Normal           : NORMAL0,
    float2 TexCoord         : TEXCOORD0,
    uniform float4 Diffuse
)
{
    float3 N;
    VS_OUTPUT Output;

    Output.Position = mul(Position, g_WorldViewProj);
    
    N = normalize(mul(Normal, (float3x3)g_WorldView));
    Output.Diffuse = saturate(dot(N, -g_LightDir)) * Diffuse + g_Ambient;

    return Output;
}

float4 ShadePixel(
    VS_OUTPUT Input
) : COLOR
{
    return Input.Diffuse;
}

technique Unselected
{
    pass p0
    {
        VertexShader = compile vs_2_0 ShadeVertex(float4(0.0f, 0.0f, 0.8f, 1.0f));
        PixelShader  = compile ps_2_0 ShadePixel();

        FillMode = Solid;
    }
}

technique Selected
{
    pass p0
    {
        VertexShader = compile vs_2_0 ShadeVertex(float4(0.6, 0.6, 1.0, 1.0));
        PixelShader  = compile ps_2_0 ShadePixel();

        FillMode = Solid;
    }
}
