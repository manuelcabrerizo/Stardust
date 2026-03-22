#include "Common2D.hlsl"

PS_Input main(VS_Input i)
{
	float4 wPos = mul(float4(i.Pos, 1.0f), Orthographic);
	PS_Input o;
	o.Pos = wPos;
	o.Color = i.Color;
	o.Uv = i.Uv;
	return o;
}