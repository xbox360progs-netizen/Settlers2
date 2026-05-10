// Общие шейдерные функции и константы
// Используется совместно с другими шейдерами

// Общие константы
float4 g_time : register(c4);  // Время в секундах
float4 g_screenSize : register(c5);  // Размер экрана (x, y, 1/x, 1/y)

// Вспомогательные функции

// Преобразование из RGB в HSV
float3 RGBtoHSV(float3 rgb)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(rgb.bg, K.wz), float4(rgb.gb, K.xy), step(rgb.b, rgb.g));
    float4 q = lerp(float4(p.xyw, rgb.r), float4(rgb.r, p.yzx), step(p.x, rgb.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Преобразование из HSV в RGB
float3 HSVtoRGB(float3 hsv)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(hsv.xxx + K.xyz) * 6.0 - K.www);
    return hsv.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsv.y);
}

// Гамма-коррекция
float3 ApplyGamma(float3 color, float gamma)
{
    return pow(color, 1.0 / gamma);
}

// Инвертирование цвета
float3 InvertColor(float3 color)
{
    return 1.0 - color;
}

// Оттенки серого
float3 Grayscale(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

// Сепия
float3 Sepia(float3 color)
{
    float3 sepia = float3(1.2, 1.0, 0.8);
    return float3(
        dot(color, float3(0.393, 0.769, 0.189)),
        dot(color, float3(0.349, 0.686, 0.168)),
        dot(color, float3(0.272, 0.534, 0.131))
    ) * sepia;
}
