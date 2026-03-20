#include "Common.hlsl"

float4 main(PS_Input i) : SV_TARGET
{
    float4 textureColor = texture0.Sample(linearWrapSamplerState, i.Uv);
    return textureColor * float4(i.Color, 1.0f);
}