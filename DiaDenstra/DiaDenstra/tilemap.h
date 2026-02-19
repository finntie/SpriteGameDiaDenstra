#pragma once

#include "register.hpp"
#include <glm.hpp>
#include <string>
#include <vector>
#define DISABLE_WARNING_4267 __pragma(warning(push)) __pragma(warning(disable: 4267))
#define ENABLE_WARNINGS __pragma(warning(pop))

class tilemap
{

public:

	static Entity initTileMap(const char* tileMapImageFile, const char* tileMapFile, const char* vertexPossesFile);

	static std::vector<glm::vec2> initSpawnLocations(const char* spawnLocationFile, const char* tileMapImageFile);

private:

	static int* tileMapToArray(const char* tileMapFile, int& widht, int& height);
	static std::vector<std::pair<glm::vec2, glm::vec2>> verticesFileToArray(const char* verticesTileMapFile);
};




