#include "camera.h"
#include "register.hpp"

void camera::setCameraPos(glm::vec2 pos)
{
	for (const auto& [entity, cameraObj] : Registry.view<cameraStr>().each())
	{
		cameraObj.cameraPos = pos;
	}
}

void camera::zoomCamera(glm::vec2 zoom)
{
	for (const auto& [entity, cameraObj] : Registry.view<cameraStr>().each())
	{
		cameraObj.cameraZoom = zoom;
	}
}

glm::vec2 camera::screenToView(glm::vec2 value)
{

	for (const auto& [entity, cameraObj] : Registry.view<cameraStr>().each())
	{
		glm::vec2 pos = cameraObj.cameraPos;
		pos.y = pos.y * -1.0f + float(SCRHEIGHT);

		return pos + ((value - glm::vec2(HALFSCRWIDTH, HALFSCRHEIGHT)) / cameraObj.cameraZoom);
	}
	return glm::vec2();
}

glm::vec2 camera::getCameraPos()
{
	for (const auto& [entity, cameraObj] : Registry.view<cameraStr>().each())
	{
		return cameraObj.cameraPos;
	}
	return { 0.0f, 0.0f };
}

glm::vec2 camera::getCameraZoom()
{
	for (const auto& [entity, cameraObj] : Registry.view<cameraStr>().each())
	{
		return cameraObj.cameraZoom;
	}
	return { 0.0f, 0.0f };
}


