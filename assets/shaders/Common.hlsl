#pragma pack_matrix(row_major)

#ifdef SPIRV
    #define VK_BINDING(b, s) [[vk::binding(b, s)]]
#else
    #define VK_BINDING(b, s) 
#endif

struct VS_Input
{
	float3 Pos : POSITION;
	float3 Nor : NORMAL;
	float2 Uv  : TEXCOORD0;
};

struct PS_Input
{
	float4 Pos   : SV_POSITION;
	float3 Nor   : NORMAL;
	float2 Uv    : TEXCOORD0;
	float3 Color : TEXCOORD1;
};

// Vulkan set 0 per frame data
VK_BINDING(0, 0) cbuffer PerFrame : register(b0)
{
	float4x4 View;
	float4x4 Perspective;
	float4x4 Orthographic;
	float Time;
};

// Vulkan set 1 per draw-object data
VK_BINDING(0, 1) cbuffer PerDraw : register(b1)
{
	float4x4 World;
	float3 Tint;
};

// Vulkan set 2 static samplers
VK_BINDING(0, 2) SamplerState linearClampSamplerState : register(s0);
VK_BINDING(1, 2) SamplerState linearWrapSamplerState : register(s1);
VK_BINDING(2, 2) SamplerState pointClampSamplerState : register(s2);
VK_BINDING(3, 2) SamplerState pointWrapSamplerState : register(s3);

// Vulkan set 3 textrue bindings
VK_BINDING(0, 3) Texture2D texture0 : register(t0);
VK_BINDING(1, 3) Texture2D texture1 : register(t1);
VK_BINDING(2, 3) Texture2D texture2 : register(t2);
VK_BINDING(3, 3) Texture2D texture3 : register(t3);