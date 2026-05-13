// === Глобальные матрицы и параметры ===
float4x4 WorldViewProjection;
float4 MenuParams : register(c4);
float4 CenterParams : register(c5);

float4 InnerColor;
float4 OuterColor;
float4 HighlightColor;
float4 LineColor;
float4 CenterInnerColor;
float4 CenterOuterColor;

// === ИСПРАВЛЕНИЕ: Структура приведена к CommonVertex2D (stride=32) ===
// CommonVertex2D layout: float3 pos, float2 uv, D3DCOLOR color, float2 padding
struct VS_INPUT {
    float3 Pos   : POSITION;   // 12 bytes - matches D3DXVECTOR3
    float2 UV    : TEXCOORD0;  // 8 bytes - matches D3DXVECTOR2
    float4 Color : COLOR0;     // 4 bytes (D3DCOLOR) + 4 bytes padding
};

struct VS_OUTPUT {
    float4 Pos      : POSITION;
    float4 Color    : COLOR0;    // Передаем цвет в пиксельный шейдер
    float2 UV       : TEXCOORD0;
    float2 CenterUV : TEXCOORD1;
};

VS_OUTPUT MainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // Convert float3 to float4 for matrix multiplication (w = 1.0)
    float4 pos4 = float4(input.Pos, 1.0f);
    
    // Трансформируем позицию через переданную матрицу
    output.Pos = mul(pos4, WorldViewProjection);
    
    // Пробрасываем цвет вершины дальше
    output.Color = input.Color;
    
    output.UV = input.UV;
    
    // Переводим UV из диапазона [0, 1] в [-1, 1] для круговых расчетов
    output.CenterUV = input.UV * 2.0 - 1.0;
    
    return output;
}

float4 MainPS(VS_OUTPUT input) : COLOR0
{
    float2 pos = input.CenterUV;
    pos.y = -pos.y; // Инвертируем Y для корректного расчета углов atan2

    float dist = length(pos);
    float PI = 3.14159265;

    // --- Центральный круг меню ---
    if (dist <= CenterParams.x) {
        float centerGradient = saturate(dist / max(CenterParams.x, 0.0001));
        float4 centerColor = lerp(CenterInnerColor, CenterOuterColor, centerGradient);
        float centerEdge = 1.0 - smoothstep(CenterParams.x - CenterParams.y, CenterParams.x, dist);
        centerColor.a *= centerEdge;
        
        // Модулируем итоговый альфа-канал цветом из SpriteRenderer (например, для эффекта Fade)
        return centerColor * input.Color;
    }

    // --- Отсечение внешних и внутренних границ кольца ---
    if (dist < MenuParams.z || dist > MenuParams.w) {
        return float4(0, 0, 0, 0);
    }

    // --- Градиент и цвет секторов ---
    float gradientFactor = saturate((dist - MenuParams.z) / max(MenuParams.w - MenuParams.z, 0.0001));
    float4 sectorColor = lerp(InnerColor, OuterColor, gradientFactor);

    // --- Расчет текущего подсвеченного сектора ---
    float angle = atan2(pos.y, pos.x);
    float normAngle = frac((angle + PI * 0.5 + PI) / (2.0 * PI));
    float currentSector = floor(normAngle * MenuParams.x);
    float sectorMask = 1.0 - step(0.5, abs(currentSector - MenuParams.y));
    sectorColor = lerp(sectorColor, HighlightColor, sectorMask * 0.85);

    // --- Отрисовка разделительных линий и обводок ---
    float sectorPhase = frac(normAngle * MenuParams.x);
    float sectorLine = 1.0 - smoothstep(0.0, CenterParams.z, min(sectorPhase, 1.0 - sectorPhase));
    float innerLine = 1.0 - smoothstep(0.0, CenterParams.z * 1.5, abs(dist - MenuParams.z));
    float outerLine = 1.0 - smoothstep(0.0, CenterParams.z * 1.5, abs(dist - MenuParams.w));
    float centerRing = 1.0 - smoothstep(0.0, CenterParams.z * 1.5, abs(dist - CenterParams.x));

    // Внешнее свечение кольца меню
    float ringGlow = saturate(1.0 - smoothstep(MenuParams.w, MenuParams.w + CenterParams.w, dist));
    sectorColor.rgb += HighlightColor.rgb * ringGlow * 0.08;

    // Смешиваем цвет линий
    float lineMask = saturate(max(sectorLine, max(innerLine, max(outerLine, centerRing))));
    float4 finalColor = lerp(sectorColor, LineColor, lineMask * LineColor.a);
    
    // ВАЖНО: Умножаем финальный цвет на input.Color для поддержки прозрачности и модуляции цвета
    return finalColor * input.Color;
}

technique RadialMenu {
    pass P0 {
        VertexShader = compile vs_3_0 MainVS();
        PixelShader  = compile ps_3_0 MainPS();
        
        // Включаем аппаратное смешивание цветов (Alpha Blending)
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
    }
}