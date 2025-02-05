#pragma once

#include <malloc.h>
#include <glm.hpp>
#include "register.hpp"
#include "spritetransform.h"

//Holds all info of each sprite
class GLTexture;
struct spriteStr
{
public:

	friend class sprite;
	friend class spritetransform;
	

	const char* name = "None";
	unsigned int* buffer = nullptr;
	unsigned int* afterBuffer = nullptr; //Buffer after all changes were made
	int AfterBufferSize = 0;
	bool bufferChanged = false;
	float depth = 0.0f;
	int height = 0;
	int width = 0;
	GLTexture* texture = nullptr;
	int currentFrame = 0;
	int maxFrames = 0;

	Stransform localSpriteTransform{}; //Transform for local buffer

	const int getAfterWidthDif() { return afterWidthDifference; }
	const int getAfterHeightDif() { return afterHeightDifference; }
	const int getAfterWidth() { return afterWidth; }
	const int getAfterHeight() { return afterHeight; }

	spriteStr() {}

	~spriteStr()
	{
		if (initializedBuffer && buffer) _aligned_free(buffer); // free only if we allocated the buffer ourselves
		if (initializedBuffer && afterBuffer) delete[] afterBuffer;
	}

	//Moveable constructor since in the destructor we free and delete data.
	spriteStr(spriteStr&& other) noexcept : buffer(other.buffer), afterBuffer(other.afterBuffer) 
	{
		other.buffer = nullptr;
		other.afterBuffer = nullptr;
	}
	//Move assigment https://www.geeksforgeeks.org/move-assignment-operator-in-cpp-11/
	spriteStr& operator= (spriteStr&& other) noexcept 
	{
		if (this != &other)
		{
			//Free data
			if (initializedBuffer && buffer) _aligned_free(buffer);
			if (initializedBuffer && afterBuffer) delete[] afterBuffer;
			//Make other's data our own
			buffer = other.buffer;
			afterBuffer = other.afterBuffer;
			// reset 'other' to valid state
			other.buffer = nullptr;
			other.afterBuffer = nullptr;
		}
		return *this;
	}
	// Disable copying to prevent accidental double frees or deletes
	spriteStr(const spriteStr&) = delete; 
	spriteStr& operator=(const spriteStr&) = delete; 

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
	static void emptySprite(spriteStr* Sprite, glm::vec4 color);

	/// <summary>Checks if the Sprites need to be changed, needs to be called every frame</summary>
	static void updateSpriteBuffer();

	/// <summary>Changes local transform scale based on new width/height </summary>
	/// <param name="Sprite">Sprite struct</param>
	/// <param name="widht">new width in pixels</param>
	/// <param name="height">new height in pixels</param>
	static void setSpriteWidthHeight(spriteStr* Sprite, int widht, int height);

	static void setFrameSprite(spriteStr* Sprite, int frame);

	/// <summary>Creates Sprite with additional stransform and put it in the registry</summary>
	/// <param name="file">: File of image</param>
	/// <param name="name">: custom of sprite</param>
	/// <param name="depth">: Behind or in front other sprites</param>
	/// <param name="frames">: How many frames does the sprite have?</param>
	/// <param name="pos">: Position</param>
	/// <param name="scale">: Scale globally (using openGL)</param>
	/// <param name="widthHeight">: Set width or heigh locally for the buffer. To use default, set to {0,0}</param>
	/// <param name="rotation">: Set rotation for openGL</param>
	/// <returns></returns>
	static Entity createSpriteToRegistry(const char* file, const char* name = "none", float depth = 0.0f, int frames = 1, glm::vec2 pos = { 0,0 }, glm::vec2 scale = { 1,1 }, glm::vec2 widthHeight = { 0,0 }, float rotation = 0);

	//Helper functions
	static void colorIntToArray(unsigned int color, float* colorArray);
	static unsigned int colorArrayToInt(float* colorArray);

private:

};

