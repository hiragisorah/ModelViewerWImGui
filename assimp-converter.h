#pragma once

#pragma warning(disable: 4819)

#include <assimp\scene.h>
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>

#include <iostream>
#include <vector>
#include <DirectXMath.h>

class AssimpModel
{
public:
	struct Bone
	{
		DirectX::XMMATRIX matrix_;
		DirectX::XMMATRIX offset_matrix_;
		std::string name_;

		int parent_id_;
		std::vector<int> children_id_;
	};

	struct Material
	{
		std::string texture_;
		DirectX::XMFLOAT4 diffuse_;
		DirectX::XMFLOAT4 specular_;	
		DirectX::XMFLOAT4 ambient_;
		DirectX::XMFLOAT4 emissive_;
		DirectX::XMFLOAT4 transparent_;
		DirectX::XMFLOAT4 reflective_;
	};

	struct VertexBoneData
	{
		unsigned int id_[4];
		float weight_[4];
	};

	struct BoneData
	{
		unsigned int bone_id;
		float weight_;
	};

private:
	struct PrivateMesh
	{
		struct Vertex
		{
			DirectX::XMFLOAT3 position_;
			DirectX::XMFLOAT3 normal_;
			DirectX::XMFLOAT2 texcoord_;
			DirectX::XMFLOAT4 color_;

			VertexBoneData bone_;
		};

		DirectX::XMMATRIX matrix_;
		std::vector<Vertex> vertices_;
		std::vector<unsigned int> indices_;
		std::string name_ = "";

		int material_id_ = -1;
	};

public:
	AssimpModel(std::string file_name);

private:
	bool Init(std::string file_name);

private:
	Assimp::Importer importer_;
	std::vector<PrivateMesh> mesh_list_;
	std::vector<Bone> bones_;
	std::vector<Material> materials_;

private:
	aiNode * const FindNodeRecursiveByName(aiNode * const node, const std::string name) const;
	aiNodeAnim * const FindNodeAnim(const unsigned int & anim_num, const std::string node_name) const;

public:
	const int GetBoneIdByName(const std::string name) const;

public:
	const unsigned int get_mesh_cnt(void) const;
	const unsigned int get_vtx_cnt(const unsigned int & mesh_num) const;
	const unsigned int get_index_cnt(const unsigned int & mesh_num) const;
	const unsigned int get_bone_cnt(void) const;
	const std::string & get_mesh_name(const unsigned int & mesh_num) const;
	const DirectX::XMMATRIX & get_mesh_transformation(const unsigned int & mesh_num) const;
	const unsigned int & get_index(const unsigned int & mesh_num, const unsigned int & index_num) const;
	const DirectX::XMFLOAT3 & get_position(const unsigned int & mesh_num, const unsigned int & vtx_num) const;
	const DirectX::XMFLOAT3 & get_normal(const unsigned int & mesh_num, const unsigned int & vtx_num) const;
	const DirectX::XMFLOAT2 & get_texcoord(const unsigned int & mesh_num, const unsigned int & vtx_num) const;
	const DirectX::XMFLOAT4 & get_color(const unsigned int & mesh_num, const unsigned int & vtx_num) const;
	const DirectX::XMMATRIX & get_bone_matrix(const unsigned int & bone_num) const;
	const DirectX::XMMATRIX & get_bone_offset_matrix(const unsigned int & bone_num) const;
	const std::string & get_bone_name(const unsigned int & bone_num) const;
	const int get_bone_id(const std::string name) const;
	const unsigned int & get_bone_id(const unsigned int & mesh_num, const unsigned int & vtx_num, const unsigned int & bone_index) const;
	const int & get_bone_parent_id(const unsigned int & bone_id) const;
	const unsigned int get_bone_child_cnt(const unsigned int & bone_id) const;
	const int & get_bone_child_id(const unsigned int & bone_id, const unsigned int & child_num) const;
	const float & get_bone_weight(const unsigned int & mesh_num, const unsigned int & vtx_num, const unsigned int & bone_index) const;
	const int & get_material_id(const unsigned int & mesh_num) const;
	const unsigned int get_material_cnt(void) const;
	const std::string & get_texture_name(const int & material_id) const;
	const DirectX::XMFLOAT4 & get_diffuse(const int & material_id) const;
	const DirectX::XMFLOAT4 & get_specular(const int & material_id) const;
	const DirectX::XMFLOAT4 & get_ambient(const int & material_id) const;
	const DirectX::XMFLOAT4 & get_emissive(const int & material_id) const;
	const DirectX::XMFLOAT4 & get_transparent(const int & material_id) const;
	const DirectX::XMFLOAT4 & get_reflective(const int & material_id) const;

private:
	bool ProcessNode(aiNode * node);
	bool ProcessMesh(PrivateMesh & mesh, aiMesh * assimp_mesh);
	bool ProcessMaterials(void);
	void ProcessPositions(PrivateMesh & mesh, aiMesh * assimp_mesh);
	void ProcessNormals(PrivateMesh & mesh, aiMesh * assimp_mesh);
	void ProcessTexCoords(PrivateMesh & mesh, aiMesh * assimp_mesh);
	void ProcessColors(PrivateMesh & mesh, aiMesh * assimp_mesh);
	void ProcessBones(PrivateMesh & mesh, aiMesh * assimp_mesh);
	void UpdateBone(void);
};