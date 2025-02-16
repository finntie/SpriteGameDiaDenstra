#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include "common.h"

struct cameraStr
{
public:
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::ortho(0.0f, float(SCRWIDTH), 0.0f, float(SCRHEIGHT), -1.0f, 1.0f);


private:

	glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
	glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

};


class camera
{
public:

	static void setCameraPos(glm::vec2 pos);
	static void zoomCamera(glm::vec2 zoom);

private:
};

