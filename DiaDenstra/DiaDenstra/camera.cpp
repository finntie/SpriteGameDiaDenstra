#include "camera.h"
#include "register.hpp"

void camera::setCameraPos(glm::vec2 pos)
{
	for (const auto& [entity, cameraObj] : Registry.view<cameraStr>().each())
	{
		cameraObj.view = glm::translate(glm::mat4(1), glm::vec3(-pos, 0.0f));
	}
}

void camera::zoomCamera(glm::vec2 zoom)
{
	for (const auto& [entity, cameraObj] : Registry.view<cameraStr>().each())
	{
		cameraObj.projection = glm::ortho(0.0f, float(SCRWIDTH) / zoom.x, 0.0f, float(SCRHEIGHT) / zoom.y, -1.0f, 1.0f);

	}
}
