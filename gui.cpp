#include "gui.h"
#include "window.h"
#include "graphics.h"

namespace GUI
{
	void Initalize(void)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplWin32_Init(Window::hwnd());
		ImGui_ImplDX11_Init(Graphics::device().Get(), Graphics::context().Get());

		ImGui::StyleColorsDark();
	}

	void Finalize(void)
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	bool Begin(void)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		return true;
	}

	bool End(void)
	{
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		return true;
	}
}

const std::string & gUI::caption(void)
{
	return this->caption_;
}

const ImVec2 gUI::position(void)
{
	return this->position_;
}

const ImVec2 gUI::size(void)
{
	return this->size_;
}

void gUI::set_caption(std::string caption)
{
	this->caption_ = caption;
}

void gUI::set_position(float x, float y)
{
	this->position_ = { x,y };
}

void gUI::set_size(float x, float y)
{
	this->size_ = { x,y };
}

void gUI::Render(void)
{
	ImGui::SetNextWindowPos(this->position_, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(this->size_, ImGuiSetCond_Once);
	
	ImGui::Begin(this->caption_.c_str());

	ImGui::End();
}
