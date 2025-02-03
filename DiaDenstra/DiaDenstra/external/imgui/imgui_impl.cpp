#include "imgui.h"
#include "imgui_impl.h"

#if defined(BEE_PLATFORM_PC) && defined(BEE_GRAPHICS_OPENGL)

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "core/engine.hpp"
#include "core/device.hpp"
#include "platform/opengl/open_gl.hpp"

bool ImGui_Impl_Init()
{
	const auto window = bee::Engine.Device().GetWindow();
	const bool opengl = ImGui_ImplOpenGL3_Init();
	const bool glfw = ImGui_ImplGlfw_InitForOpenGL(window, true);	
	return glfw && opengl;
}

void ImGui_Impl_Shutdown()
{
	ImGui_ImplGlfw_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData*)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.3f, 0.2f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#elif defined(BEE_PLATFORM_XBOX)

#include "imgui_impl_xbox.h"


bool ImGui_Impl_Init()
{
	return ImGui_Impl_Xbox_Init();
}

void ImGui_Impl_Shutdown()
{
	ImGui_Impl_Xbox_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	ImGui_Impl_Xbox_NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	ImGui_Impl_Xbox_RenderDrawData(ImGui::GetDrawData());
}

#elif defined(BEE_PLATFORM_PROSPERO)

#include "imgui_impl_prospero.h"

bool ImGui_Impl_Init()
{
	return ImGui_Impl_Prospero_Init();
}

void ImGui_Impl_Shutdown()
{
	ImGui_Impl_Prospero_Shutdown();
}


void ImGui_Impl_NewFrame()
{
	ImGui_Impl_Prospero_NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	ImGui_Impl_Prospero_RenderDrawData(ImGui::GetDrawData());
}

#elif defined(BEE_PLATFORM_SWITCH)

#include "imgui_impl_switch.h"

bool ImGui_Impl_Init()
{
	return ImGui_Impl_Switch_Init();
}

void ImGui_Impl_Shutdown()
{
	ImGui_Impl_Switch_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	ImGui_Impl_Switch_NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	ImGui_Impl_Switch_RenderDrawData(ImGui::GetDrawData());
}

#elif defined(BEE_PLATFORM_PROSPERO)

bool ImGui_Impl_Init() { return true; }

void ImGui_Impl_Shutdown() { }

void ImGui_Impl_NewFrame() { }

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data) { }

#endif