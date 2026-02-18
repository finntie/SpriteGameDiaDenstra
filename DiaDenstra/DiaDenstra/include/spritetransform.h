#pragma once

#include <map>
#include <glm.hpp>

using namespace glm;

struct Stransform
{
public:
	
	const glm::vec2 getScale() { return Scale; }
	const float getRotation() { return Rotation; }
	const glm::vec2 getRotationPivot() { return RotationPivot; }
	const glm::vec2 getTranslation() { return Translation; }
	const glm::vec2 getVel() { return appliedVel; }

	void setScale(glm::vec2 scale) { Scale = scale, scaleChanged = true; }
	void setRotation(float rotation) { Rotation = glm::radians(rotation), rotationChanged = true; }
	void setRotationPivot(glm::vec2 pivot) { RotationPivot = pivot; }
	void setTranslation(glm::vec2 translation) { Translation = translation; }
	void setVel(glm::vec2 vel) { appliedVel = vel; }

	bool scaleChanged = true;
	bool rotationChanged = true;

private:

	glm::vec2 Scale = { 1.0f,1.0f };
	glm::vec2 RotationPivot{ 0.5f, 0.5f };
	float Rotation = 0.0f;
	glm::vec2 Translation = { 0,0 };
	glm::vec2 appliedVel = { 0,0 };
};

struct spriteStr;
class spritetransform
{
public:

	/// <summary>Applies scale, rotation and translation to the x and y position</summary>
	/// <param name="x">Pixel x location, returns new location</param>
	/// <param name="y">Pixel y location, returns new location</param>
	/// <param name="color">Current color of the pixel, returns new color of this pixel</param>
	/// <param name="useAvarageColor">Calculate avarage between neighbouring pixels if enabled</param>
	/// <param name="Sprite">Sprite information</param>
	/// <param name="scale">Scale</param>
	/// <param name="rotation">Rotation</param>
	/// <param name="translation">Position</param>
	static void applyTransform(int& x, int& y, std::pair<int, int>* fillPixels, spriteStr & Sprite, glm::vec2 scale, float rotation, glm::vec2 translation);

	static void fillRotateGaps(spriteStr& Sprite, float rotation, glm::vec2 scale);
	
	static void rotatePointFloat(glm::vec2& point, float rot, float cosTheta, float sinTheta, float halfSpriteX, float halfSpriteY);
	static void barycentric(float pointx, float pointy, float ax, float ay,float bx, float by, float cx, float cy, float& u, float& v, float& w);


private:



};

