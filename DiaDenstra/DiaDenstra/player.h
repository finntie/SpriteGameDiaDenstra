#pragma once
#include "register.hpp"
#include <glm.hpp>

struct gunInfo
{
	Entity gunEntity{};
	enum type
	{
		HAND, PISTOL, SHOTGUN, MINIGUN, SNIPER, BOUNCER, TPPEARL
	};
	bool left = false;
	std::vector<int> inventory{ 0 };
	int activeType = 0; //Index in inventory
	float shootCooldown = 0.0f;
	float maxCooldown = 0.5f;
	float maxReloadTime = 1.5f;
	std::vector<int> bulletsLeft{ 10 }; //Same as inventory
};

struct playerStr
{
	float health = 100.0f;
	float speed = 10.0f;
	int playerNumber = 0;
	gunInfo gun;
	std::string playerName = "Empty";

	//Other players
	glm::vec2 targetPos{0,0};
	bool shot = false;
	glm::vec2 bulletPos{ 0, 0 };
	glm::vec2 bulletDir{ 0,0 };
	int bulletType = 0;
	int healthStatus = 0; //0 = nothing | 1 = respawn | 2 = died

};

struct bulletStr
{
	int health = 1;
	float lifetime = 32.0f; //32 seconds is long enough
	float hitCooldown = 0.02f;
	int type = 0;
};

class Dance;
class player
{
public:

	player() {};
	~player() {};

	void init(Dance& danceObj);

	void control(Dance& danceObj, float dt);

	void updateOthers(Dance& danceObj, float dt);

	void endScreen();

	void getPlayerPosses(const std::string& data);

	void getPlayerShot(const std::string& data);

	void getPlayerHit(const std::string& data);

	//Difference is that this does create a sensor and the instance not
	Entity createOwnPlayer(glm::vec2 pos, int number);
	static Entity createPlayerInstance(glm::vec2 pos, int number);

	bool isOwnPlayer(int number) { return number == ownPlayerNumber; }
	int getOwnPlayerNumber() { return ownPlayerNumber; };

	static void writeName(const char* name);
	static std::string getName(bool random);
	static void setName(int playerNumber, const char* name);

	std::vector<glm::vec2> spawnPositions;

private:

	void initGunType(playerStr& Player, gunInfo::type gunType);

	Entity createHandAABB(glm::vec2 pos, std::string name, int playerNumber);
	Entity createBullet(glm::vec2 pos, float direction, std::string name, int Type, int playerNumber, float speed = 300.0f, int health = 1, float lifetime = 32.0f);

	int ownPlayerNumber = 0;

	float jumpTimer = 0.0f;
	int jumpSensorID = 0;
	int totalTouchingGroundAmount = 0;
	bool canJump = false;
	bool shot = false;
	Entity newBulletEntity;
	int newBulletType = 0;
	float deathCooldown = 5.0f;

	float updateGunRotation = 0.0f;
	float shootCooldown = 0.0f;
	float pearlCooldown = 0.0f;
	int typeReloading = -1;
	int checkedFrame1 = 0;
	int checkedFrame2 = 0;
	int handSensorID = 5;
	float dt = 0;

	int bulletDamage[7] = { 30, 15, 15, 5, 55, 20, 0 }; //Custom set damage per type
	int maxBullets[7] = { 1, 7, 4, 50, 5, 7, 1 }; //Custom set bullets per type
};

