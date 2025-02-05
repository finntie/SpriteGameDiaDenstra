#include "player.h"
#include "register.hpp"
#include "screen.h"
#include "sprite.h"
#include "spritetransform.h"
#include "physics.h"
#include "common.h"
#include "input.hpp"

#include <string>
#include <queue>

void player::init()
{
}

void player::control(float dt)
{
	jumpTimer -= dt;
	updateGunRotation -= dt;
	shootCooldown -= dt;

	for (const auto& [entity, player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		//Find own player
		if (player.playerNumber == ownPlayerNumber)
		{
			//-------------------------------------------Check Jump----------------------------------------------------
			//Check if can jump
			int frameCheck = checkedFrame1;
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
				shootCooldown = 0.1f; //Depends on gun

				//Create bullet
				Entity bulletEntity = sprite::createSpriteToRegistry("assets/basicbullet.png", "OwnBullet", 9.0f, 1, gunTransform.getTranslation());
				physics::Physics().drawOrder.push_back(bulletEntity);

				physics::Physics().createBullet(bulletEntity, gunTransform.getTranslation(), dir, 473.0f, 7.5f);

			}

			//-------------------------------------------Bullets Interaction----------------------------------------------------

			std::queue<Entity> deleteEntities;

			//Loop over each bullet
			for (const auto& [bulletEntity, transform, Sprite] : Registry.view<Stransform, spriteStr>().each())
			{
				if (Sprite.name == "OwnBullet")
				{
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
							printf(Sprite.name); //Debug only

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
	}
}

Entity player::createOwnPlayer()
{
	Entity spriteEntity = sprite::createSpriteToRegistry("assets/Player.png", "OwnPlayer", 10.0f, 1, { 480, 630 });
	auto& Sprite = Registry.get<spriteStr>(spriteEntity);
	Sprite.localSpriteTransform.setScale({ 0.5f, 0.5f });
	sprite::updateSpriteBuffer();
	auto& Player = CreateComponent<playerStr>(spriteEntity);
	physics::Physics().createBody(spriteEntity, physics::capsule, physics::dynamicObj, { 480, 630 }, { {0,12.5},{0,-12.5},{12.5,12.5} }, physics::ownplayer);


	//Create sensor for ground detection to know when to jump.
	physics::Physics().createSensor(spriteEntity, physics::ownplayer, physics::ground, glm::vec2(0, -22.5f), 5.0f, jumpSensorID);

	//Create pistol entity
	Entity pistolEntity = sprite::createSpriteToRegistry("assets/glock.png", "Glock", 11.0f, 1, { 500, 630 });
	Player.gunEntity = pistolEntity;

	//Create Joints
	physics::Physics().attachJoint(spriteEntity, pistolEntity, 0.5f, { 25,0 }, { -5,0 });


	return spriteEntity;
}

Entity player::createPlayerInstance()
{

	Entity spriteEntity = sprite::createSpriteToRegistry("assets/Player.png", "Player", 10.0f, 1, { 500, 630 });
	auto& Sprite = Registry.get<spriteStr>(spriteEntity);
	Sprite.localSpriteTransform.setScale({ 0.5f, 0.5f });
	sprite::updateSpriteBuffer();
	auto& Player = CreateComponent<playerStr>(spriteEntity);
	Player.playerNumber = 1;
	physics::Physics().createBody(spriteEntity, physics::capsule, physics::dynamicObj, { 500, 630 }, { {0,12.5},{0,-12.5},{12.5,12.5} }, physics::player);

	return spriteEntity;
}
