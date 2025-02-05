#include "sprite.h"

#include "spritetransform.h"
#include "opengl.h"
#include "register.hpp"
#include "screen.h"

#include <stb_image.h>


void sprite::initFromFile(spriteStr* Sprite, const char* file, const int frames, const char* name)
{
	// use stb_image to load the image file
	int n;
	unsigned char* data = stbi_load(file, &Sprite->width, &Sprite->height, &n, 0);
	if (!data)
	{
		printf("Loading file failed\n");
		return; // load failed
	}
	//Check if frames adds up to width
	if (Sprite->width % frames > 0) printf("Width not dividable with amount of frames\n");

	Sprite->name = name;
	Sprite->buffer = (unsigned int*)_aligned_malloc(Sprite->width * Sprite->height * sizeof(unsigned int), 64);
	if (Sprite->buffer == NULL)
	{
		printf("Error trying to init file\n");
		return;
	}
	Sprite->initializedBuffer = true; //We need to delete buffer in destructor
	const int s = Sprite->width * Sprite->height;
	if (n == 1) /* greyscale */ for (int i = 0; i < s; i++)
	{
		const unsigned char p = data[i];
		Sprite->buffer[i] = p + (p << 8) + (p << 16);
	}
	else if (n == 3) //RGB
	{
		for (int i = 0; i < s; i++) Sprite->buffer[i] = (data[i * n + 0] << 16) + (data[i * n + 1] << 8) + data[i * n + 2] + (255 << 24);
	}
	else //RGBA
	{
		for (int i = 0; i < s; i++) Sprite->buffer[i] = (data[i * n + 3] << 24) + (data[i * n + 0] << 16) + (data[i * n + 1] << 8) + data[i * n + 2];
	}
	//Add Stransform
	Sprite->localSpriteTransform = Stransform();
	Sprite->maxFrames = frames;
	int frameOffset = Sprite->width;
	Sprite->width /= frames;
	//Copy only part to afterBuffer
	Sprite->afterBuffer = new unsigned int[Sprite->width * Sprite->height];
	Sprite->AfterBufferSize = Sprite->width * Sprite->height;
	for (int y = 0; y < Sprite->height; y++)
	{
		for (int x = 0; x < Sprite->width; x++)
		{
			Sprite->afterBuffer[x + y * Sprite->width] = Sprite->buffer[x + y * frameOffset];
		}
	}

	// free stb_image data
	stbi_image_free(data);

	//Create Texture
	Sprite->texture = new GLTexture(Sprite->width, Sprite->height, GLTexture::DEFAULT);
}

void sprite::initEmpty(spriteStr* Sprite, const int frames, const char* name, int width, int height)
{
	Sprite->width = width;
	Sprite->height = height;

	Sprite->buffer = (unsigned int*)_aligned_malloc(Sprite->width * Sprite->height * sizeof(unsigned int), 64);
	if (Sprite->buffer == NULL)
	{
		printf("Error tryint to init file\n");
		return;
	}
	Sprite->initializedBuffer = true; //We need to delete buffer in destructor
	const int s = Sprite->width * Sprite->height;
	for (int i = 0; i < s; i++) Sprite->buffer[i] = (0 << 24) + (255 << 16) + (255 << 8) + 255; //Whole Sprite White

	Sprite->maxFrames = frames;
	Sprite->name = name;

	int frameOffset = Sprite->width;
	Sprite->width /= frames;
	//Copy only part to afterBuffer
	Sprite->afterBuffer = new unsigned int[Sprite->width * Sprite->height];
	Sprite->AfterBufferSize = Sprite->width * Sprite->height;
	for (int y = 0; y < Sprite->height; y++)
	{
		for (int x = 0; x < Sprite->width; x++)
		{
			Sprite->afterBuffer[x + y * Sprite->width] = Sprite->buffer[(x + frameOffset) + y * frameOffset];
		}
	}
	//Create Texture
	Sprite->texture = new GLTexture(Sprite->width, Sprite->height, GLTexture::DEFAULT);
}

void sprite::emptySprite(spriteStr* Sprite, glm::vec4 color)
{
	const int s = Sprite->width * Sprite->maxFrames * Sprite->height;
	for (int i = 0; i < s; i++) Sprite->buffer[i] = (int(color.w * 255) << 24) + (int(color.x * 255) << 16) + (int(color.y * 255) << 8) + int(color.z * 255); //Whole Sprite White
}


void sprite::updateSpriteBuffer()
{
	for (const auto& [spriteEntity, Sprite] : Registry.view<spriteStr>().each())
	{
		Stransform& transform = Sprite.localSpriteTransform;

		//Check if scale or rotation changed
		if (transform.scaleChanged || transform.rotationChanged || Sprite.frameChanged)
		{
			Sprite.bufferChanged = true;
			float rot = transform.getRotation();
			glm::vec2 scale = transform.getScale();


			//Due to rotation, the sprite can increase in width and height
			if (rot != 0)
			{
				//Size = w * cos(n) + h * sin(n)
				Sprite.afterWidth =  int(ceil((Sprite.width * scale.x) * std::abs(glm::cos(rot)) + (Sprite.height * scale.y) * std::abs(glm::sin(rot)))) + 2;
				Sprite.afterHeight = int(ceil((Sprite.width * scale.x) * std::abs(glm::sin(rot)) + (Sprite.height * scale.y) * std::abs(glm::cos(rot)))) + 2;
				//Also add + 2 to add extra space (needed)
			}
			else
			{
				Sprite.afterWidth = int(0.5f + (Sprite.width * scale.x));
				Sprite.afterHeight = int(0.5f + (Sprite.height * scale.y));
			}
			int newSizeBuffer = Sprite.afterWidth * Sprite.afterHeight;

			if (newSizeBuffer != Sprite.AfterBufferSize)
			{
				//Change size
				if (Sprite.AfterBufferSize != 0)
				{
					delete[] Sprite.afterBuffer; //Dont forget to delete the old one.
					Sprite.afterBuffer = nullptr;
				}
				Sprite.afterBuffer = new unsigned int[newSizeBuffer];
				Sprite.AfterBufferSize = newSizeBuffer;
			}
			//Clear buffer first
			for (int i = 0; i < Sprite.afterHeight * Sprite.afterWidth; i++)
			{
				Sprite.afterBuffer[i] = 0;
			}

			//Fill this buffer
			float offset = scale.x > scale.y ? scale.x * 0.42f : scale.y * 0.42f;
			glm::vec2 trans = { offset,offset };//Fill translation with max rotation offset.
			int maxAmountPixels = int(ceil(scale.x) * ceil(scale.y) + 1);
			std::pair<int, int>* fillPixelArray = new std::pair<int, int>[maxAmountPixels];
			
			for (int y = 0; y < Sprite.height; y++)
			{
				for (int x = 0; x < Sprite.width; x++)
				{
					//Get the transformed pixel
					int Bx = x, By = y;
					unsigned color = Sprite.buffer[(x + Sprite.width * Sprite.currentFrame) + y * Sprite.width * Sprite.maxFrames];
					spritetransform::applyTransform(Bx, By, fillPixelArray, Sprite, scale, rot, trans);
					for (int i = 1; i < fillPixelArray[0].first; i++) //Loop over all the pixels
					{
						int Px = fillPixelArray[i].first;
						int Py = fillPixelArray[i].second;
						//Translate due to rotation.
						if (rot != 0)
						{
							Sprite.afterWidthDifference = int(0.5f + ((float(Sprite.afterWidth) - float(Sprite.width) * transform.getScale().x) * 0.5f));
							Sprite.afterHeightDifference = int(0.5f + ((float(Sprite.afterHeight) - float(Sprite.height) * transform.getScale().y) * 0.5f));
							Px += Sprite.afterWidthDifference;
							Py += Sprite.afterHeightDifference;
						}

						//Fill the array
						if (Px < 0 || Py < 0 || Px >= Sprite.afterWidth || Py >= Sprite.afterHeight)
						{
							printf("Image drawn outside bounds, should not happen\n");
						}
						else Sprite.afterBuffer[Px + Py * Sprite.afterWidth] = color;
					}
				}
			}
			//Now we want to fill in the pixels when rotated, because of the rotation, we could have gaps
			if (transform.rotationChanged)
			{
				spritetransform::fillRotateGaps(Sprite, rot, scale);
			}

			transform.scaleChanged = false;
			transform.rotationChanged = false;
			Sprite.frameChanged = false;

			delete[] fillPixelArray;


			//change the texture to the vector
			delete Sprite.texture;
			Sprite.texture = nullptr;
			Sprite.texture = new GLTexture(Sprite.afterWidth, Sprite.afterHeight, GLTexture::DEFAULT);

		}
	}
}

void sprite::setSpriteWidthHeight(spriteStr* Sprite, int width, int height)
{
	float xScale = float(width) /  float(Sprite->width);
	float yScale = float(height) / float(Sprite->height);
	Sprite->localSpriteTransform.setScale({ xScale, yScale });
}

void sprite::setFrameSprite(spriteStr* Sprite, int frame) 
{
	if (frame >= 0 && frame < Sprite->maxFrames)
	{
		Sprite->currentFrame = frame;
		Sprite->frameChanged = true;
	}
	else
	{
		printf("set-frame not valid\n");
	}
}

Entity sprite::createSpriteToRegistry(const char* file, const char* name, float depth, int frames, glm::vec2 pos, glm::vec2 scale, glm::vec2 widthHeight, float rotation)
{
	Entity spriteEntity = Registry.create();
	auto& Sprite = CreateComponent<spriteStr>(spriteEntity); 
	Sprite.depth = depth; 
	initFromFile(&Sprite, file, frames, name);
	if (widthHeight.x > 0 && widthHeight.y > 0)
	{
		setSpriteWidthHeight(&Sprite, int(widthHeight.x), int(widthHeight.y));
	}
	auto& transform = CreateComponent<Stransform>(spriteEntity);
	transform.setScale(scale); 
	transform.setTranslation(pos); 
	transform.setRotation(rotation); 
	updateSpriteBuffer();
	return spriteEntity;
}

void sprite::colorIntToArray(unsigned int color, float* colorArray)
{
	//int x = color >> 24;
	//int y = (color >> 16) - (x << 8);
	//int z = (color >> 8) - (x << 16) - (y << 8);

	colorArray[0] = float((color >> 24) & 0xFF) / 255.0f;
	colorArray[1] = float((color >> 16) & 0xFF) / 255.0f;
	colorArray[2] = float((color >> 8) & 0xFF) / 255.0f;
	colorArray[3] = float(color & 0xFF) / 255.0f;
}

unsigned int sprite::colorArrayToInt(float* colorArray)
{
	return (unsigned(colorArray[0] * 255) << 24) + (unsigned(colorArray[1] * 255) << 16) + (unsigned(colorArray[2] * 255) << 8) + unsigned(colorArray[3] * 255);
}
