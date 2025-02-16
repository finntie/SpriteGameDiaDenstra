#pragma once

class Shader;
class screen;
class sprite;
class player;
class Dance;
class gamesystem
{
public:

	gamesystem() {};
	~gamesystem() {};

	void init();

	void running(float dt);

	void drawSprites(Shader* shader);

	void shutdown() {};
	
	int windowPosx = 0;
	int windowPosy = 0;

	bool closeGame = false;
	screen* screenObj = nullptr;
	Dance* danceObj = nullptr;
private:

	sprite spriteObj;
	player playerObj;

	int gameStatus = 0; //0 = main screen | 1 = options | 2 = connection Menu | 3 = Join Menu | 4 = Host Menu | 5 = Wait Lobby | 6 = game | 7 = pause

	int danceMode = -1;

	bool gamestateInit = false;

};

