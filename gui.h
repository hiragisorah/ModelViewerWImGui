#pragma once

#include <string>

#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx11.h"
#include "imgui\imgui_impl_win32.h"

namespace GUI
{
	void Initalize(void);
	void Finalize(void);

	bool Begin(void);
	bool End(void);
}

class gUI
{
public:
	gUI(void)
		: caption_("no_name")
		, position_(ImVec2(20.f, 20.f))
		, size_(ImVec2(200.f, 200.f))
	{}

private:
	std::string caption_;
	ImVec2 position_;
	ImVec2 size_;

public:
	const std::string & caption(void);
	const ImVec2 position(void);
	const ImVec2 size(void);

	void set_caption(std::string caption);
	void set_position(float x, float y);
	void set_size(float x, float y);

public:
	void Render(void);
};