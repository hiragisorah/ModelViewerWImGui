#pragma once

#include "window.h"
#include "graphics.h"
#include "gui.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include <DirectXMath.h>
#include <string>
#include <vector>

namespace App
{
	using namespace DirectX;

	void Initalize(void);
	void Finalize(void);

	bool Run(void);

	void ShowGUI(void);
	
	void FileReadCombo(void);

	void RemoveMeshes(void);
	void RemoveVertices(void);
	void RemoveIndices(void);
	void RemoveBones(void);
	void RemoveAnims(void);
	void UpdateMeshes(void);
	void UpdateVertices(void);
	void UpdateIndices(void);
	void UpdateBones(void);
	void UpdateAnims(void);

	void ProcessBones(unsigned int mesh_index, const aiMesh * mesh);
	void ProcessAnimation(aiAnimation * anim, const aiScene * scene);
	void ProcessNode(aiNode * node, const aiScene * scene);
	Mesh ProcessMesh(aiMesh * mesh, const aiScene * scene);
}