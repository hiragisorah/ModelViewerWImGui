#include "app.h"

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "path-manager.h"

bool bShow = true;

namespace App
{
	PathManager manager_;

	std::vector<bool> selectables_dir_;

	int combo_dir_;
	std::string file_status_ = "";

	std::vector<std::string> mesh_list_box_;

	Assimp::Importer importer_;

	Model model_;

	int mesh_list_box_current = 0;
	int vertices_list_box_current = 0;
	int indices_list_box_current = 0;
	int bone_list_box_current = 0;
	int anim_list_box_current = 0;

	char ** mesh_list_box = {};
	char ** vertices_list_box = {};
	char ** indices_list_box = {};
	char ** bone_list_box = {};
	char ** anim_list_box = {};

	// Animation

	std::vector<Animation> animations_;

	std::vector<std::string> bone_name_list_;
	std::unordered_map<std::string, BoneData> bone_mapping_;
	unsigned int bone_cnt_ = 0;
	struct InstantBone
	{
		float weight_ = 0.f;
		int id_ = 0;
	};

	std::vector<std::vector<InstantBone>> bones_;

	void Initalize(void)
	{
		//ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.7f, 0.2f, 1.0f));
		//ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0f, 0.3f, 0.1f, 1.0f));

		manager_.SetPath("resource/model");
		selectables_dir_.resize(manager_.FileCnt());
	}

	void Finalize(void)
	{
		importer_.FreeScene();
	}

	bool Run(void)
	{
		if(bShow)
			ShowGUI();
		
		return true;
	}

	void ShowGUI(void)
	{
		ImGui::SetNextWindowPos(ImVec2(20.f, 40.f), ImGuiSetCond_Once);
		ImGui::SetNextWindowSize(ImVec2(600.f, 600.f), ImGuiSetCond_Once);
		
		ImGui::Begin("Input", nullptr, ImGuiWindowFlags_NoResize);
		{
			std::string combo_str = "";
			for (auto & file : manager_.Files())
				combo_str += file.filename().string() + "*";

			std::replace(combo_str.begin(), combo_str.end(), '*', '\0');

			ImGui::Text("Model");
			if (ImGui::Combo(file_status_.c_str(), &combo_dir_, combo_str.c_str()))
			{
				FileReadCombo();
			}

			{
				ImGui::Text("Meshes");

				ImGui::ListBox("##Meshes", &mesh_list_box_current, mesh_list_box, model_.meshes_.size(), 4);
			}

			{
				ImGui::Text("Vertices");

				if (model_.meshes_.size())
				{
					auto mesh = model_.meshes_[mesh_list_box_current];
					ImGui::ListBox("##Vertices", &vertices_list_box_current, vertices_list_box, mesh.vertices_.size(), 4);
				}
				else
				{
					ImGui::ListBox("##Vertices", &vertices_list_box_current, nullptr, 0, 4);
				}
			}

			{
				ImGui::Text("Indices");

				if (model_.meshes_.size())
				{
					auto mesh = model_.meshes_[mesh_list_box_current];
					ImGui::ListBox("##Indices", &indices_list_box_current, indices_list_box, mesh.indices_.size(), 4);
				}
				else
				{
					ImGui::ListBox("##Indices", &indices_list_box_current, nullptr, 0, 4);
				}
				if (ImGui::Button("Show", ImVec2(100.f, 20.f)))
				{
					Graphics::SetupModel(model_);
				}
			}

			{
				ImGui::Text("Bones");

				ImGui::ListBox("##Bones", &bone_list_box_current, bone_list_box, bones_.size(), 4);
			}

			{
				ImGui::Text("Animations");

				ImGui::ListBox("##Animations", &anim_list_box_current, anim_list_box, animations_.size(), 4);
			}
		}
		ImGui::End();
	}

	void FileReadCombo(void)
	{
		RemoveMeshes();
		RemoveVertices();
		RemoveIndices();
		RemoveBones();

		auto path = sys::absolute(manager_.Files()[combo_dir_]).string();
		auto scene = importer_.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
		if (scene == nullptr)
		{
			file_status_ = "Failed.";
			model_.meshes_.clear();
			MessageBoxA(0, importer_.GetErrorString(), "", MB_OK);
		}
		else
		{
			file_status_ = "Success.";
			model_.meshes_.clear();
			ProcessNode(scene->mRootNode, scene);
		}

		UpdateMeshes();
		UpdateVertices();
		UpdateIndices();
		UpdateBones();
	}

	void RemoveMeshes(void)
	{
		if (mesh_list_box)
		{
			for (unsigned int n = 0; n < model_.meshes_.size(); n++)
			{
				if (mesh_list_box[n])
					delete[] mesh_list_box[n];
			}
			delete[] mesh_list_box;
		}
	}

	void RemoveVertices(void)
	{
		if (model_.meshes_.size())
		{
			auto mesh = model_.meshes_[mesh_list_box_current];

			if (vertices_list_box)
			{
				for (unsigned int n = 0; n < mesh.vertices_.size(); ++n)
				{
					if (vertices_list_box[n])
						delete[] vertices_list_box[n];
				}

				delete[] vertices_list_box;
			}
		}
	}

	void RemoveIndices(void)
	{
		if (model_.meshes_.size())
		{
			auto mesh = model_.meshes_[mesh_list_box_current];

			if (indices_list_box)
			{
				for (unsigned int n = 0; n < mesh.indices_.size(); ++n)
				{
					if (indices_list_box[n])
						delete[] indices_list_box[n];
				}

				delete[] indices_list_box;
			}
		}
	}

	void RemoveBones(void)
	{
		if (model_.meshes_.size())
		{
			auto mesh = model_.meshes_[mesh_list_box_current];

			if (bone_list_box)
			{
				for (unsigned int n = 0; n < mesh.vertices_.size(); ++n)
				{
					if (bone_list_box[n])
						delete[] bone_list_box[n];
				}

				delete[] bone_list_box;
			}
		}
	}

	void RemoveAnims(void)
	{
	}

	void UpdateMeshes(void)
	{	
		mesh_list_box = new char*[model_.meshes_.size()];

		for (unsigned int n = 0; n < model_.meshes_.size(); ++n)
		{
			mesh_list_box[n] = new char[model_.meshes_[n].name_.size() + 1];
			strcpy(mesh_list_box[n], model_.meshes_[n].name_.c_str());
		}
	}

	void UpdateVertices(void)
	{
		if (model_.meshes_.size())
		{
			auto mesh = model_.meshes_[mesh_list_box_current];

			vertices_list_box = new char*[mesh.vertices_.size()];

			for (unsigned int n = 0; n < mesh.vertices_.size(); ++n)
			{
				char tmp[200] = {};
				auto & vtx = mesh.vertices_[n];
				auto & pos = vtx.position_;
				auto & norm = vtx.normal_;
				auto & uv = vtx.texcoord_;
				sprintf(tmp, "Pos: %3.3f, %3.3f, %3.3f Norm: %3.3f, %3.3f, %3.3f UV: %3.3f, %3.3f", pos.x, pos.y, pos.z, norm.x, norm.y, norm.z, uv.x, uv.y);
				std::string test = tmp;

				vertices_list_box[n] = new char[test.size() + 1];
				strcpy(vertices_list_box[n], test.c_str());
			}
		}
	}

	void UpdateIndices(void)
	{
		if (model_.meshes_.size())
		{
			auto mesh = model_.meshes_[mesh_list_box_current];

			indices_list_box = new char*[mesh.indices_.size()];

			for (unsigned int n = 0; n < mesh.indices_.size(); ++n)
			{
				std::string test = std::to_string(mesh.indices_[n]);

				indices_list_box[n] = new char[test.size() + 1];
				strcpy(indices_list_box[n], test.c_str());
			}
		}
	}

	void UpdateBones(void)
	{
		if (model_.meshes_.size())
		{
			auto mesh = model_.meshes_[mesh_list_box_current];

			bone_list_box = new char*[mesh.vertices_.size()];

			for (unsigned int n = 0; n < mesh.vertices_.size(); ++n)
			{
				char tmp[200] = {};
				auto & vtx = mesh.vertices_[n];
				auto & id = vtx.bone_data_.ids_;
				auto & weight = vtx.bone_data_.weights_;
				sprintf(tmp, "id: %3d, weight: %3.3f", id, weight);
				std::string test = tmp;

				bone_list_box[n] = new char[test.size() + 1];
				strcpy(bone_list_box[n], test.c_str());
			}
		}
	}

	void UpdateAnims(void)
	{
		if (animations_.size())
		{

		}
	}

	void ProcessBones(unsigned int mesh_index, const aiMesh * mesh)
	{
		bones_.clear();
		bones_.resize(mesh->mNumVertices);
		auto & _mesh = model_.meshes_[mesh_index];
		auto & _bones = bones_[mesh_index];
		for (unsigned int n = 0; n < mesh->mNumBones; ++n)
		{
			auto & bone = mesh->mBones[n];

			std::string bone_name(bone->mName.data);

			auto & bone_location = bone_mapping_[bone_name];

			if (bone_location.index_ == -1)
			{
				bone_location.index_ = bone_cnt_;
				bone_cnt_++;
			}
			
			for (unsigned int i = 0; i < bone->mNumWeights; ++i)
			{
				auto & vertex_num = bone->mWeights[i].mVertexId;
				auto & weight = bone->mWeights[i].mWeight;

				InstantBone tmp_bone;
				tmp_bone.id_ = bone_location.index_;
				tmp_bone.weight_ = weight;

				bones_[vertex_num].emplace_back(tmp_bone);
			}
		}
		for (int n = 0; n < mesh->mNumVertices; ++n)
		{
			auto & bone_info = bones_[n];

			std::sort(bone_info.begin(), bone_info.end(),
				[](InstantBone & a, InstantBone & b)
			{
				return a.weight_ < b.weight_;
			});

			bone_info.resize(4);
		}
	}
	
	void ProcessAnimation(aiAnimation * anim, const aiScene * scene)
	{
		bone_cnt_ = 0;
	}

	void ProcessNode(aiNode * node, const aiScene * scene)
	{
		for (unsigned int n = 0; n < node->mNumMeshes; ++n)
		{
			auto mesh = scene->mMeshes[node->mMeshes[n]];
			model_.meshes_.emplace_back(ProcessMesh(mesh, scene));
			ProcessBones(node->mMeshes[n], mesh);
		}

		for (unsigned int n = 0; n < node->mNumChildren; ++n)
			ProcessNode(node->mChildren[n], scene);
	}

	Mesh ProcessMesh(aiMesh * mesh, const aiScene * scene)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::string texture;

		for (unsigned int n = 0; n < mesh->mNumVertices; ++n)
		{
			Vertex vertex;

			vertex.position_.x = mesh->mVertices[n].x;
			vertex.position_.y = mesh->mVertices[n].y;
			vertex.position_.z = mesh->mVertices[n].z;

			if (mesh->mNormals)
			{
				vertex.normal_.x = (float)mesh->mNormals[n].x;
				vertex.normal_.y = (float)mesh->mNormals[n].y;
				vertex.normal_.z = (float)mesh->mNormals[n].z;
			}

			if (mesh->mTextureCoords[0])
			{
				vertex.texcoord_.x = (float)mesh->mTextureCoords[0][n].x;
				vertex.texcoord_.y = (float)mesh->mTextureCoords[0][n].y;
			}

			vertices.emplace_back(vertex);
		}

		for (unsigned int n = 0; n < mesh->mNumFaces; ++n)
		{
			aiFace face = mesh->mFaces[n];

			for (unsigned int i = 0; i < face.mNumIndices; ++i)
				indices.emplace_back(face.mIndices[i]);
		}

		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial * mat = scene->mMaterials[mesh->mMaterialIndex];
			aiString str;
			mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
			texture = str.C_Str();
		}

		Mesh ret;

		ret.vertices_ = vertices;
		ret.indices_ = indices;
		ret.texture_ = texture;
		ret.name_ = "[" + std::to_string(model_.meshes_.size()) + "]: " + mesh->mName.C_Str();

		return ret;
	}
}