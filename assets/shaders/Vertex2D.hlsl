#include "Common.hlsl"

PS_Input main(VS_Input i)
{
	float4 wPos = mul(float4(i.Pos, 1.0f), Orthographic);
	PS_Input o;
	o.Pos = wPos;
	o.Nor = i.Nor;
	o.Uv = i.Uv;
	o.Color = Tint;
	return o;
}