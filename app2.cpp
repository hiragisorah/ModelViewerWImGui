#include "app2.h"
#include "assimp-converter.h"

namespace App2
{
	using namespace DirectX;

	Model model_;
	std::vector<XMMATRIX> matrices_;
	std::vector<XMMATRIX> matrices_anim_;

	void Initalize(void)
	{
		AssimpModel kaoru("xbot.fbx");
		Mesh mesh;

		auto cnt = kaoru.get_bone_cnt();

		std::vector<XMMATRIX> matrices;
		
		matrices.resize(cnt);

		for (unsigned int n = 0; n < cnt; ++n)
		{
			matrices[n] = kaoru.get_bone_matrix(n);
			//matrices[n] = kaoru.get_bone_offset_matrix(n) * kaoru.get_bone_matrix(n) * XMMatrixInverse(nullptr, kaoru.get_bone_offset_matrix(n));
		}

		int n = kaoru.GetBoneIdByName("mixamorig:RightArm");
		auto & mtx = matrices[n];
		mtx *= XMMatrixRotationY(-0.5f);

		//auto & mtx2 = matrices[kaoru.GetBoneIdByName("mixamorig:RightHand")];
		//mtx2 = XMMatrixRotationY(-0.5f) * mtx2;

		for (unsigned int n = 0; n < cnt; ++n)
		{
			auto matrix = XMMatrixInverse(nullptr, kaoru.get_bone_offset_matrix(n)) * matrices[n] * kaoru.get_bone_offset_matrix(n);

			int target = n;
			int id = 0;

			while (id != -1)
			{
				id = kaoru.get_bone_parent_id(target);


				if (id == target || id == -1)
					break;

				//std::cout << kaoru.get_bone_name(target) << " - " << id << std::endl;

				matrix = XMMatrixInverse(nullptr, kaoru.get_bone_offset_matrix(id)) * matrices[id] * kaoru.get_bone_offset_matrix(id) * matrix;
				target = id;
			}

			matrix = matrix;

			matrices_.emplace_back(matrix);
		}

		Graphics::SetupBonesAnim(matrices_);

		//
		model_.meshes_.resize(kaoru.get_mesh_cnt());

		for (unsigned int n = 0; n < kaoru.get_mesh_cnt(); ++n)
		{
			auto & mesh = model_.meshes_[n];

			for (unsigned int v = 0; v < kaoru.get_vtx_cnt(n); ++v)
			{
				auto & pos = kaoru.get_position(n, v);
				auto & norm = kaoru.get_normal(n, v);
				VertexBoneData bd;
				for (unsigned int b = 0; b < 4; ++b)
				{
					bd.ids_[b] = kaoru.get_bone_id(n, v, b);
					bd.weights_[b] = kaoru.get_bone_weight(n, v, b);
				}
				mesh.vertices_.emplace_back(XMFLOAT3(pos.x, pos.y, pos.z), XMFLOAT3(norm.x, norm.y, norm.z), XMFLOAT2(0, 0), bd);
			}

			for (unsigned int i = 0; i < kaoru.get_index_cnt(n); ++i)
			{
				auto & index = kaoru.get_index(n, i);
				mesh.indices_.emplace_back(index);
			}
		}

		model_.topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		
		model_.shader_ = "skinned";

		//

		Graphics::SetupModel(model_);
	}

	void Finalize(void)
	{
	}

	bool Run(void)
	{
		return true;
	}

}