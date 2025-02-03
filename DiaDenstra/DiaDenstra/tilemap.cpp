#include "tilemap.h"

#include "screen.h"
#include "sprite.h"
#include "spritetransform.h"
#include "register.hpp"
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


void tilemap::initTileMap(const char* tileMapImageFile, const char* tileMapFile, const char* vertexPossesFile)
{
	int width, height, widthTile, heightTile;
	std::vector<std::vector<glm::vec2>> MapShapes;
	std::list<std::pair<unsigned, unsigned>> VerticesToCheck;
	int* tileMap = tileMapToArray(tileMapFile, width, height); //Fill tilemap 
	std::vector<std::pair<glm::vec2, glm::vec2>> verticesPerTile = verticesFileToArray(vertexPossesFile); //Fill vertices corrisponding to tiles

	int n;
	unsigned char* data = stbi_load(tileMapImageFile, &widthTile, &heightTile, &n, 0);
	if (!data) { printf("error loading file\n"); return; } // load failed

	//Create sprite
	Entity spriteEntity = Registry.create();
	spriteStr& Sprite = CreateComponent<spriteStr>(spriteEntity);
	Sprite.depth = 0.0f;
	physics::Physics().drawOrder.push_back(spriteEntity);
	auto& transform = CreateComponent<Stransform>(spriteEntity);
	transform.setScale(glm::vec2(1.4, 1.12));
	sprite::initEmpty(&Sprite, 1, "TileMapSprite", width * heightTile, height * heightTile); //Heighttile is the size of the tile
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

	//Triangulate the tilemap and put it in the physics class.
	triangulateArea(MapShapes, Bounds);

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

void tilemap::triangulateArea(std::vector<std::vector<glm::vec2>> polygons, std::vector<std::vector<glm::vec2>> outsideArea)
{

	// Create triangulation, this will be passed into the registry.
	CDT::Triangulation<double> t;

	// Load path from file
	Clipper2Lib::PathsD Path1;
	Clipper2Lib::PathsD Path2;
	Clipper2Lib::PathsD solution;

	std::vector<CDT::V2d<double>> vertices;
	CDT::EdgeVec edges;

	//Checks for the polygons
	for (size_t i = 0; i < polygons.size(); i++)
	{
		//Check if they are clockwise
		if (!geometry2d::IsClockwise(polygons[i]))
		{
			std::reverse(polygons[i].begin(), polygons[i].end());
		}
		//Check if its the outside map, if so, put it in front.
		if (i > 1)
		{
			//Check with random point against for polygon if point is inside it, if so, polygon 0 is the outline
			if (!geometry2d::IsPointInsidePolygon(polygons[i][0], polygons[0]))
			{
				std::iter_swap(polygons.begin(), polygons.begin() + i); //Repeat until outline is in the front.
			}
		}
	}
	//Reverse first shape
	std::reverse(polygons[0].begin(), polygons[0].end());

	physics::Physics().MapShapes = polygons;

	//Go back
	std::reverse(polygons[0].begin(), polygons[0].end());

	// Convert polygons to path2
	for (size_t i = 0; i < polygons.size(); i++)
	{

		Clipper2Lib::PathD path;
		for (size_t j = 0; j < polygons[i].size(); j++)
		{
			Clipper2Lib::PointD pointD(polygons[i][j].x, polygons[i][j].y);  // Convert glm::vec2 to PointD
			path.push_back(pointD);
		}
		Path2.push_back(path);
	}

	// Convert outsideArea to path1
	for (size_t i = 0; i < outsideArea.size(); i++)
	{
		if (geometry2d::IsClockwise(outsideArea[i]))
		{
			std::reverse(outsideArea[i].begin(), outsideArea[i].end());
		}

		Clipper2Lib::PathD path;
		for (size_t j = 0; j < outsideArea[i].size(); j++)
		{
			Clipper2Lib::PointD pointD(outsideArea[i][j].x, outsideArea[i][j].y);  // Convert glm::vec2 to PointD
			path.push_back(pointD);
		}
		Path1.push_back(path);
	}

	// get the difference of these two, this will remove the playing field
	solution = Difference(Path1, Path2, Clipper2Lib::FillRule::NonZero);
	//Remove the outline from Path2
	Path2.erase(Path2.begin());
	//Now union the result 
	solution = Clipper2Lib::Union(solution, Path2, Clipper2Lib::FillRule::NonZero);

	//Simplify?
	solution = Clipper2Lib::SimplifyPaths(solution, 2.0);  

	int n = 0;

	// Add solution to CDT
	for (int i = 0; i < int(solution.size()); i++)
	{
		for (int j = 0; j < int(solution[i].size()); j++)
		{
			n++;
			vertices.push_back({ solution[i][j].x, solution[i][j].y });
			if (j < int(solution[i].size()) - 1)
			{
				edges.push_back(CDT::Edge(n - 1, n));
			}
		}
		edges.push_back(CDT::Edge(n - int(solution[i].size()), int(solution[i].size()) - 1 + n - int(solution[i].size())));
	}

	// Add edges and vertices and remove the outertriangles and holes
	 RemoveDuplicatesAndRemapEdges(vertices, edges);
	t.insertVertices(vertices);
	t.insertEdges(edges);
	t.eraseOuterTrianglesAndHoles();


	physics::Physics().Triangles = t;

}
