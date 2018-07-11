#include "assimp-converter.h"

#include <assimp\scene.h>
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>

#include <algorithm>

Assimp::Importer importer;

static unsigned int indent = 0;
using namespace DirectX;

XMMATRIX aiMatrix4x42XMMATRIX(aiMatrix4x4 & m)
{
	m.Transpose();
	return XMLoadFloat4x4(&XMFLOAT4X4(&m.a1));
}

AssimpModel::AssimpModel(std::string file_name)
{
	if (!Init(file_name)) return;

	auto & root = importer.GetScene()->mRootNode;

	ProcessNode(root);
}

bool AssimpModel::Init(std::string file_name)
{
	importer.FreeScene();
	
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene * scene = importer.ReadFile(file_name
		, aiPostProcessSteps::aiProcess_Triangulate
		| aiPostProcessSteps::aiProcess_MakeLeftHanded
		| aiPostProcessSteps::aiProcess_FlipUVs
	);

	if (scene == nullptr)
	{
		std::cout << importer.GetErrorString() << std::endl;
		return false;
	}
	else
	{
		std::cout << file_name.c_str() << " exported." << std::endl;
		return true;
	}

	this->mesh_list_.clear();

	auto aimatrix = scene->mRootNode->mTransformation.Inverse();

	this->global_inverse_matrix_ = XMMatrixInverse(nullptr, aiMatrix4x42XMMATRIX(aimatrix));
}

aiNode * const AssimpModel::FindNodeRecursiveByName(aiNode * const node, const std::string & name) const
{
	aiNode * ret = node->FindNode(name.c_str());

	for (unsigned int n = 0; n < node->mNumChildren && ret == nullptr; ++n)
		ret = this->FindNodeRecursiveByName(node->mChildren[n], name);

	return ret;
}

const int AssimpModel::GetBoneIdByName(const std::string & name)
{
	int ret = -1;

	auto search = std::find_if(this->bones_.begin(), this->bones_.end(), [&](Bone & a) { return a.name_ == name.c_str(); });

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

const int AssimpModel::get_bone_id(const std::string name)
{
	int ret = -1;

	auto search = std::find_if(this->bones_.begin(), this->bones_.end(), [&](Bone & a) { return a.name_ == name.c_str(); });

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

const int & AssimpModel::get_bone_child_id(const unsigned int & bone_id, const unsigned int & child_id) const
{
	return this->bones_[bone_id].children_id_[child_id];
}

const float & AssimpModel::get_bone_weight(const unsigned int & mesh_num, const unsigned int & vtx_num, const unsigned int & bone_index) const
{
	return this->mesh_list_[mesh_num].vertices_[vtx_num].bone_.weight_[bone_index];
}

const DirectX::XMMATRIX & AssimpModel::get_global_inverse_matrix(void) const
{
	return this->global_inverse_matrix_;
}

bool AssimpModel::ProcessNode(aiNode * node)
{
	auto scene = importer.GetScene();

	//std::cout << node->mName.C_Str() << std::endl;

	if (!scene->HasMeshes()) return false;

	for (unsigned int n = 0; n < node->mNumMeshes; ++n)
	{
		this->mesh_list_.emplace_back(PrivateMesh());
		auto & mesh = this->mesh_list_.back();
		auto & assimp_mesh = scene->mMeshes[node->mMeshes[n]];
		this->ProcessMesh(mesh, assimp_mesh);
	}

	this->UpdateBone();

	indent++;
	for (unsigned int n = 0; n < node->mNumChildren; ++n)
	{
		for (unsigned int i = 0; i < indent; ++i)
		{
			//std::cout << " ";
		}
		this->ProcessNode(node->mChildren[n]);
	}
	indent--;

	return true;
}

bool AssimpModel::ProcessMesh(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	if (!assimp_mesh->HasPositions()
		|| !assimp_mesh->HasNormals()
		|| !assimp_mesh->HasTextureCoords(0))
		return false;

	for (unsigned int n = 0; n < assimp_mesh->mNumFaces; ++n)
	{
		aiFace & face = assimp_mesh->mFaces[n];

		for (unsigned int i = 0; i < face.mNumIndices; ++i)
			mesh.indices_.emplace_back(face.mIndices[i]);
	}

	this->ProcessPositions(mesh, assimp_mesh);
	this->ProcessNormals(mesh, assimp_mesh);
	this->ProcessTexCoords(mesh, assimp_mesh);
	this->ProcessBones(mesh, assimp_mesh);

	return true;
}

void AssimpModel::ProcessPositions(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	mesh.vertices_.resize(assimp_mesh->mNumVertices);

	for (unsigned int n = 0; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & pos = mesh.vertices_[n].position_;
		auto & assimp_pos = assimp_mesh->mVertices[n];
		pos = { assimp_pos.x, assimp_pos.y, assimp_pos.z };
	}
}

void AssimpModel::ProcessNormals(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	for (unsigned int n = 0; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & norm = mesh.vertices_[n].normal_;
		auto & assimp_norm = assimp_mesh->mNormals[n];
		norm = { assimp_norm.x, assimp_norm.y, assimp_norm.z };
	}
}

void AssimpModel::ProcessTexCoords(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	for (unsigned int n = 0; n < assimp_mesh->mNumVertices; ++n)
	{
		auto & uv = mesh.vertices_[n].texcoord_;
		auto & assimp_uv = assimp_mesh->mTextureCoords[0][n];
		uv = { assimp_uv.x, assimp_uv.y };
	}
}

void AssimpModel::ProcessBones(PrivateMesh & mesh, aiMesh * assimp_mesh)
{
	std::vector<std::vector<BoneData>> vertices_bones;

	vertices_bones.resize(assimp_mesh->mNumVertices);

	for (unsigned int n = 0; n < assimp_mesh->mNumBones; ++n)
	{
		auto & bone = assimp_mesh->mBones[n];

		std::string bone_name = bone->mName.C_Str();

		unsigned int bone_id = 0;

		auto search = std::find_if(this->bones_.begin(), this->bones_.end(), [&](Bone & a) { return a.name_ == bone_name; });
		if (search == this->bones_.end())
		{
			this->bones_.emplace_back(Bone());
			auto & global_bone = this->bones_.back();

			aiNode * node = this->FindNodeRecursiveByName(importer.GetScene()->mRootNode, bone_name);

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

	for (unsigned int n = 0; n < assimp_mesh->mNumVertices; ++n)
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
		aiNode * node = this->FindNodeRecursiveByName(importer.GetScene()->mRootNode, bone.name_);

		if (node->mParent)
		{
			bone.parent_id_ = this->GetBoneIdByName(node->mParent->mName.C_Str());
			if (bone.parent_id_ != -1)
			{
				this->bones_[bone.parent_id_].children_id_.emplace_back((int)this->bones_.size() - 1);
			}
			else
			{
				int a = 0;
			}
		}
		else
		{
			bone.parent_id_ = -1;
		}
	}
}
