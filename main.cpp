#include "app2.h"
#include "assimp-converter.h"
#include <iostream>

int main(void)
{
	Window::Initalize();
	Graphics::Initalize();
	//GUI::Initalize();
	App2::Initalize();

	while (Window::Run() && Graphics::Begin() && App2::Run() && /*GUI::End() &&*/ Graphics::End());

	App2::Finalize();
	//GUI::Finalize();
	Graphics::Finalize();
	Window::Finalize();

	return 0;
}