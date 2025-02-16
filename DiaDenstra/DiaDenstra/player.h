#pragma once
#include "register.hpp"
#include <glm.hpp>
struct playerStr
{
	float health = 100.0f;
	float speed = 10.0f;
	int playerNumber = 0;
	Entity gunEntity{};
	glm::vec2 gunDir{0,1};

	//Other players
	glm::vec2 targetPos{0,0};
	bool shot = false;
	glm::vec2 bulletPos{ 0, 0 };
	glm::vec2 bulletDir{ 0,0 };

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

	void getPlayerPosses(const std::string& data);

	void getPlayerShot(const std::string& data);

	void getPlayerHit(const std::string& data);

	//Difference is that this does create a sensor and the instance not
	Entity createOwnPlayer(glm::vec2 pos, int number);
	static Entity createPlayerInstance(glm::vec2 pos, int number);

	bool isOwnPlayer(int number) { return number == ownPlayerNumber; }


	std::vector<glm::vec2> spawnPositions;

private:

	int ownPlayerNumber = 0;

	float jumpTimer = 0.0f;
	int jumpSensorID = 0;
	int totalTouchingGroundAmount = 0;
	bool canJump = false;
	bool shot = false;
	Entity newBulletEntity;
	float deathCooldown = 5.0f;

	float updateGunRotation = 0.0f;
	float shootCooldown = 0.0f;
	int checkedFrame1 = 0;
	int checkedFrame2 = 0;
	float dt = 0;
};

