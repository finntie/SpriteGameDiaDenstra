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

void camera::followPosition(glm::vec2 pos)
{
	setCameraPos(pos);
	//Useless function?
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


