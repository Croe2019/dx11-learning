cbuffer CBPerFrame : register(b0)
{
    float4x4 gWorldViewProj;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 col : COLOR;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput o;
    o.pos = mul(float4(input.pos, 1.0f), gWorldViewProj);
    o.col = input.col;
    return o;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.col;
}
