#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <d3dcompiler.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl\client.h>
#include <Windows.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;

struct BoneData
{
	int index_ = -1;
	DirectX::XMMATRIX init_;
};

struct VertexBoneData
{
	unsigned int ids_[4];
	float weights_[4];
};

struct Vertex
{
	Vertex(DirectX::XMFLOAT3 pos,
		DirectX::XMFLOAT3 norm,
		DirectX::XMFLOAT2 uv,
		VertexBoneData bone_data = {})
		: position_(pos)
		, normal_(norm)
		, texcoord_(uv)
		, bone_data_(bone_data)
	{}

	DirectX::XMFLOAT3 position_;
	DirectX::XMFLOAT3 normal_;
	DirectX::XMFLOAT2 texcoord_;

	VertexBoneData bone_data_;
};

struct Mesh
{
	std::string name_;
	std::vector<Vertex> vertices_;
	std::vector<unsigned int> indices_;
	std::string texture_;
};

struct Model
{
	std::vector<Mesh> meshes_;
	std::unordered_map<std::string, BoneData> bones_;
	D3D11_PRIMITIVE_TOPOLOGY topology_;
	std::string shader_;
};

struct RenderingObject
{
	struct InnerMesh
	{
		ComPtr<ID3D11Buffer> vertex_buffer_;
		ComPtr<ID3D11Buffer> index_buffer_;
		unsigned int index_cnt_ = 0;
	};

	std::string shader_;
	std::vector<InnerMesh> mesh_;
	D3D11_PRIMITIVE_TOPOLOGY topology_;
};

struct Animation
{
	float ticks_;
};

namespace Graphics
{
	void Initalize(void);
	bool Begin(void);
	bool End(void);
	void Finalize(void);

	ComPtr<ID3D11Device> & device(void);
	ComPtr<ID3D11DeviceContext> & context(void);

	void SetupModel(Model & model);
	void SetupBones(std::vector<DirectX::XMMATRIX> & bones);
	void SetupBonesAnim(std::vector<DirectX::XMMATRIX> & bones);

	void LoadShader(std::string path);
}