#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include "common.h"

struct cameraStr
{
public:
	glm::vec2 cameraPos = glm::vec2(0.0f, 0.0f);
	glm::vec2 cameraZoom = glm::vec2(1.0f, 1.0f);

	glm::mat4 getView() { return glm::translate(glm::mat4(1), glm::vec3(-cameraPos, 0.0f));	}
	glm::mat4 getProjection() 
	{
		float halfWidth = (HALFSCRWIDTH / cameraZoom.x) ;
		float halfHeight = (HALFSCRHEIGHT / cameraZoom.y);
		return glm::ortho(-halfWidth,  halfWidth,  -halfHeight,  halfHeight, -1.0f, 1.0f);
	}

private:

};


class camera
{
public:

	static void setCameraPos(glm::vec2 pos);
	static void zoomCamera(glm::vec2 zoom);
	
	//Due to the camera changment, some positions need to be offset.
	static glm::vec2 screenToView(glm::vec2 value);

	static glm::vec2 getCameraPos();
	static glm::vec2 getCameraZoom();
private:
};

