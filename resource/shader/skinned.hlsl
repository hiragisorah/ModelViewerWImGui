Texture2D tex : register(t0);

SamplerState own_sampler : register(s0);

cbuffer unique : register(b0)
{
	row_major matrix g_world;
	row_major matrix g_view;
	row_major matrix g_proj;
};

cbuffer bone_buffer : register(b1)
{
	row_major matrix bones_[255];
}

//cbuffer camera : register(b1)
//{
//    row_major matrix g_v : packoffset(c0);
//    row_major matrix g_p : packoffset(c4);

//    float4 g_eye : packoffset(c8);
//    float4 g_at : packoffset(c9);

//    float2 g_view_port : packoffset(c10);

//    float4 g_color : packoffset(c11);
//};

struct VsIn
{
	float3 position_ : POSITION;
	float3 normal_ : NORMAL;
	float2 uv_ : TEXCOORD;
	uint4  Bones : BONE_INDEX;//ボーンのインデックス
	float4 Weights : BONE_WEIGHT;//ボーンの重み
};

struct VsOut
{
	float4 sv_position_ : SV_Position;
	float4 position_ : TEXCOORD0;
	float4 normal_ : TEXCOORD1;
	float2 uv_ : TEXCOORD2;
};

struct PsOut
{
	float4 color_ : SV_Target0;
};

//スキニング後の頂点・法線が入る
struct Skin
{
	float4 position_;
	float3 normal_;
};

//
// Skin SkinVert( VSSkinIn Input )
//頂点をスキニング（ボーンにより移動）する。サブ関数（バーテックスシェーダーで使用）
Skin SkinVert(VsIn Input)
{
	Skin Output = (Skin)0;

	float4 position_ = float4(Input.position_, 1);
	float3 normal_ = Input.normal_;

	//ボーン0
	uint iBone = Input.Bones.x;
	float fWeight = Input.Weights.x;
	matrix m = bones_[iBone];
	Output.position_ += fWeight * mul(position_, m);
	Output.normal_ += fWeight * mul(normal_, (float3x3)m);
	//ボーン1
	iBone = Input.Bones.y;
	fWeight = Input.Weights.y;
	m = bones_[iBone];
	Output.position_ += fWeight * mul(position_, m);
	Output.normal_ += fWeight * mul(normal_, (float3x3)m);
	//ボーン2
	iBone = Input.Bones.z;
	fWeight = Input.Weights.z;
	m = bones_[iBone];
	Output.position_ += fWeight * mul(position_, m);
	Output.normal_ += fWeight * mul(normal_, (float3x3)m);
	//ボーン3
	iBone = Input.Bones.w;
	fWeight = Input.Weights.w;
	m = bones_[iBone];
	Output.position_ += fWeight * mul(position_, m);
	Output.normal_ += fWeight * mul(normal_, (float3x3)m);

	return Output;
}

VsOut VS(VsIn input)
{
	VsOut output = (VsOut)0;

	Skin skinned = SkinVert(input);

	input.position_ = skinned.position_.xyz;

	matrix wvp = mul(g_world, mul(g_view, g_proj));

	output.sv_position_ = mul(float4(input.position_, 1), wvp);
	output.position_ = mul(float4(input.position_, 1), g_world);
	output.normal_.xyz = normalize(mul(float4(input.normal_, 1), (float3x3) g_world));
	output.uv_ = input.uv_;

	return output;
}

PsOut PS(VsOut input)
{
	PsOut output = (PsOut)0;

	output.color_ = tex.Sample(own_sampler, input.uv_);
	output.color_ = input.normal_;

	return output;
}