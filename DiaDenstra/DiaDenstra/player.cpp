#include "player.h"

#include "register.hpp"
#include "screen.h"
#include "sprite.h"
#include "spritetransform.h"
#include "physics.h"
#include "common.h"
#include "camera.h"
#include "input.hpp"
#include "networking/dance.h"
#include "ui.h"
#include "worldStats.hpp"

#include <fstream>
#include <string>
#include <queue>



void player::init(Dance& danceObj)
{
	//Packages
	//PlayerNumber | Pos.x | Pos.y
	danceObj.CreatePackage("OurPlayerPos", 0, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	//PlayerNumber | shot | bulletPos.x | bulletPos.y | bulletVel.x | bulletVel.y | BulletType
	danceObj.CreatePackage("OurPlayerShot", 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0);
	//PlayerNumber | OtherPlayer | PlayerRespawned | BulletType
	danceObj.CreatePackage("OurPlayerHit", 0, 0, 0, 0);
	//PlayerNumber | rotation | left/Right | GunType
	danceObj.CreatePackage("OurPlayerGunPos", 0, 0.0f, 0, 0);


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
	pearlCooldown -= dt;
	deathCooldown -= dt;
	int frameCheck = checkedFrame1;
	Entity ourEntity{};
	newBulletEntity = entt::null;

	for (const auto& [entity, Player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		//Find own player
		if (Player.playerNumber == ownPlayerNumber && Player.health > 0.0f)
		{
			ourEntity = entity;

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

			//camera
			camera::setCameraPos(transform.getTranslation());
			camera::zoomCamera(glm::vec2(1.5));

			//-------------------------------------------Gun Movement----------------------------------------------------

			auto& gunTransform = Registry.get<Stransform>(Player.gun.gunEntity);
			auto& gunSprite = Registry.get<spriteStr>(Player.gun.gunEntity);

			//Set rotation
			glm::vec2 mousepos = input::Input().getMousePosOffset();
			mousepos.y = float(SCRHEIGHT) - mousepos.y;
			glm::vec2 dir = normalize(mousepos - gunTransform.getTranslation());
			float dirX = mousepos.x - transform.getTranslation().x;
			float rotation = glm::degrees(atan2(dir.y, dir.x));

			if (updateGunRotation <= 0.0f) //Prevent too many atan2 calculations.
			{
				
				gunTransform.setRotation(rotation);
				updateGunRotation = 0.04f;
			}

			//Set position
			if (dirX < 0) //Left of player
			{
				danceObj.AddDataToParameter("OurPlayerGunPos", 2, 1);
				gunTransform.setTranslation({ transform.getTranslation().x - 20, transform.getTranslation().y });
				//Flip scale = Flipping sprite
				glm::vec2 scale = gunTransform.getScale();
				if (scale.y > 0)gunTransform.setScale({ scale.x , scale.y * -1 });
			}
			else //Right of player
			{
				danceObj.AddDataToParameter("OurPlayerGunPos", 2, 0);
				gunTransform.setTranslation({ transform.getTranslation().x + 20, transform.getTranslation().y });
				//Flip scale = Flipping sprite
				glm::vec2 scale = gunTransform.getScale();
				if (scale.y < 0)gunTransform.setScale({ scale.x , scale.y * -1 });

			}
			//Send package
			danceObj.AddDataToParameter("OurPlayerGunPos", 0, ownPlayerNumber);
			danceObj.AddDataToParameter("OurPlayerGunPos", 1, rotation);
			danceObj.AddDataToParameter("OurPlayerGunPos", 3, Player.gun.inventory[Player.gun.activeType]);

			danceObj.sendPackage("OurPlayerGunPos");

			//-------------------------------------------Gun Interaction----------------------------------------------------

			//Switching gun
			if (input::Input().getCurrentMouseScroll() != 0)
			{
				int index = Player.gun.activeType + int(input::Input().getCurrentMouseScroll());
				
				if (index < 0) index = int(Player.gun.inventory.size()) - (std::abs(index) % Player.gun.inventory.size());
				else index = index % Player.gun.inventory.size();

				Player.gun.activeType = Player.gun.inventory[index];
				initGunType(Player, static_cast<gunInfo::type>(Player.gun.inventory[Player.gun.activeType]));
			}

			//Reloading
			if (input::Input().getKeyDownOnce(input::Key::R) && Player.gun.bulletsLeft[Player.gun.activeType] < maxBullets[Player.gun.inventory[Player.gun.activeType]] && typeReloading == -1)
			{
				shootCooldown = Player.gun.maxReloadTime;
				typeReloading = Player.gun.activeType;
			}
			//Reloaded
			if (typeReloading != -1 && shootCooldown <= 0.0f)
			{
				Player.gun.bulletsLeft[typeReloading] = maxBullets[typeReloading];
				typeReloading = -1;
			}

			//Shooting
			if (input::Input().getMouseButton(input::MouseButton::Left) && shootCooldown <= 0.0f && Player.gun.bulletsLeft[Player.gun.activeType] > 0)
			{
				shootCooldown = Player.gun.maxCooldown; //Depends on gun

				int Type = Player.gun.inventory[Player.gun.activeType];

				//Create bullet
				if (Type != 0) Player.gun.bulletsLeft[Player.gun.activeType]--;

				float rotation = glm::degrees(atan2(dir.y, dir.x));
				Entity bulletEntity{};
				switch (Type)
				{
				case 0: //Hand
					bulletEntity = createHandAABB(gunTransform.getTranslation(), "OwnBullet", ownPlayerNumber);
					break;
				case 2: //Shotgun
					bulletEntity = createBullet(gunTransform.getTranslation(), rotation - 15, "OwnBullet", Type, ownPlayerNumber, 275.0f, 1, 0.16f);
					bulletEntity = createBullet(gunTransform.getTranslation(), rotation, "OwnBullet", Type, ownPlayerNumber, 275.0f, 1, 0.16f);
					bulletEntity = createBullet(gunTransform.getTranslation(), rotation + 15, "OwnBullet", Type, ownPlayerNumber, 275.0f, 1, 0.16f);
					break;
				case 4:
					bulletEntity = createBullet(gunTransform.getTranslation(), rotation, "OwnBullet", Type, ownPlayerNumber, 500.0f);
					break;
				case 5:
					bulletEntity = createBullet(gunTransform.getTranslation(), rotation, "OwnBullet", Type, ownPlayerNumber, 300, 4);
					break;
				default:
					bulletEntity = createBullet(gunTransform.getTranslation(), rotation, "OwnBullet", Type, ownPlayerNumber);
					break;
				}

				newBulletEntity = bulletEntity;
				newBulletType = Type;
				shot = true;
			}

			//-------------------------------------------Pearl Interaction----------------------------------------------------

			else if (input::Input().getMouseButton(input::MouseButton::Right) && pearlCooldown <= 0.0f)
			{
				//Can not pearl if shot at the same frame
				pearlCooldown = 5.0f; //Depends on gun
				float rotation = glm::degrees(atan2(dir.y, dir.x));
				Entity bulletEntity{};

				bulletEntity = createBullet(gunTransform.getTranslation(), rotation, "OwnBullet", 6, ownPlayerNumber, 200.0f);
				newBulletType = 6;
				newBulletEntity = bulletEntity;
				shot = true;
			}

			//------------------------------------------Show Player's Health--------------------------------------------------
			{
				ui::UI().image("PlayerHealthBackground", { 100.0f, SCRHEIGHT - 32.0f }, "assets/uiSprites/textBackgroundHalf.png", 10);
				ui::UI().setNextSize(2.0f, 2.0f);
				ui::UI().text("PlayerHealthText", std::to_string(int(Player.health)).c_str(), { 100.0f, SCRHEIGHT - 32.0f});
			}

			//------------------------------------------Show Player's Bullets Left--------------------------------------------------
			{
				int xOffset = 32;
				for (int i = 0; i < Player.gun.bulletsLeft[Player.gun.activeType]; i++)
				{
					char label[16];
					sprintf_s(label, sizeof(label), "BulletLeft%d", i);
					ui::UI().image(label, { SCRWIDTH - xOffset, SCRHEIGHT - 32.0f }, "assets/uiSprites/bulletsUI.png", 10);
					xOffset += 16;
				}
			}
			//------------------------------------------------Show Reload Time--------------------------------------------------
			{
				if (typeReloading != -1)
				{
					ui::UI().image("PlayerHealthBackground", { SCRWIDTH - 128, SCRHEIGHT - 64 }, "assets/uiSprites/textBackgroundHalf.png", 10);
					char text[16];
					sprintf_s(text, sizeof(text), "Reloading: %.1f", shootCooldown);
					ui::UI().setNextSize(2.0f, 2.0f);
					ui::UI().text("ReloadTimeLeft", text, { SCRWIDTH - 96, SCRHEIGHT - 64 });
				}
			}

		}
		else if (Player.playerNumber == ownPlayerNumber) //Player is dead
		{
			ui::UI().image("DeadImageBackground", { SCRWIDTH * 0.5f, SCRHEIGHT * 0.5f }, "assets/uiSprites/backgroundMenu.png", 15);

			ui::UI().setNextSize(3, 3);
			ui::UI().text("DeadText", "You Died", { SCRWIDTH * 0.5f, SCRHEIGHT * 0.7f });

			if (deathCooldown >= 0.0f)
			{
				char respawnTime[18];
				sprintf_s(respawnTime, sizeof(respawnTime), "Respawn in %d", int(deathCooldown));
				ui::UI().setNextSize(2, 2);
				ui::UI().text("RespawnText", respawnTime, { SCRWIDTH * 0.5f, SCRHEIGHT * 0.55f });
			}


			if (deathCooldown <= 0.0f && ui::UI().button("RespawnButton", { SCRWIDTH * 0.5f, SCRHEIGHT * 0.45f }, false, "assets/uiSprites/respawnButton.png"))
			{
				//Respawn
				Player.health = 100.0f;
				Player.healthStatus = 1;
				totalTouchingGroundAmount = 0;
				//TODO: invernability

				danceObj.AddDataToParameter("OurPlayerHit", 0, ownPlayerNumber);
				danceObj.AddDataToParameter("OurPlayerHit", 1, ownPlayerNumber);
				danceObj.AddDataToParameter("OurPlayerHit", 2, 1);
				danceObj.sendPackage("OurPlayerHit", true);
			}
		}
		else
		{
			//-------------------------------------------Gun Visuals for other players----------------------------------------------------
			auto& gunTransform = Registry.get<Stransform>(Player.gun.gunEntity);
			Dance::ReturnPackageInfo RPI;

			danceObj.getPackage("OurPlayerGunPos", true, &RPI);

			if (RPI.succeeded)
			{
				int PNumber = RPI.getVariable<int>(0);

				if (PNumber == Player.playerNumber)
				{
					float rotation = RPI.getVariable<float>(1);
					Player.gun.left = bool(RPI.getVariable<int>(2));
					Player.gun.activeType = RPI.getVariable<int>(3); //This will be the guntype, not index of inventory
					auto& gunSprite = Registry.get<spriteStr>(Player.gun.gunEntity);

					gunTransform.setRotation(rotation);

					initGunType(Player, static_cast<gunInfo::type>(Player.gun.activeType));
				}

			}

			//Set position
			if (Player.gun.left) //Left of player
			{
				gunTransform.setTranslation({ transform.getTranslation().x - 20, transform.getTranslation().y });
				//Flip scale = Flipping sprite
				glm::vec2 scale = gunTransform.getScale();
				if (scale.y > 0)gunTransform.setScale({ scale.x , scale.y * -1 });
			}
			else //Right of player
			{
				gunTransform.setTranslation({ transform.getTranslation().x + 20, transform.getTranslation().y });
				//Flip scale = Flipping sprite
				glm::vec2 scale = gunTransform.getScale();
				if (scale.y < 0)gunTransform.setScale({ scale.x , scale.y * -1 });

			}

			//-------------------------------------------Name Above Players----------------------------------------------------
			{
				std::string textLabel = "PlayerNameText";
				textLabel += std::to_string(Player.playerNumber);
				ui::UI().setNextSize(1.2f, 1.2f);
				glm::vec2 namePos = glm::vec2({ transform.getTranslation().x, transform.getTranslation().y + 25 });
				ui::UI().setNextWorldSpace();
				ui::UI().text(textLabel.c_str(), Player.playerName.c_str(), namePos, 4288421649);
			}
		}

		//-------------------------------------------Respawn or died----------------------------------------------------

		if (Player.healthStatus == 1)
		{
			physics::Physics().moveEntityIn(entity, spawnPositions[rand() % (spawnPositions.size() - 1)]);
		}
		else if (Player.healthStatus == 2)
		{
			physics::Physics().moveEntityOut(entity);
		}
		Player.healthStatus = 0;
	}


	//---------------------------------------------Other Controls-------------------------------------------------------

	if (input::Input().getKeyDown(input::Key::Tab))
	{
		worldStats::WorldStats().showAllData();
	}


	//-------------------------------------------Bullets Interaction----------------------------------------------------

	std::queue<Entity> deleteEntities;

	//Loop over each bullet
	for (const auto& [bulletEntity, transform2, Sprite, bullet] : Registry.view<Stransform, spriteStr, bulletStr>().each())
	{
		if (Sprite.name == "OwnBullet" || Sprite.name.compare(0, 11, "OtherBullet") == 0)
		{
			bullet.hitCooldown -= dt;
			bullet.lifetime -= dt;
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
					
					//Pearl interaction
					if (bulletNumber == ownPlayerNumber && bullet.type == 6)
					{
						auto* bulletTransform = Registry.try_get<Stransform>(bulletEntity);
						if (bulletTransform != nullptr)
						{
							physics::Physics().setBodyPos(bulletTransform->getTranslation(), ourEntity);
						}
					}

					playerStr* otherPlayer = nullptr;
					otherPlayer = Registry.try_get<playerStr>(otherEntity);
					if (otherPlayer != nullptr)
					{
						printf("Player %i hit Player %i\n", bulletNumber, otherPlayer->playerNumber); //Debug only


						if (bulletNumber == ownPlayerNumber)
						{
							//we hit someone
							danceObj.AddDataToParameter("OurPlayerHit", 0, ownPlayerNumber);
							danceObj.AddDataToParameter("OurPlayerHit", 1, otherPlayer->playerNumber);
							danceObj.AddDataToParameter("OurPlayerHit", 2, 0);
							danceObj.AddDataToParameter("OurPlayerHit", 3, bullet.type);
							danceObj.sendPackage("OurPlayerHit", true);

							worldStats::WorldStats().playerDidDamage(bulletNumber, otherPlayer->playerNumber, bulletDamage[bullet.type]);
							otherPlayer->health -= bulletDamage[bullet.type];
							if (otherPlayer->health <= 0.0f)
							{
								worldStats::WorldStats().playerKilled(ownPlayerNumber, otherPlayer->playerNumber, bullet.type);
								deathCooldown = 5.0f;
								otherPlayer->healthStatus = 2;
							}
						}

					}

					deleteEntities.push(bulletEntity);
				}
			}
			else if (hitObject) //We did not hit an entity, but we may hit the ground
			{
				if (bullet.hitCooldown <= 0.0f)
				{
					bullet.hitCooldown = 0.02f;
					bullet.health -= 1;

					if (checkFrame != checkedFrame2 && bullet.health <= 0)
					{
						//Pearl interaction
						if (bulletNumber == ownPlayerNumber && bullet.type == 6)
						{
							auto* bulletTransform = Registry.try_get<Stransform>(bulletEntity);
							if (bulletTransform != nullptr)
							{
								physics::Physics().setBodyPos(bulletTransform->getTranslation(), ourEntity);
							}
						}

						checkedFrame2 = frameCheck;
						deleteEntities.push(bulletEntity);
					}
				}
			}
			else if (bullet.lifetime <= 0.0f && checkFrame != checkedFrame2)
			{
				deleteEntities.push(bulletEntity);
			}
		}
	}
	//-------------------------------------------Hand Interaction----------------------------------------------------

	//Loop over each AABB sensor
	for (const auto& [bulletEntity, transform2, AABB] : Registry.view<Stransform, physics::AABBSensor>().each())
	{
		//Get player number from the bullet
		int bulletNumber = -1;
		if (AABB.name.compare(0, 11, "OtherBullet") == 0)
		{
			bulletNumber = std::stoi(&AABB.name[11]);
		}
		else
		{
			bulletNumber = ownPlayerNumber;
		}

		int checkFrame = checkedFrame2;
		//Entity otherEntity;
		bool hitObject = false;

		Entity* others = new Entity[16];
		int amount = 0;
		physics::Physics().getAllSensoredBodies(bulletEntity, handSensorID, checkFrame, others, amount);

		if (amount > 0)
		{
			if (checkFrame != checkedFrame2)
			{
				checkedFrame2 = frameCheck;
				//Hit occured
				for (int i = 0; i < amount; i++)
				{
					playerStr* otherPlayer = nullptr;
					otherPlayer = Registry.try_get<playerStr>(others[i]);
					if (otherPlayer != nullptr)
					{
						printf("Player %i hit Player %i\n", bulletNumber, otherPlayer->playerNumber); //Debug only


						if (bulletNumber == ownPlayerNumber)
						{
							//we hit someone
							danceObj.AddDataToParameter("OurPlayerHit", 0, ownPlayerNumber);
							danceObj.AddDataToParameter("OurPlayerHit", 1, otherPlayer->playerNumber);
							danceObj.AddDataToParameter("OurPlayerHit", 2, 0);
							danceObj.AddDataToParameter("OurPlayerHit", 3, 0);
							danceObj.sendPackage("OurPlayerHit", true);

							worldStats::WorldStats().playerDidDamage(bulletNumber, otherPlayer->playerNumber, bulletDamage[0]);
							otherPlayer->health -= bulletDamage[0];
							if (otherPlayer->health <= 0.0f)
							{
								worldStats::WorldStats().playerKilled(ownPlayerNumber, otherPlayer->playerNumber, 0);
								otherPlayer->healthStatus = 2;
							}
						}
					}
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
		Registry.destroy(bulletEntity);
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
			if (shot && Registry.valid(newBulletEntity))
			{
				danceObj.AddDataToParameter("OurPlayerShot", 1, 1);
				auto& bulletTransform = Registry.get<Stransform>(newBulletEntity);
				danceObj.AddDataToParameter("OurPlayerShot", 2, bulletTransform.getTranslation().x);
				danceObj.AddDataToParameter("OurPlayerShot", 3, bulletTransform.getTranslation().y);
				glm::vec2 dir = glm::normalize(physics::Physics().getVelocity(newBulletEntity));
				danceObj.AddDataToParameter("OurPlayerShot", 4, dir.x);
				danceObj.AddDataToParameter("OurPlayerShot", 5, dir.y);
				danceObj.AddDataToParameter("OurPlayerShot", 6, newBulletType);

				danceObj.sendPackage("OurPlayerShot", true);
			}
			shot = false;


		}
		else
		{
			physics::Physics().setBodyPos(player.targetPos, entity);

			if (player.shot)
			{
				player.shot = false;
				//Create bullet of other player
				std::string name = "OtherBullet" + std::to_string(player.playerNumber);
				float rotation = glm::degrees(atan2(player.bulletDir.y, player.bulletDir.x));

				switch (player.bulletType)
				{
				case 0:
					createHandAABB(player.bulletPos, name, player.playerNumber);
					break;
				case 2:
					createBullet(player.bulletPos, rotation, name, player.bulletType, player.playerNumber, 275.0f, 1, 0.16f);
					createBullet(player.bulletPos, rotation, name, player.bulletType, player.playerNumber, 275.0f, 1, 0.16f);
					createBullet(player.bulletPos, rotation, name, player.bulletType, player.playerNumber, 275.0f, 1, 0.16f);
					break;
				case 4:
					createBullet(player.bulletPos, rotation, name, player.bulletType, player.playerNumber, 400.0f);
					break;
				case 5:
					createBullet(player.bulletPos, rotation, name, player.bulletType, player.playerNumber, 250.0f, 4);
					break;
				case 6:
					createBullet(player.bulletPos, rotation, name, player.bulletType, player.playerNumber, 200.0f);
					break;
				default:
					createBullet(player.bulletPos, rotation, name, player.bulletType, player.playerNumber);
					break;
				}
			}
		}
	}
}

void player::endScreen()
{
	ui::UI().setNextSize(3, 3);
	ui::UI().text("EndScreenText", "GAME DONE", { SCRWIDTH * 0.5f, SCRHEIGHT * 0.8f });
	worldStats::WorldStats().showAllData();
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

				player.bulletType = Dance::dataToVariable<int>(data, 8);
				player.shot = true;
				player.bulletPos = pos;
				player.bulletDir = dir;

			}
		}
	}
}

void player::getPlayerHit(const std::string& data)
{
	int playerNumber = Dance::dataToVariable<int>(data, 2);

	int otherPlayer = Dance::dataToVariable<int>(data, 3);

	bool respawned = bool(Dance::dataToVariable<int>(data, 4));

	int bulletType = Dance::dataToVariable<int>(data, 5);


	for (const auto& [entity, player, transform] : Registry.view<playerStr, Stransform>().each())
	{
		if (respawned)
		{
			if (player.playerNumber == playerNumber)
			{
				player.healthStatus = 1;
				player.health = 100.0f;
			}
		}
		else if (player.playerNumber == otherPlayer)
		{
			worldStats::WorldStats().playerDidDamage(playerNumber, otherPlayer, bulletDamage[bulletType]);
			player.health -= bulletDamage[bulletType];
			if (player.health <= 0.0f)
			{
				worldStats::WorldStats().playerKilled(playerNumber, otherPlayer, bulletType);

				player.healthStatus = 2;

				if (otherPlayer == player.playerNumber) deathCooldown = 5.0f;
			}
		}
	}

}


Entity player::createOwnPlayer(glm::vec2 pos, int number)
{
	Entity spriteEntity = sprite::createSpriteToRegistry("assets/Player.png", "OwnPlayer", 10.0f, 1, pos);
	auto& transform = Registry.get<Stransform>(spriteEntity);
	transform.setScale({ 0.5f, 0.5f });
	sprite::updateSpriteBuffer(0);
	auto& Player = CreateComponent<playerStr>(spriteEntity);
	Player.playerNumber = number;
	ownPlayerNumber = number;
	physics::Physics().createBody(spriteEntity, physics::capsule, physics::dynamicObj, pos, { {0,12.5},{0,-12.5},{12.5,12.5} }, physics::ownplayer, physics::none, number + 1);


	//Create sensor for ground detection to know when to jump.
	physics::Physics().createSensor(spriteEntity, physics::ownplayer, physics::ground, glm::vec2(0, -22.5f), 5.0f, jumpSensorID);

	//Create pistol entity
	Entity gunEntity = sprite::createSpriteToRegistry("assets/glock.png", "Gun", 11.0f, 6, pos);
	Player.gun.gunEntity = gunEntity;
	initGunType(Player, gunInfo::PISTOL);
	Player.gun.inventory.push_back(1);

	//Debug
	Player.gun.inventory.push_back(2);
	Player.gun.inventory.push_back(3);
	Player.gun.inventory.push_back(4);
	Player.gun.inventory.push_back(5);
	for (int i = 1; i < 7; i++)
	{
		Player.gun.bulletsLeft.push_back(maxBullets[i]);
	}


	//Create Joints
	//physics::Physics().attachJoint(spriteEntity, gunEntity, 0.5f, { 25,0 }, { -5,0 });


	return spriteEntity;
}

Entity player::createPlayerInstance(glm::vec2 pos, int number)
{

	Entity spriteEntity = sprite::createSpriteToRegistry("assets/Player.png", "Player", 10.0f, 1, pos);
	auto& Sprite = Registry.get<spriteStr>(spriteEntity);
	Sprite.localSpriteTransform.setScale({ 0.5f, 0.5f });
	sprite::updateSpriteBuffer(0);
	auto& Player = CreateComponent<playerStr>(spriteEntity);
	Player.playerNumber = number;
	physics::Physics().createBody(spriteEntity, physics::capsule, physics::kinematic, pos, { {0,12.5},{0,-12.5},{12.5,12.5} }, physics::player, physics::none, number + 1);

	Entity gunEntity = sprite::createSpriteToRegistry("assets/glock.png", "Gun", 11.0f, 6, pos);
	Player.gun.gunEntity = gunEntity;

	return spriteEntity;
}

void player::writeName(const char* name)
{
	std::fstream file("assets/names.txt", std::ios::in | std::ios::out);

	if (file.is_open())
	{
		std::string oldName;
		file >> oldName;
		std::streampos pos = file.tellg();
		file.seekp(0);
		file << name;
		if (strlen(name) < oldName.length())
		{
			file << std::string(oldName.length() - strlen(name), ' ');
		}
		file.close();
	}
	else 
	{
		printf("File opening error\n");
	}
}

std::string player::getName(bool random)
{
	std::string output;

	std::ifstream file("assets/names.txt", std::ios::in);
	if (!file.is_open())
	{
		printf("File opening error\n");
		return nullptr;
	}

	if (!random) //Return name
	{
		file >> output;
	}
	else //Random name
	{
		std::string line;
		int lineN = 0;
		while (std::getline(file, line))
		{
			lineN++;
			if (lineN > 2)
			{
				int randNumber = rand() % 7;
				int wordNumber = 0;
				std::istringstream iss(line);
				std::string word;
				while (iss >> word)
				{
					if (wordNumber == randNumber)
					{
						output += word;
					}
					wordNumber++;
				}
			}
		}
	}

	
	return output;
}

void player::setName(int playerNumber, const char* name)
{
	for (const auto& [entity, player] : Registry.view<playerStr>().each())
	{
		if (player.playerNumber == playerNumber)
		{
			player.playerName = name;
		}
	}
}

void player::initGunType(playerStr& Player, gunInfo::type gunType)
{
	auto& Sprite = Registry.get<spriteStr>(Player.gun.gunEntity);
	auto& transform = Registry.get<Stransform>(Player.gun.gunEntity);

	sprite::setFrameSprite(&Sprite, static_cast<int>(gunType));
	Player.gun.activeType = static_cast<int>(gunType);
	switch (gunType)
	{
	case gunInfo::HAND:
		Player.gun.maxCooldown = 0.3f;
		Player.gun.maxReloadTime = 0.3f;
		transform.setRotationPivot({ 0.3f,0.5f });
		break;
	case gunInfo::PISTOL:
		Player.gun.maxCooldown = 0.2f;
		Player.gun.maxReloadTime = 1.5f;
		transform.setRotationPivot({ 0.47f,0.45f });
		break;
	case gunInfo::SHOTGUN:
		Player.gun.maxCooldown = 0.7f;
		Player.gun.maxReloadTime = 2.0f;
		transform.setRotationPivot({ 0.45f,0.45f });
		break;
	case gunInfo::MINIGUN:
		Player.gun.maxCooldown = 0.05f;
		Player.gun.maxReloadTime = 3.5f;
		transform.setRotationPivot({ 0.45f,0.6f });
		break;
	case gunInfo::SNIPER:
		Player.gun.maxCooldown = 1.0f;
		Player.gun.maxReloadTime = 2.5f;
		transform.setRotationPivot({ 0.4f,0.47f });
		break;
	case gunInfo::BOUNCER:
		Player.gun.maxCooldown = 0.4f;
		Player.gun.maxReloadTime = 2.0f;
		transform.setRotationPivot({ 0.35f,0.42f });
		break;
	default:
		break;
	}
	Player.gun.shootCooldown = Player.gun.maxCooldown;
}

Entity player::createHandAABB(glm::vec2 pos, std::string name, int playerNumber)
{
	Entity handEntity = sprite::createParticle("assets/handParticle.png", 4, 0, 0.05f, pos);
	physics::Physics().drawOrder.push_back(handEntity);
	uint64_t mask = physics::player | physics::ownplayer;
	physics::Physics().createAABBboxSensor(handEntity, pos, 16, 16, mask, playerNumber + 1, name, handSensorID);
	return handEntity;
}

Entity player::createBullet(glm::vec2 pos, float rotation, std::string name, int Type, int playerNumber, float speed, int health, float lifeTime)
{
	Entity bulletEntity = sprite::createSpriteToRegistry("assets/basicbullet.png", name.c_str(), 9.0f, 6, pos, {1,1}, {0,0}, rotation);
	auto& Sprite = Registry.get<spriteStr>(bulletEntity);
	sprite::setFrameSprite(&Sprite, Type - 1);
	physics::Physics().drawOrder.push_back(bulletEntity);

	physics::Physics().createBullet(bulletEntity, pos, rotation, speed, 7.5f, playerNumber + 1, 2);
	auto& bullet = CreateComponent<bulletStr>(bulletEntity);
	bullet.type = Type;
	bullet.lifetime = lifeTime;
	bullet.health = 1;
	if (Type == 5) bullet.health = 4;
	return bulletEntity;
}