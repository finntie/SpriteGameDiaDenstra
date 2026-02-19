#include "tilemap.h"

#include "screen.h"
#include "sprite.h"
#include "spritetransform.h"
#include "physics.h"
#include "common.h"

DISABLE_WARNING_4267
#include "CDT.h"
ENABLE_WARNINGS
#include "clipper2/clipper.h"
#include "geometry2d.hpp"
#include <stb_image.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <list>


Entity tilemap::initTileMap(const char* tileMapImageFile, const char* tileMapFile, const char* vertexPossesFile)
{
	int width, height, widthTile, heightTile;
	std::vector<std::vector<glm::vec2>> MapShapes;
	std::list<std::pair<unsigned, unsigned>> VerticesToCheck;
	int* tileMap = tileMapToArray(tileMapFile, width, height); //Fill tilemap 
	std::vector<std::pair<glm::vec2, glm::vec2>> verticesPerTile = verticesFileToArray(vertexPossesFile); //Fill vertices corrisponding to tiles

	int n;
	unsigned char* data = stbi_load(tileMapImageFile, &widthTile, &heightTile, &n, 0);
	if (!data) { printf("error loading file\n"); return entt::null; } // load failed

	//Create sprite
	Entity spriteEntity = Registry.create();
	spriteStr& Sprite = CreateComponent<spriteStr>(spriteEntity);
	Sprite.depth = 0.0f;
	physics::Physics().drawOrder.push_back(spriteEntity);
	sprite::initEmpty(&Sprite, 1, "TileMapSprite", width * heightTile, height * heightTile); //Heighttile is the size of the tile
	auto& transform = CreateComponent<Stransform>(spriteEntity);
	transform.setTranslation({ Sprite.width * 0.5f, -Sprite.height * 0.5f });
	float inverseSize = 1.0f / float(heightTile);
	//Load temporarily buffer
	spriteStr tileMapSprite;
	sprite::initFromFile(&tileMapSprite, tileMapImageFile, int(float(widthTile) / float(heightTile) + 0.5f), "temporarysprite");

	//Now we can easily grab each frame for each tile
	for (int y = 0; y < Sprite.height; y += heightTile)
	{
		for (int x = 0; x < Sprite.width; x += heightTile) //Skip sprite length
		{
			const int frame = tileMap[int(float(x) * inverseSize + 0.5f) + int(float(y) * inverseSize + 0.5f) * width];

			if (frame != -1)
			{
				for (int y2 = 0; y2 < heightTile; y2++) //one tile at a time
				{
					//Copy line data into the sprite
					memcpy(&Sprite.buffer[x + (y2 + y) * Sprite.width], &tileMapSprite.buffer[(frame * heightTile) + y2 * widthTile], sizeof(unsigned) * heightTile);
				}

				if (verticesPerTile[frame].first.x != -1)
				{
					//Based on frame, add correct vertices to the list
					std::pair<unsigned, unsigned> line;
					line.first = unsigned(x + heightTile * verticesPerTile[frame].first.x) + unsigned(y + heightTile * verticesPerTile[frame].first.y) * Sprite.width;
					line.second = unsigned(x + heightTile * verticesPerTile[frame].second.x) + unsigned(y + heightTile * verticesPerTile[frame].second.y) * Sprite.width;
					VerticesToCheck.push_back(line);
				}
			}
		}
	}

	//Order the lines so they form polygons
	bool found = false;
	int safetyMax = int(VerticesToCheck.size() * VerticesToCheck.size()) + 1; //Should never loop more than this
	int i = 0;
	std::vector<glm::vec2> MapVertices;
	unsigned checkAgainst = 0;


	while (VerticesToCheck.size() > 0) //Create all shapes
	{
		checkAgainst = VerticesToCheck.front().second; //Start with first line, second vertexPos
		//Add first one
		MapVertices.push_back(glm::vec2(VerticesToCheck.front().first % Sprite.width, VerticesToCheck.front().first / Sprite.width));
		MapVertices.push_back(glm::vec2(VerticesToCheck.front().second % Sprite.width, VerticesToCheck.front().second / Sprite.width));

		//remove from the list so we don't check it anymore
		VerticesToCheck.pop_front();

		while (true) //Create a shape
		{
			for (auto it = VerticesToCheck.begin(); it != VerticesToCheck.end();) //Create a line
			{
				i++; //Could be wrong placed
				if (i > safetyMax) break; //Safety break
				if (checkAgainst != it->first)
				{
					if (checkAgainst != it->second)
					{
						it++;
						continue;
					}
					else
					{
						checkAgainst = it->first; //We need to check for the other vertex in the line. (A=A-B=B-C)
						MapVertices.push_back(glm::vec2(it->first % Sprite.width, it->first / Sprite.width));
					}
				}
				else
				{
					checkAgainst = it->second;
					MapVertices.push_back(glm::vec2(it->second % Sprite.width, it->second / Sprite.width));
				}
				found = true;
				VerticesToCheck.erase(it); //Delete
				break;
			}
			if (!found || i > safetyMax)
			{
				//If we did not find the match, we should have created the polygon (first line and last line should match)
				//We check to be certain
				if (checkAgainst != MapVertices[0].x + MapVertices[0].y * Sprite.width)
				{
					printf("Uh Oh... the tilemap Polygons don't line up (or the max safety it too low)\n");
				}
				break;
			}
			found = false;
		}
		if (i > safetyMax) break; //Safety-Break

		//Should have created a polygon if nothing went wrong
		MapShapes.push_back(MapVertices);
		MapVertices.clear(); //Re-use
		found = false;


	}
	std::vector<std::vector<glm::vec2>> Bounds;
	std::vector<glm::vec2> Binds = { {0,0}, {SCRWIDTH, 0}, {SCRWIDTH, SCRHEIGHT},{0, SCRHEIGHT } };
	Bounds.push_back(Binds);

	//Do some last checks with the polygons before sending it over
	
	//Checks for the polygons
	for (size_t i = 0; i < MapShapes.size(); i++)
	{
		//Check if they are clockwise
		if (!geometry2d::IsClockwise(MapShapes[i]))
		{
			std::reverse(MapShapes[i].begin(), MapShapes[i].end());
		}
		//Check if its the outside map, if so, put it in front.
		if (i > 1)
		{
			//Check with random point against for polygon if point is inside it, if so, polygon 0 is the outline
			if (!geometry2d::IsPointInsidePolygon(MapShapes[i][0], MapShapes[0]))
			{
				std::iter_swap(MapShapes.begin(), MapShapes.begin() + i); //Repeat until outline is in the front.
			}
		}
	}
	//Reverse first shape
	std::reverse(MapShapes[0].begin(), MapShapes[0].end());

	//Send it
	physics::Physics().MapShapes = MapShapes;

	return spriteEntity;
}

std::vector<glm::vec2> tilemap::initSpawnLocations(const char* spawnLocationFile, const char* tileMapImageFile)
{
	int width = 0, height = 0;
	int* tileMap = tileMapToArray(spawnLocationFile, width, height); //Fill tilemap 

	int widthTile = 0, heightTile = 0;
	int n;
	unsigned char* data = stbi_load(tileMapImageFile, &widthTile, &heightTile, &n, 0);
	if (!data) { printf("error loading file\n"); return std::vector<glm::vec2>(); } // load failed

	std::vector<glm::vec2> output{};

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (tileMap[x + y * width] != -1)
			{
				//Player spawn tile
				output.push_back(glm::vec2(x * heightTile, -y * heightTile));
			}
		}
	}
	return output;
}

int* tilemap::tileMapToArray(const char* tileMapFile, int& width, int& height)
{
	width = 0;
	height = 0;
	int* output = nullptr;

	std::ifstream file(tileMapFile);
	if (!file)
	{
		printf("File opening error\n");
		return output;
	}
	//First check how big our file is
	std::string line;
	while (file >> line)
	{
		std::stringstream ss(line);
		std::string word;
		width = 0;
		height++;

		while (std::getline(ss, word, ','))
		{
			width++;
		}
	}
	//Create buffer
	output = new int[width * height];
	int i = 0;
	//Fill it
	file = std::ifstream(tileMapFile);
	if (!file)
	{
		printf("File opening error\n");
		return output;
	}
	while (file >> line)
	{
		std::stringstream ss(line);
		std::string word;
		while (std::getline(ss, word, ','))
		{
			output[i++] = std::stoi(word);
		}
	}

	return output;
}

std::vector<std::pair<glm::vec2, glm::vec2>> tilemap::verticesFileToArray(const char* verticesTileMapFile)
{
	int amount = 0;
	std::vector<std::pair<glm::vec2, glm::vec2>> output{};

	std::ifstream file(verticesTileMapFile);
	if (!file)
	{
		printf("File opening error\n");
		return output;
	}
	//First check how big our file is
	std::string line;
	while (file >> line)
	{
		amount++;
	}
	//Create buffer
	output.resize(amount);
	int i = 0;
	//Fill it
	file = std::ifstream(verticesTileMapFile);
	if (!file)
	{
		printf("File opening error\n");
		return output;
	}
	while (file >> line)
	{
		std::stringstream ss(line);
		std::string word;
		int wordi = 0;
		while (std::getline(ss, word, ','))
		{
			// 0.5, 1, 1, 0
			std::pair<glm::vec2, glm::vec2> p;
			float value = std::stof(word);

			if (wordi < 2)
			{
				if (value == -1)
				{
					output[i].first = { -1, -1 };
					output[i].second = { -1, -1 };
				}
				else
				{
					if (wordi == 0) output[i].first.x = value;
					else output[i].first.y = value;
				}
			}
			else
			{
				if (wordi == 2) output[i].second.x = value;
				else output[i].second.y = value;
			}
			wordi++;
		}
		i++;
	}

	return output;
}


