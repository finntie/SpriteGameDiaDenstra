#pragma once

#include <glm.hpp>
#include <string>
#include <vector>
#define DISABLE_WARNING_4267 __pragma(warning(push)) __pragma(warning(disable: 4267))
#define ENABLE_WARNINGS __pragma(warning(pop))

class tilemap
{

public:

	static void initTileMap(const char* tileMapImageFile, const char* tileMapFile, const char* vertexPossesFile);


private:

	static int* tileMapToArray(const char* tileMapFile, int& widht, int& height);
	static std::vector<std::pair<glm::vec2, glm::vec2>> verticesFileToArray(const char* verticesTileMapFile);
	static void triangulateArea(std::vector<std::vector<glm::vec2>> polygons, std::vector<std::vector<glm::vec2>> outsideArea);
};




