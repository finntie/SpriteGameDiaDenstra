#include "player.h"
#include "register.hpp"
#include "screen.h"
#include "sprite.h"
#include "spritetransform.h"
#include "physics.h"
#include "common.h"
#include "input.hpp"
#include "dance.h"

#include <string>
#include <queue>



void player::init(Dance& danceObj)
{
	//Packages
	//PlayerNumber | Pos.x | Pos.y
	danceObj.CreatePackage("OurPlayerPos", 0, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	//PlayerNumber | shot | bulletPos.x | bulletPos.y | bulletVel.x | bulletVel.y
	danceObj.CreatePackage("OurPlayerShot", 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	//PlayerNumber | OtherPlayer | PlayerRespawned
	danceObj.CreatePackage("OurPlayerHit", 0, 0, 0);


	//Set callback function
	danceObj.CreatePackageCallBackFunction("OurPlayerHit", [&](const std::string& data) {getPlayerHit(data); });
	danceObj.CreatePackageCallBackFunction("OurPlayerShot", [&](const std::string& data) {getPlayerShot(data); });
	danceObj.CreatePackageCallBackFunction("OurPlayerPos", [&](const std::string& data) {getPlayerPosses(data); });

}

void player::control(Dance& danceObj, float _dt)
{
	dt = _dt;
	jumpTimer -= dt;
	updateGunRotation -= dt;
	shootCooldown -= dt;
	deathCooldown -= dt;
	int frameCheck = checkedFrame1;

	for (const auto& [entity, player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		//Find own player
		if (player.playerNumber == ownPlayerNumber && player.health > 0.0f)
		{
			//-------------------------------------------Check Jump----------------------------------------------------
			//Check if can jump
			int2 Jump = physics::Physics().sensorTouchedBody(entity, jumpSensorID, frameCheck, physics::polygon);
			if (frameCheck != checkedFrame1)
			{
				checkedFrame1 = frameCheck;
				if (Jump.x > 0)
				{
					totalTouchingGroundAmount += Jump.x;
					//printf("totalTouch: %i\n", totalTouchingGroundAmount);
				}
				if (Jump.y > 0)
				{
					totalTouchingGroundAmount -= Jump.y;
					//printf("totalTouch: %i\n", totalTouchingGroundAmount);
				}
				if (totalTouchingGroundAmount > 0) canJump = true;
				else canJump = false;
				//Should Never happen, but you never know
				if (totalTouchingGroundAmount < 0) totalTouchingGroundAmount = 0;
			}
			transform.setVel({ 0, 0 });

			//-------------------------------------------Movement----------------------------------------------------

			if (input::Input().getKeyDown(input::Key::A))
			{
				transform.setVel({ -1, 0 });

			}
			if (input::Input().getKeyDown(input::Key::D))
			{
				transform.setVel({ 1, 0 });
			}
			if (input::Input().getKeyDown(input::Key::Space) && jumpTimer <= 0.0f && canJump)
			{
				physics::Physics().jump(entity);
				jumpTimer = 0.1f;
			}

			//-------------------------------------------Gun Movement----------------------------------------------------

			auto& gunTransform = Registry.get<Stransform>(player.gunEntity);
			auto& gunSprite = Registry.get<spriteStr>(player.gunEntity);

			//Set rotation
			glm::vec2 mousepos = input::Input().mousePos;
			mousepos.y = float(SCRHEIGHT) - mousepos.y;
			glm::vec2 dir = normalize(mousepos - gunTransform.getTranslation());
			float dirX = mousepos.x - transform.getTranslation().x;


			if (updateGunRotation <= 0.0f) //Prevent too many atan2 calculations.
			{
				gunTransform.setRotation(glm::degrees(atan2(dir.y, dir.x)));
				updateGunRotation = 0.04f;
			}

			//Set position
			if (dirX < 0) //Left of player
			{
				gunTransform.setTranslation({ transform.getTranslation().x - 25, transform.getTranslation().y });
				//Flip scale = Flipping sprite
				glm::vec2 scale = gunTransform.getScale();
				if (scale.y > 0)gunTransform.setScale({ scale.x , scale.y * -1 });
			}
			else //Right of player
			{
				gunTransform.setTranslation({ transform.getTranslation().x + 25, transform.getTranslation().y });
				//Flip scale = Flipping sprite
				glm::vec2 scale = gunTransform.getScale();
				if (scale.y < 0)gunTransform.setScale({ scale.x , scale.y * -1 });

			}

			//-------------------------------------------Gun Interaction----------------------------------------------------

			if (input::Input().getMouseButton(input::MouseButton::Left) && shootCooldown <= 0.0f)
			{
				shootCooldown = 0.05f; //Depends on gun

				//Create bullet
				Entity bulletEntity = sprite::createSpriteToRegistry("assets/basicbullet.png", "OwnBullet", 9.0f, 1, gunTransform.getTranslation());
				physics::Physics().drawOrder.push_back(bulletEntity);

				physics::Physics().createBullet(bulletEntity, gunTransform.getTranslation(), dir, 473.0f, 7.5f, ownPlayerNumber + 1);
				newBulletEntity = bulletEntity;
				shot = true;
			}


		}
		else if (player.playerNumber == ownPlayerNumber)
		{
			//Dead
			if (deathCooldown <= 0.0f)
			{
				//Respawn
				physics::Physics().moveEntityIn(entity, spawnPositions[rand() % (spawnPositions.size() - 1)]);
				player.health = 100.0f;
				totalTouchingGroundAmount = 0;
				//TODO: invernability

				danceObj.AddDataToParameter("OurPlayerHit", 0, ownPlayerNumber);
				danceObj.AddDataToParameter("OurPlayerHit", 1, ownPlayerNumber);
				danceObj.AddDataToParameter("OurPlayerHit", 2, 1);
				danceObj.sendPackage("OurPlayerHit");

			}
		}
	}
	//-------------------------------------------Bullets Interaction----------------------------------------------------

	std::queue<Entity> deleteEntities;

	//Loop over each bullet
	for (const auto& [bulletEntity, transform2, Sprite] : Registry.view<Stransform, spriteStr>().each())
	{
		if (Sprite.name == "OwnBullet" || Sprite.name.compare(0, 11, "OtherBullet") == 0)
		{
			//Get player number from the bullet
			int bulletNumber = -1;
			if (Sprite.name.compare(0, 11, "OtherBullet") == 0)
			{
				bulletNumber = std::stoi(&Sprite.name[11]);
			}
			else
			{
				bulletNumber = ownPlayerNumber;
			}

			int checkFrame = checkedFrame2;
			Entity otherEntity;
			bool hitObject = false;
			if (physics::Physics().getHitEntity(bulletEntity, otherEntity, checkFrame, hitObject, physics::ground))
			{
				if (checkFrame != checkedFrame2)
				{
					checkedFrame2 = frameCheck;
					//Hit occured
					auto& Sprite = Registry.get<spriteStr>(otherEntity);

					playerStr* otherPlayer = nullptr;
					otherPlayer = Registry.try_get<playerStr>(otherEntity);
					if (otherPlayer != nullptr)
					{
						printf("Player %i hit Player %i\n", bulletNumber, otherPlayer->playerNumber); //Debug only
					}
					if (bulletNumber == ownPlayerNumber)
					{
						//we hit someone
						danceObj.AddDataToParameter("OurPlayerHit", 0, ownPlayerNumber);
						danceObj.AddDataToParameter("OurPlayerHit", 1, otherPlayer->playerNumber);
						danceObj.AddDataToParameter("OurPlayerHit", 2, 0);
						danceObj.sendPackage("OurPlayerHit");
					}


					deleteEntities.push(bulletEntity);
				}
			}
			else if (hitObject) //We did not hit an entity, but we may hit the ground
			{
				if (checkFrame != checkedFrame2)
				{
					checkedFrame2 = frameCheck;
					deleteEntities.push(bulletEntity);
				}
			}
		}
	}
	//Delete them all
	while (!deleteEntities.empty())
	{
		Entity bulletEntity = deleteEntities.front();
		deleteEntities.pop();
		//delete from drawOrder
		for (int i = 0; i < physics::Physics().drawOrder.size(); i++)
		{
			if (physics::Physics().drawOrder[i] == bulletEntity)
			{
				physics::Physics().drawOrder.erase(physics::Physics().drawOrder.begin() + i);
				break;
			}
		}
		physics::Physics().destroyBody(bulletEntity);
		//Destroy bullet
		Registry.destroy(bulletEntity, 1);
	}
}

void player::updateOthers(Dance& danceObj, float dt)
{
	for (const auto& [entity, player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		//Find own player
		if (player.playerNumber == ownPlayerNumber)
		{
			//Pass our own position to everyone
			danceObj.AddDataToParameter("OurPlayerPos", 0, ownPlayerNumber);
			danceObj.AddDataToParameter("OurPlayerPos", 1, transform.getTranslation().x);
			danceObj.AddDataToParameter("OurPlayerPos", 2, transform.getTranslation().y);
			danceObj.sendPackage("OurPlayerPos");

			//Bullet interaction
			danceObj.AddDataToParameter("OurPlayerShot", 0, ownPlayerNumber);
			danceObj.AddDataToParameter("OurPlayerShot", 1, 0);
			if (shot)
			{
				danceObj.AddDataToParameter("OurPlayerShot", 1, 1);
				auto& bulletTransform = Registry.get<Stransform>(newBulletEntity);
				danceObj.AddDataToParameter("OurPlayerShot", 2, bulletTransform.getTranslation().x);
				danceObj.AddDataToParameter("OurPlayerShot", 3, bulletTransform.getTranslation().y);
				glm::vec2 dir = glm::normalize(physics::Physics().getVelocity(newBulletEntity));
				danceObj.AddDataToParameter("OurPlayerShot", 4, dir.x);
				danceObj.AddDataToParameter("OurPlayerShot", 5, dir.y);

				danceObj.sendPackage("OurPlayerShot");
			}
			shot = false;


		}
		else
		{
			physics::Physics().setBodyPos(player.targetPos, entity, dt);

			if (player.shot)
			{
				player.shot = false;
				//Create bullet of other player
				std::string name = "OtherBullet" + std::to_string(player.playerNumber);
				Entity bulletEntity = sprite::createSpriteToRegistry("assets/basicbullet.png", name.c_str(), 9.0f, 1, player.bulletPos);
				physics::Physics().drawOrder.push_back(bulletEntity);
				physics::Physics().createBullet(bulletEntity, player.bulletPos, player.bulletDir, 473.0f, 7.5f, player.playerNumber + 1);
			}
		}
	}
}

void player::getPlayerPosses(const std::string& data)
{
	int playerNumber = Dance::dataToVariable<int>(data, 2); //0 and 1 are not important


	for (const auto& [entity, player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		//Find the player
		if (player.playerNumber == playerNumber)
		{
			float x = Dance::dataToVariable<float>(data, 3); //0 and 1 are not important
			float y = Dance::dataToVariable<float>(data, 4); //0 and 1 are not important
			player.targetPos = { x, y };
		}
	}
}

void player::getPlayerShot(const std::string& data)
{
	int playerNumber = Dance::dataToVariable<int>(data, 2); //0 and 1 are not important

	for (const auto& [entity, player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		//Find the player
		if (player.playerNumber == playerNumber)
		{
			if (Dance::dataToVariable<int>(data, 3)) //If shot
			{
				glm::vec2 pos = { Dance::dataToVariable<float>(data, 4) , Dance::dataToVariable<float>(data, 5) };
				glm::vec2 dir = { Dance::dataToVariable<float>(data, 6) , Dance::dataToVariable<float>(data, 7) };

				player.shot = true;
				player.bulletPos = pos;
				player.bulletDir = dir;
			}
		}
	}
}

void player::getPlayerHit(const std::string& data)
{
	int playerNumber = Dance::dataToVariable<int>(data, 2); //Not used atm

	int otherPlayer = Dance::dataToVariable<int>(data, 3);

	bool respawned = bool(Dance::dataToVariable<int>(data, 4));


	for (const auto& [entity, player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		if (respawned)
		{
			if (player.playerNumber == playerNumber)
			{
				physics::Physics().moveEntityIn(entity, {0,0});
				player.health = 100.0f;
			}
		}
		else if (player.playerNumber == otherPlayer)
		{
			player.health -= 10.0f; //TODO: health taken is determined by gun type
			if (player.health <= 0.0f)
			{
				physics::Physics().moveEntityOut(entity);

				if (otherPlayer == player.playerNumber) deathCooldown = 2.5f;
			}
		}
	}

}


Entity player::createOwnPlayer(glm::vec2 pos, int number)
{
	Entity spriteEntity = sprite::createSpriteToRegistry("assets/Player.png", "OwnPlayer", 10.0f, 1, pos);
	auto& Sprite = Registry.get<spriteStr>(spriteEntity);
	Sprite.localSpriteTransform.setScale({ 0.5f, 0.5f });
	sprite::updateSpriteBuffer();
	auto& Player = CreateComponent<playerStr>(spriteEntity);
	Player.playerNumber = number;
	ownPlayerNumber = number;
	physics::Physics().createBody(spriteEntity, physics::capsule, physics::dynamicObj, pos, { {0,12.5},{0,-12.5},{12.5,12.5} }, physics::ownplayer, physics::none, number + 1);


	//Create sensor for ground detection to know when to jump.
	physics::Physics().createSensor(spriteEntity, physics::ownplayer, physics::ground, glm::vec2(0, -22.5f), 5.0f, jumpSensorID);

	//Create pistol entity
	Entity pistolEntity = sprite::createSpriteToRegistry("assets/glock.png", "Glock", 11.0f, 1, pos);
	Player.gunEntity = pistolEntity;

	//Create Joints
	physics::Physics().attachJoint(spriteEntity, pistolEntity, 0.5f, { 25,0 }, { -5,0 });


	return spriteEntity;
}

Entity player::createPlayerInstance(glm::vec2 pos, int number)
{

	Entity spriteEntity = sprite::createSpriteToRegistry("assets/Player.png", "Player", 10.0f, 1, pos);
	auto& Sprite = Registry.get<spriteStr>(spriteEntity);
	Sprite.localSpriteTransform.setScale({ 0.5f, 0.5f });
	sprite::updateSpriteBuffer();
	auto& Player = CreateComponent<playerStr>(spriteEntity);
	Player.playerNumber = number;
	physics::Physics().createBody(spriteEntity, physics::capsule, physics::kinematic, pos, { {0,12.5},{0,-12.5},{12.5,12.5} }, physics::player, physics::none, number + 1);

	return spriteEntity;
}
