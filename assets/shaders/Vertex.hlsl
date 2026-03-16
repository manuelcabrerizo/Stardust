#include "Common.hlsl"

PS_Input main(VS_Input i)
{
	float4 wPos = mul(float4(i.Pos, 1.0f), World);
	wPos = mul(wPos, View);
	wPos = mul(wPos, Proj);

	PS_Input o;
	o.Pos = wPos;
	o.Uv = i.Uv;
	o.Color = Tint;
	return o;
}