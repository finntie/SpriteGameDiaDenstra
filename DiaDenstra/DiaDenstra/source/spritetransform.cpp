#include "spritetransform.h"
#include "sprite.h"
#include <iostream>

void  spritetransform::applyTransform(int& x, int& y, std::pair<int,int>* fillPixels, spriteStr& Sprite, glm::vec2 scale, float rotation, glm::vec2 translation)
{

	//-------------------------------------Scaling------------------------------------------

	//------X-------
	int oldX = x;
	if (scale.x != 1.0f)
	{
		//Get new x position in float
		x = int(((float(x + 1) * scale.x))+0.5f) - 1;
	}
	//-------Y-------
	int oldY = y;
	if (scale.y != 1.0f)
	{
		//Get new x position in float
		y = int(((float(y + 1) * scale.y))+0.5f) - 1;
	}
	
	//Check if we skipped pixels, add those to the container
	int ArrayIndex = 1;


	float decimalX = (float(oldX + 1) * scale.x) - int(float(oldX + 1) * scale.x);
	//First check if decimal is greater than 0.5, only than we may skip a pixel
	//Than we check if decimal - scale.x decimal is smaller than 0.5. To check if previously we did not skip.
	//Example: int(0.8f + 0.5f) * (-1 * floor(0.8f - (1.2f - int(1.2f)) - 0.5f)
	//Becomes: 1 * (-1 * floor(0.8f - (0.2f) - 0.5f)
	//Becomes: 1 * (-1 * 0.0) == 0
	//This means that it is greater than 0.5, but it did skip a pixel previously
	int extraX = int(decimalX + 0.5f) * int(-1 * std::floor(decimalX - (scale.x - int(scale.x)) - 0.5f));
	extraX += int(scale.x) - 1; //Add scale (2.6 always skips 1) 

	//Same for y
	float decimalY = (float(oldY + 1) * scale.y) - int(float(oldY + 1) * scale.y);
	int extraY = int(decimalY + 0.5f) * int(-1 * std::floor(decimalY - (scale.y - int(scale.y)) - 0.5f));
	extraY += int(scale.y) - 1; //Add scale (2.6 always skips 1) 


	for (int yR = 0; yR < extraY + 1; yR++) //Add the + 1 so we always will loop at least once
	{
		for (int xR = 0; xR < extraX + 1; xR++)
		{
			fillPixels[ArrayIndex].first = x - xR;
			fillPixels[ArrayIndex].second = y - yR;
			ArrayIndex++;
		}
	}
	fillPixels[0] = std::make_pair(ArrayIndex, -1);
	
	

	//--------------------------------------------------Rotation-------------------------------------------------

	if (rotation != 0)
	{
		//Set variables
		const float halfSpriteX = Sprite.width * scale.x * 0.5f;
		const float halfSpriteY = Sprite.height * scale.y * 0.5f;
		const float cosTheta = std::cos(rotation);
		const float sinTheta = std::sin(rotation);

		//Rotate all points inside the deque
		for (int i = 1; i < ArrayIndex; i++) //Loop over all the values in the deque.
		{

			//Move the middle to (0,0) so we are rotation around the middle.
			float xOffset = float(fillPixels[i].first) - halfSpriteX;
			float yOffset = float(fillPixels[i].second) - halfSpriteY;
			//xOffset = 1;
			//yOffset = 1;

			// Apply the rotation formula
			float rotatedX = xOffset * cosTheta - yOffset * sinTheta;
			float rotatedY = xOffset * sinTheta + yOffset * cosTheta;

			//Move back
			fillPixels[i].first =  int(0.5f + (rotatedX + halfSpriteX));
			fillPixels[i].second = int(0.5f + (rotatedY + halfSpriteY));
		}
		
	}

	//--------------------------------------------------Translation-------------------------------------------------

	//Easiest 
	x += int(translation.x);
	y += int(translation.y);
}

void spritetransform::fillRotateGaps(spriteStr& Sprite, float rotation, glm::vec2 scale)
{
	//use barycentric coordinates

	//First create 2 triangles out of the sprite, watch the order
	glm::vec2 A = { Sprite.width * scale.x, 0 };
	glm::vec2 B = { 0, 0 };
	glm::vec2 C = { 0, Sprite.height * scale.y };
	glm::vec2 D = { A.x, C.y};

	//Rotate Triangles
	const float halfSpriteX = Sprite.width * scale.x * 0.5f;
	const float halfSpriteY = Sprite.height * scale.y * 0.5f;
	const float cosTheta = std::cos(rotation);
	const float sinTheta = std::sin(rotation);
	rotatePointFloat(A, rotation, cosTheta, sinTheta, halfSpriteX, halfSpriteY);
	rotatePointFloat(B, rotation, cosTheta, sinTheta, halfSpriteX, halfSpriteY);
	rotatePointFloat(C, rotation, cosTheta, sinTheta, halfSpriteX, halfSpriteY);
	rotatePointFloat(D, rotation, cosTheta, sinTheta, halfSpriteX, halfSpriteY);

	A += glm::vec2(Sprite.afterWidthDifference, Sprite.afterHeightDifference);
	B += glm::vec2(Sprite.afterWidthDifference, Sprite.afterHeightDifference);
	C += glm::vec2(Sprite.afterWidthDifference, Sprite.afterHeightDifference);
	D += glm::vec2(Sprite.afterWidthDifference, Sprite.afterHeightDifference);

	float u, v, w;
	//Repeat for every pixel inside a bounding box around the rotated triangle
	for (int y = 0; y < Sprite.afterHeight; y++)
	{
		for (int x = 0; x < Sprite.afterWidth; x++)
		{
			//Before doing all the calculation, why not check if there is a gap first?
			if (Sprite.afterBuffer[x + y * Sprite.afterWidth] == 0)
			{
				barycentric(float(x), float(y), A.x, A.y, B.x, B.y, C.x, C.y, u, v, w);
				if (u >= 0 && v >= 0 && w >= 0 && u + v + w - 1 < 0.001f)
				{
					//Point is inside this triangle
					//formula u = uAx + vBx + wCx
					//formula v = uAy + vBy + wCy
					//Where ABC is unrotated triangle
					int u2 = int(0.5f + (u * Sprite.width  + v * 0 + w * 0)); //corrisponding x and y for original image
					int v2 = int(0.5f + (u * 0 + v * 0 + w * Sprite.height ));

					u2 = std::min(Sprite.width - 1, int(u * Sprite.width)); //Clamp for stability
					v2 = std::min(Sprite.height - 1, int(w * Sprite.height)); //Clamp for stability
					//Now we can finally grab the corrisponding color
					Sprite.afterBuffer[x + y * Sprite.afterWidth] = Sprite.buffer[u2 + v2 * Sprite.width];
				}
				else
				{
					barycentric( float(x), float(y), C.x, C.y, D.x, D.y, A.x, A.y, u, v, w);
					if (u >= 0 && v >= 0 && w >= 0 && u + v + w - 1 < 0.001f)
					{
						int u2 = int(0.5f + (u * 0 + v * Sprite.width + w * Sprite.width)); //corrisponding x and y for original image
						int v2 = int(0.5f + (u * Sprite.height + v * Sprite.height + w * 0));
						//Now we can finally grab the corrisponding color
						if (u2 >= Sprite.width || v2 >= Sprite.height) continue;
						Sprite.afterBuffer[x + y * Sprite.afterWidth] = Sprite.buffer[u2 + v2 * Sprite.width];
					}
				}
			}
		}
	}

	float breakpoint = 2.0f;
	breakpoint = 1.0f;
}


void spritetransform::rotatePointFloat(glm::vec2& point, float rot, float cosTheta, float sinTheta, float halfSpriteX, float halfSpriteY)
{
	//Move the middle to (0,0) so we are rotation around the middle.
	float xOffset = point.x - halfSpriteX;
	float yOffset = point.y - halfSpriteY;

	point.x = halfSpriteX + (xOffset * cosTheta - yOffset * sinTheta);
	point.y = halfSpriteY + (xOffset * sinTheta + yOffset * cosTheta);
}



//Copied from https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
void spritetransform::barycentric(float px, float py, float ax, float ay, float bx, float by, float cx, float cy, float& u, float& v, float& w)
{
	float v0x = bx - ax, v1x = cx - ax, v2x = px - ax;
	float v0y = by - ay, v1y = cy - ay, v2y = py - ay;
	float den = v0x * v1y - v1x * v0y;
	v = (v2x * v1y - v1x * v2y) / den;
	w = (v0x * v2y - v2x * v0y) / den;
	u = 1.0f - v - w;
}