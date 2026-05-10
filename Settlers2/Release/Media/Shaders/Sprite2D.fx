// Шейдер для 2D спрайтов
// Поддерживает позицию, цвет, UV координаты

// Vertex Shader входные данные
struct VS_INPUT {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

// Vertex Shader выходные данные
struct VS_OUTPUT {
    float4 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

// Матрица трансформации (World * View * Projection)
float4x4 matWVP : register(c0);

// Vertex Shader
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // Трансформируем позицию
    output.position = mul(float4(input.position, 1.0f), matWVP);
    
    // Передаем цвет и UV без изменений
    output.color = input.color;
    output.texCoord = input.texCoord;
    
    return output;
}

// Pixel Shader входные данные
struct PS_INPUT {
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

// Текстура спрайта
texture spriteTexture;
sampler2D spriteSampler = sampler_state
{
    Texture = <spriteTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Pixel Shader
float4 PS(PS_INPUT input) : COLOR
{
    // Сэмплируем текстуру
    float4 texColor = tex2D(spriteSampler, input.texCoord);
    
    // Применяем альфа-блендинг (умножаем цвет вершины на цвет текстуры)
    float4 finalColor = texColor * input.color;
    
    return finalColor;
}

// Техника
technique Sprite2D
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS();
        PixelShader = compile ps_2_0 PS();
    }
}
