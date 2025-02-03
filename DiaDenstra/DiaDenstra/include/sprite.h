#pragma once

#include <malloc.h>
#include <glm.hpp>
#include "register.hpp"
//Holds all info of each sprite
class GLTexture;
struct spriteStr
{
public:

	friend class sprite;
	friend class spritetransform;
	

	const char* name = "None";
	unsigned int* buffer = 0;
	unsigned int* afterBuffer = nullptr; //Buffer after all changes were made
	int AfterBufferSize = 0;
	float depth = 0.0f;
	int height = 0;
	int width = 0;
	GLTexture* texture;
	bool bufferChanged = false;
	int currentFrame = 0;
	int maxFrames = 0;


	int getAfterWidthDif() { return afterWidthDifference; }
	int getAfterHeightDif() { return afterHeightDifference; }
	int getAfterWidth() { return afterWidth; }
	int getAfterHeight() { return afterHeight; }

	~spriteStr()
	{
		if (initializedBuffer) _aligned_free(buffer); // free only if we allocated the buffer ourselves
		if (initializedBuffer && afterBuffer) delete[] afterBuffer;
	}

protected:

	int afterHeight = 0;
	int afterWidth = 0;
	int afterWidthDifference = 0;
	int afterHeightDifference = 0;

	bool initializedBuffer = false;
	bool frameChanged = true;
};

class screen;
class sprite
{
public:

	sprite() {};
	~sprite() {};

	static void initFromFile(spriteStr* Sprite, const char* file, const int frames, const char* name);
	static void initEmpty(spriteStr* Sprite, const int frames, const char* name, int width, int height);
	/// <summary>Clears sprite to a color</summary>
	/// <param name="Sprite">Sprite that needs to be cleared</param>
	/// <param name="color">Color it clears to, (1,1,1) = white</param>
	static void emptySprite(spriteStr* Sprite, glm::vec3 color);

	static void drawSpritesPerPixel(screen& screenObj);

	static void updateSpriteBuffer(screen& screenObj);

	static void setFrameSprite(spriteStr* Sprite, int frame);

	//Helper functions
	static void colorIntToArray(unsigned int color, float* colorArray);
	static unsigned int colorArrayToInt(float* colorArray);

private:

};

