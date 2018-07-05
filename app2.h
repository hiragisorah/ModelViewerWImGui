#pragma once

#include "window.h"
#include "graphics.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include <DirectXMath.h>
#include <string>
#include <vector>

namespace App2
{
	using namespace DirectX;

	void Initalize(void);
	void Finalize(void);

	bool Run(void);
}