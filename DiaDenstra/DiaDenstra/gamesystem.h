#pragma once

class Shader;
class screen;
class sprite;
class player;
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
	screen* screenObj;
private:
	sprite spriteObj;
	player playerObj;
	float test = 0;

};

