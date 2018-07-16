#include "app2.h"
#include "assimp-converter.h"

AssimpModel * test = nullptr;

namespace App2
{
	using namespace DirectX;

	Model ReadModel(const AssimpModel & assimp_model)
	{
		Model model;
		Mesh mesh;

		auto mesh_cnt = assimp_model.get_mesh_cnt();

		model.meshes_.resize(assimp_model.get_mesh_cnt());

		for (unsigned int n = 0; n < mesh_cnt; ++n)
		{
			auto & mesh = model.meshes_[n];

			for (unsigned int v = 0; v < assimp_model.get_vtx_cnt(n); ++v)
			{
				auto & pos = assimp_model.get_position(n, v);
				auto & norm = assimp_model.get_normal(n, v);
				auto & uv = assimp_model.get_texcoord(n, v);
				VertexBoneData bd;
				for (unsigned int b = 0; b < 4; ++b)
				{
					bd.ids_[b] = assimp_model.get_bone_id(n, v, b);
					bd.weights_[b] = assimp_model.get_bone_weight(n, v, b);
				}
				mesh.vertices_.emplace_back(XMFLOAT3(pos.x, pos.y, pos.z), XMFLOAT3(norm.x, norm.y, norm.z), XMFLOAT2(uv.x, uv.y), bd);
			}

			mesh.transform_ = assimp_model.get_mesh_transformation(n);

			for (unsigned int i = 0; i < assimp_model.get_index_cnt(n); ++i)
			{
				auto & index = assimp_model.get_index(n, i);
				mesh.indices_.emplace_back(index);
			}
		}

		return model;
	}

	void CalcBoneFinalMatrix(const AssimpModel & assimp_model, std::vector<XMMATRIX> & bones, const XMMATRIX & parent_matrix = XMMatrixIdentity(), const XMMATRIX & reverse_parent_matrix = XMMatrixIdentity(), const int & x = 0)
	{
		auto matrix = assimp_model.get_bone_matrix(x);

		static float f = 0.f;

		//if (assimp_model.GetBoneIdByName("mixamorig:RightArm") == x)
		//{
		//	matrix = assimp_model.get_bone_final_offset_matrix(x);
		//	matrix *= XMMatrixRotationY(-1.5f);
		//	matrix *= XMMatrixInverse(nullptr, assimp_model.get_bone_final_offset_matrix(x));
		//	matrix *= assimp_model.get_bone_matrix(x);
		//}

		if (assimp_model.GetBoneIdByName("mixamorig:RightArm") == x)
		{
			matrix = assimp_model.get_bone_offset_matrix(x);
			matrix *= XMMatrixRotationY(f);
			matrix *= XMMatrixInverse(nullptr, assimp_model.get_bone_offset_matrix(x));
			matrix *= assimp_model.get_bone_matrix(x);
			bones[x] = matrix * reverse_parent_matrix;
		}
		else if (assimp_model.GetBoneIdByName("mixamorig:RightHand") == x)
		{
			matrix = assimp_model.get_bone_offset_matrix(x);
			matrix *= XMMatrixRotationY(f);
			matrix *= XMMatrixInverse(nullptr, assimp_model.get_bone_offset_matrix(x));
			matrix *= assimp_model.get_bone_matrix(x);
			bones[x] = matrix * reverse_parent_matrix;
		}
		else
		{
			bones[x] = parent_matrix * matrix;
		}

		f += 0.001f;

		 
		auto child_cnt = assimp_model.get_bone_child_cnt(x);
		for (auto n = 0U; n < child_cnt; ++n)
		{
			auto child_id = assimp_model.get_bone_child_id(x, n);
			CalcBoneFinalMatrix(assimp_model, bones, parent_matrix * matrix, matrix * reverse_parent_matrix, child_id);
		}

	}

	void ShowStaticModel(const AssimpModel & assimp_model)
	{
		auto model = ReadModel(assimp_model);

		model.topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		model.shader_ = "test";

		Graphics::SetupModel(model);
	}

	void ShowSkinnedModel(const AssimpModel & assimp_model)
	{
		auto model = ReadModel(assimp_model);

		model.topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		model.shader_ = "skinned";

		std::vector<XMMATRIX> bones;

		auto bone_cnt = assimp_model.get_bone_cnt();
		bones.resize(bone_cnt);

		CalcBoneFinalMatrix(assimp_model, bones);

		Graphics::SetupModel(model);
		Graphics::SetupBonesAnim(bones);
	}

	void AddPosition(const AssimpModel & assimp_model, Mesh & mesh, CXMMATRIX & parent_matrix = XMMatrixIdentity(), const int & x = 0)
	{
		auto matrix = assimp_model.get_bone_matrix(x);

		if (assimp_model.GetBoneIdByName("mixamorig:RightArm") == x)
		{
			matrix = XMMatrixRotationY(-1.5f) * matrix;
		}

		//if (assimp_model.GetBoneIdByName("mixamorig:RightHand") == x)
		//{
		//	matrix = matrix * XMMatrixRotationY(-1.5f);
		//}

		XMMATRIX final_matrix = matrix * parent_matrix;

		if (x != 0)
		{
			auto & parent_pos = parent_matrix.r[3];
			mesh.vertices_.emplace_back(XMFLOAT3(XMVectorGetX(parent_pos), XMVectorGetY(parent_pos), XMVectorGetZ(parent_pos)), XMFLOAT3(0, 0, 0), XMFLOAT2(0, 0));

			auto & pos = final_matrix.r[3];
			mesh.vertices_.emplace_back(XMFLOAT3(XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos)), XMFLOAT3(0, 0, 0), XMFLOAT2(0, 0));
		}

		auto child_cnt = assimp_model.get_bone_child_cnt(x);
		for (auto n = 0U; n < child_cnt; ++n)
		{
			auto child_id = assimp_model.get_bone_child_id(x, n);
			AddPosition(assimp_model, mesh, final_matrix, child_id);
		}
	}

	void ShowBone(const AssimpModel & assimp_model)
	{
		Model model;
		Mesh mesh;

		model.topology_ = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

		model.shader_ = "test";

		AddPosition(assimp_model, mesh);

		mesh.indices_.resize(mesh.vertices_.size());

		auto index_cnt = mesh.indices_.size();

		for (auto n = 0U; n < index_cnt; ++n)
			mesh.indices_[n] = n;

		model.meshes_.emplace_back(mesh);

		Graphics::SetupModel(model);
	}

	void Initalize(void)
	{
		test = new AssimpModel("boxing.fbx");
		//ShowStaticModel(test);
		ShowBone(*test);
		ShowSkinnedModel(*test);
	}

	void Finalize(void)
	{
		delete test;
	}

	bool Run(void)
	{
		std::vector<XMMATRIX> bones;

		auto bone_cnt = test->get_bone_cnt();
		bones.resize(bone_cnt);

		CalcBoneFinalMatrix(*test, bones);

		for (auto n = 0U; n < bone_cnt; ++n)
			bones[n] = bones[n] * test->get_bone_offset_matrix(n);

		Graphics::SetupBonesAnim(bones);

		return true;
	}

}