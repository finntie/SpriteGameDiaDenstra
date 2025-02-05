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
};



class player
{
public:

	player() {};
	~player() {};

	void init();

	void control(float dt);

	//Difference is that this does create a sensor and the instance not
	Entity createOwnPlayer();
	static Entity createPlayerInstance();


private:

	int ownPlayerNumber = 0;

	float jumpTimer = 0.0f;
	int jumpSensorID = 0;
	int totalTouchingGroundAmount = 0;
	bool canJump = false;

	float updateGunRotation = 0.0f;
	float shootCooldown = 0.0f;
	int checkedFrame1 = 0;
	int checkedFrame2 = 0;
};

