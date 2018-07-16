#include "assimp-converter.h"
#include <algorithm>

using namespace DirectX;

XMMATRIX aiMatrix4x42XMMATRIX(aiMatrix4x4 & m)
{
	m.Transpose();
	return XMLoadFloat4x4(&XMFLOAT4X4(&m.a1));
}

AssimpModel::AssimpModel(std::string file_name)
{
	if (!Init(file_name)) return;

	auto & root = this->importer_.GetScene()->mRootNode;

	this->ProcessMaterials();
	this->ProcessNode(root);
	this->UpdateBone();
}

bool AssimpModel::Init(std::string file_name)
{
	this->importer_.FreeScene();
	
	this->importer_.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene * scene = this->importer_.ReadFile(file_name
		, aiPostProcessSteps::aiProcess_Triangulate // 三角化（TriangleStripのため）
		| aiPostProcessSteps::aiProcess_MakeLeftHanded // 左手座標（DirectXに合わせる）
		| aiPostProcessSteps::aiProcess_CalcTangentSpace // よくわからん
		| aiPostProcessSteps::aiProcess_FlipUVs // UV反転（DirectXに合わせる）
		| aiPostProcessSteps::aiProcess_FlipWindingOrder // ポリゴンの向き（DirectX CCW）
		| aiPostProcessSteps::aiProcess_LimitBoneWeights // ボーンを４つに抑える
		| aiPostProcessSteps::aiProcess_OptimizeMeshes // メッシュの最適化
		| aiPostProcessSteps::aiProcess_OptimizeGraph // 上に同じ
		| aiPostProcessSteps::aiProcess_ValidateDataStructure // なんか有効なモノ以外消してくれるっぽい
		| aiPostProcessSteps::aiProcess_FindInvalidData // おかしな情報が入ってたら警告だしてくれるらしい
		| aiPostProcessSteps::aiProcess_SortByPType // タイプごとにソート、なんの？しらん
		//| aiPostProcessSteps::aiProcess_PreTransformVertices
	);

	if (scene == nullptr)
	{
		std::cout << this->importer_.GetErrorString() << std::endl;
		return false;
	}
	else
	{
		std::cout << file_name.c_str() << " imported." << std::endl;
	}

	this->mesh_list_.clear();

	return true;
}

aiNode * const AssimpModel::FindNodeRecursiveByName(aiNode * const node, const std::string name) const
{
	auto * ret = node->FindNode(name.c_str());

	for (auto n = 0U; n < node->mNumChildren && ret == nullptr; ++n)
		ret = this->FindNodeRecursiveByName(node->mChildren[n], name);

	return ret;
}

aiNodeAnim * const AssimpModel::FindNodeAnim(const unsigned int & anim_num, const std::string node_name) const
{
	auto scene = this->importer_.GetScene();

	if (!scene->HasAnimations() || scene->mNumAnimations <= anim_num)
		return nullptr;

	auto animation = scene->mAnimations[anim_num];

	for (auto n = 0U; n < animation->mNumChannels; n++)
	{
		auto node_anim = animation->mChannels[n];

		if (std::string(node_anim->mNodeName.data) == node_name)
			return node_anim;
	}

	return nullptr;
}

const int AssimpModel::GetBoneIdByName(const std::string name) const
{
	int ret = -1;

	auto search = std::find_if(this->bones_.begin(), this->bones_.end(), [&](const Bone & a) { return a.name_ == name; });

	if (search != this->bones_.end())
		ret = std::distance(this->bones_.begin(), search);

	return ret;
}

const unsigned int AssimpModel::get_mesh_cnt(void) const
{
	return this->mesh_list_.size();
}

const unsigned int AssimpModel::get_vtx_cnt(const unsigned int & mesh_num) const
{
	return this->mesh_list_[mesh_num].vertices_.size();
}

const unsigned int AssimpModel::get_index_cnt(const unsigned int & mesh_num) const
{
	return this->mesh_list_[mesh_num].indices_.size();
}

const unsigned int AssimpModel::get_bone_cnt(void) const
{
	return this->bones_.size();
}

const std::string & AssimpModel::get_mesh_name(const unsigned int & mesh_num) const
{
	return this->mesh_list_[mesh_num].name_;
}

const DirectX::XMMATRIX & AssimpModel::get_mesh_transformation(const unsigned int & mesh_num) const
{
	return this->mesh_list_[mesh_num].matrix_;
}

const unsigned int & AssimpModel::get_index(const unsigned int & mesh_num, const unsigned int & index_num) const
{
	return this->mesh_list_[mesh_num].indices_[index_num];
}

const XMFLOAT3 & AssimpModel::get_position(const unsigned int & mesh_num, const unsigned int & vtx_num) const
{
	return this->mesh_list_[mesh_num].vertices_[vtx_num].position_;
}

const XMFLOAT3 & AssimpModel::get_normal(const unsigned int & mesh_num, const unsigned int & vtx_num) const
{
	return this->mesh_list_[mesh_num].vertices_[vtx_num].normal_;
}

const XMFLOAT2 & AssimpModel::get_texcoord(const unsigned int & mesh_num, const unsigned int & vtx_num) const
{
	return this->mesh_list_[mesh_num].vertices_[vtx_num].texcoord_;
}

const DirectX::XMFLOAT4 & AssimpModel::get_color(const unsigned int & mesh_num, const unsigned int & vtx_num) const
{
	return this->mesh_list_[mesh_num].vertices_[vtx_num].color_;
}

const XMMATRIX & AssimpModel::get_bone_matrix(const unsigned int & bone_num) const
{
	return this->bones_[bone_num].matrix_;
}

const XMMATRIX & AssimpModel::get_bone_offset_matrix(const unsigned int & bone_num) const
{	
	return this->bones_[bone_num].offset_matrix_;
}

const std::string & AssimpModel::get_bone_name(const unsigned int & bone_num) const
{
	return this->bones_[bone_num].name_;
}

const int AssimpModel::get_bone_id(const std::string name) const
{
	int ret = -1;

	auto search = std::find_if(this->bones_.begin(), this->bones_.end(), [&](const Bone & a) { return a.name_ == name.c_str(); });

	if (search != this->bones_.end())
		ret = std::distance(this->bones_.begin(), search);

	return ret;
}

const unsigned int & AssimpModel::get_bone_id(const unsigned int & mesh_num, const unsigned int & vtx_num, const unsigned int & bone_index) const
{
	return this->mesh_list_[mesh_num].vertices_[vtx_num].bone_.id_[bone_index];
}

const int & AssimpModel::get_bone_parent_id(const unsigned int & bone_id) const
{
	return this->bones_[bone_id].parent_id_;
}

const unsigned int AssimpModel::get_bone_child_cnt(const unsigned int & bone_id) const
{
	return this->bones_[bone_id].children_id_.size();
}

const int & AssimpModel::get_bone_child_id(const unsigned int & bone_id, const unsigned int & child_num) const
{
	return this->bones_[bone_id].children_id_[child_num];
}

const float & AssimpModel::get_bone_weight(const unsigned int & mesh_num, const unsigned int & vtx_num, const unsigned int & bone_index) const
{
	return this->mesh_list_[mesh_num].vertices_[vtx_num].bone_.weight_[bone_index];
}

const int & AssimpModel::get_material_id(const unsigned int & mesh_num) const
{
	return this->mesh_list_[mesh_num].material_id_;
}

const unsigned int AssimpModel::get_material_cnt(void) const
{
	return this->materials_.size();
}

const std::string & AssimpModel::get_texture_name(const int & material_id) const
{
	return this->materials_[material_id].texture_;
}

const XMFLOAT4 & AssimpModel::get_diffuse(const int & material_id) const
{
	return this->materials_[material_id].diffuse_;
}

const XMFLOAT4 & AssimpModel::get_specular(const int & material_id) const
{
	return this->materials_[material_id].specular_;
}

const XMFLOAT4 & AssimpModel::get_ambient(const int & material_id) const
{
	return this->materials_[material_id].ambient_;
}

const XMFLOAT4 & AssimpModel::get_emissive(const int & material_id) const
{
	return this->materials_[material_id].emissive_;
}

const XMFLOAT4 & AssimpModel::get_transparent(const int & material_id) const
{
	return this->materials_[material_id].transparent_;
}

const XMFLOAT4 & AssimpModel::get_reflective(const int & material_id) const
{
	return this->materials_[material_id].reflective_;
}

bool AssimpModel::ProcessNode(aiNode * node)
{
	auto scene = this->importer_.GetScene();

	if (!scene->HasMeshes()) return false;

	for (auto n = 0U; n < node->mNumMeshes; ++n)
	{
		this->mesh_list_.emplace_back(PrivateMesh());
		auto & mesh = this->mesh_list_.back();
		auto & assimp_mesh = scene->mMeshes[node->mMeshes[n]];

		auto mt = node->mTransformation;

		auto node_parent = node->mParent;
		while (node_parent != nullptr)
		{
			mt = node_parent->mTransformation * mt;
			node_parent = node_parent->mParent;
		}

		mesh.matrix_ = aiMatrix4x42XMMATRIX(mt);

		this->ProcessMesh(mesh, assimp_mesh);
	}

	for (auto n = 0U; n < node->mNumChildren; ++n)
	{
		this->ProcessNode(node->mChildren[n]);
	}

	return true;
}

bool AssimpModel::ProcessMesh(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	if (!assimp_mesh->HasPositions()
		|| !assimp_mesh->HasNormals()
		|| !assimp_mesh->HasTextureCoords(0))
		return false;

	mesh.material_id_ = assimp_mesh->mMaterialIndex;

	for (auto n = 0U; n < assimp_mesh->mNumFaces; ++n)
	{
		auto & face = assimp_mesh->mFaces[n];

		for (unsigned int i = 0; i < face.mNumIndices; ++i)
			mesh.indices_.emplace_back(face.mIndices[i]);
	}

	this->ProcessPositions(mesh, assimp_mesh);
	this->ProcessNormals(mesh, assimp_mesh);
	this->ProcessTexCoords(mesh, assimp_mesh);
	this->ProcessBones(mesh, assimp_mesh);

	return true;
}

bool AssimpModel::ProcessMaterials(void)
{
	auto scene = this->importer_.GetScene();

	if (!scene->HasMaterials())
		return false;

	this->materials_.resize(scene->mNumMaterials);

	aiString str;
	for (auto n= 0U; n < scene->mNumMaterials; ++n)
	{
		auto & mat = this->materials_[n];
		auto assimp_mat = scene->mMaterials[n];
		
		assimp_mat->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &str);
		mat.texture_ = str.C_Str();

		aiColor3D color(0.f, 0.f, 0.f);
		assimp_mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		mat.diffuse_ = XMFLOAT4(reinterpret_cast<float*>(&color));
		assimp_mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
		mat.specular_ = XMFLOAT4(reinterpret_cast<float*>(&color));
		assimp_mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
		mat.ambient_ = XMFLOAT4(reinterpret_cast<float*>(&color));
		assimp_mat->Get(AI_MATKEY_COLOR_EMISSIVE, color);
		mat.emissive_ = XMFLOAT4(reinterpret_cast<float*>(&color));
		assimp_mat->Get(AI_MATKEY_COLOR_TRANSPARENT, color);
		mat.transparent_ = XMFLOAT4(reinterpret_cast<float*>(&color));
		assimp_mat->Get(AI_MATKEY_COLOR_REFLECTIVE, color);
		mat.reflective_ = XMFLOAT4(reinterpret_cast<float*>(&color));
	}

	return true;
}

void AssimpModel::ProcessPositions(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	mesh.vertices_.resize(assimp_mesh->mNumVertices);

	for (auto n = 0U; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & pos = mesh.vertices_[n].position_;
		auto & assimp_pos = assimp_mesh->mVertices[n];
		pos = XMFLOAT3(reinterpret_cast<float*>(&assimp_pos));
	}
}

void AssimpModel::ProcessNormals(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	for (auto n = 0U; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & norm = mesh.vertices_[n].normal_;
		auto & assimp_norm = assimp_mesh->mNormals[n];
		norm = XMFLOAT3(reinterpret_cast<float*>(&assimp_norm));
	}
}

void AssimpModel::ProcessTexCoords(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	for (auto n = 0U; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & uv = mesh.vertices_[n].texcoord_;
		auto & assimp_uv = assimp_mesh->mTextureCoords[0][n];
		uv = { assimp_uv.x, assimp_uv.y };
	}
}

void AssimpModel::ProcessColors(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	for (auto n = 0U; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & color = mesh.vertices_[n].color_;
		auto & assimp_color = assimp_mesh->mColors[0][n];
		color = XMFLOAT4(reinterpret_cast<float*>(&assimp_color));
	}
}

void AssimpModel::ProcessBones(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	std::vector<std::vector<BoneData>> vertices_bones;

	vertices_bones.resize(assimp_mesh->mNumVertices);

	for (auto n = 0U; n < assimp_mesh->mNumBones; ++n)
	{
		auto & bone = assimp_mesh->mBones[n];
		
		std::string bone_name = bone->mName.C_Str();

		unsigned int bone_id = 0;

		auto search = std::find_if(this->bones_.begin(), this->bones_.end(), [&](Bone & a) { return a.name_ == bone_name; });
		if (search == this->bones_.end())
		{
			this->bones_.emplace_back(Bone());
			auto & global_bone = this->bones_.back();

			auto * node = this->FindNodeRecursiveByName(this->importer_.GetScene()->mRootNode, bone_name);

			global_bone.name_ = bone_name;

			global_bone.matrix_ = aiMatrix4x42XMMATRIX(node->mTransformation);

			global_bone.offset_matrix_ = aiMatrix4x42XMMATRIX(bone->mOffsetMatrix);

			bone_id = std::distance(this->bones_.begin(), this->bones_.end() - 1);
		}
		else
		{
			bone_id = std::distance(this->bones_.begin(), search);
		}

		for (unsigned int i = 0; i < bone->mNumWeights; ++i)
		{
			auto & weight = bone->mWeights[i].mWeight;
			auto & vtx_id = bone->mWeights[i].mVertexId;
			BoneData bone;
			bone.bone_id = bone_id;
			bone.weight_ = weight;
			vertices_bones[vtx_id].emplace_back(bone);
		}
	}

	for (auto & vertices_bone : vertices_bones)
	{
		std::sort(vertices_bone.begin(), vertices_bone.end(),
			[](BoneData & a, BoneData & b) {
			return b.weight_ < a.weight_;
		});

		vertices_bone.resize(4);
	}

	for (auto n = 0U; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & vtx = mesh.vertices_[n];
		for (unsigned int i = 0; i < 4U; ++i)
		{
			auto & id = vertices_bones[n][i].bone_id;
			auto & weight = vertices_bones[n][i].weight_;
			vtx.bone_.id_[i] = id;
			vtx.bone_.weight_[i] = weight;
		}
	}
}

void AssimpModel::UpdateBone(void)
{
	for (auto & bone : this->bones_)
	{
		auto * node = this->FindNodeRecursiveByName(this->importer_.GetScene()->mRootNode, bone.name_);

		if (node->mParent)
		{
			bone.parent_id_ = this->GetBoneIdByName(node->mParent->mName.C_Str());
			if (bone.parent_id_ != -1)
			{
				this->bones_[bone.parent_id_].children_id_.emplace_back(this->GetBoneIdByName(bone.name_));
			}
		}
		else
		{
			bone.parent_id_ = -1;
		}
	}
}
