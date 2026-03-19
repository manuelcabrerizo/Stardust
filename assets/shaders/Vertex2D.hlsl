#include "Common.hlsl"

PS_Input main(VS_Input i)
{
	float4 wPos = mul(float4(i.Pos, 1.0f), World);
	wPos = mul(wPos, Orthographic);

	PS_Input o;
	o.Pos = wPos;
	o.Nor = i.Nor;
	o.Uv = i.Uv;
	o.Color = Tint;
	return o;
}