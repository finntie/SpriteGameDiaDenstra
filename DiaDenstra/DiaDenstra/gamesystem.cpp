#include "sprite.h"
#include "player.h"
#include "dance.h"
#include "gamesystem.h" 


#include "opengl.h"
#include "screen.h"
#include "register.hpp"
#include "spritetransform.h"
#include "input.hpp"
#include "common.h"
#include "tilemap.h"
#include "physics.h"
#include "ui.h"
#include "camera.h"
#include "extra.hpp"

#include <random>   


entt::registry Registry;

void gamesystem::init()
{
	//Set randomizer seed
	unsigned seed = int(std::chrono::system_clock::now().time_since_epoch().count());
	srand(seed);

	playerObj = player();
	playerObj.init(*danceObj);

	tilemap::initTileMap("assets/tilemap/ground.png", "assets/tilemap/Map1.csv", "assets/tilemap/vertices.txt");

	playerObj.spawnPositions = tilemap::initSpawnLocations("assets/tilemap/PlayerSpawnMap.csv", "assets/tilemap/ground.png");

	physics::Physics().init();
	physics::Physics().createBodyMap();
	ui::UI().setScreen(screenObj);

	danceObj->init(true, true); //We won't need to use ipv6 for LAN.

	Entity spriteEntity = sprite::createSpriteToRegistry("assets/background.png", "BackGround", -1.0f, 1, { SCRWIDTH * 0.5f, SCRHEIGHT * 0.5f }, {1,1});
	physics::Physics().drawOrder.push_back(spriteEntity); 
	
	
	
	physics::Physics().sortSprites();

}



void gamesystem::running(float dt)
{
	screenObj->Clear(0);
	switch (gameStatus)
	{
	case 0: //Main Menu
	
		if (ui::UI().button("PlayButton", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.5f }, false, "assets/uiSprites/playButton.png"))
		{
			gameStatus = 2;
		}
		if (ui::UI().button("OptionsButton", { SCRWIDTH - 75,  75 }, false, "assets/uiSprites/optionButton.png"))
		{
			gameStatus = 1;
		}
		ui::UI().image("BackgroundImage", { 0,0 }, "assets/uiSprites/background.png", 0, 1, 1);
	
		break;
	case 1:

		ui::UI().image("BackgroundImage", { 0,0 }, "assets/uiSprites/background.png",0, 1, 1);

		if (ui::UI().button("BackButton", { 75,  75 }, false, "assets/uiSprites/backButton.png"))
		{
			gameStatus = 0;
		}

		break;
	case 2:

		ui::UI().image("BackgroundImage", { 0,0 }, "assets/uiSprites/background.png",0, 1, 1);
		if (ui::UI().button("BackButton", { 75,  75 }, false, "assets/uiSprites/backButton.png"))
		{
			gameStatus = 0;
		}
		if (ui::UI().button("JoinLobbyButton", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.6f }, false, "assets/uiSprites/joinLobbyButton.png"))
		{
			gameStatus = 3;
		}
		if (ui::UI().button("HostLobbyButton", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.4f }, false, "assets/uiSprites/hostLobbyButton.png"))
		{
			gameStatus = 4;
		}

		break;
	case 3: //Join menu
	{
		
		ui::UI().image("BackgroundImage", { 0,0 }, "assets/uiSprites/background.png", 0, 1, 1);

		ui::UI().setNextSize(2, 2);
		ui::UI().text("SameDeviceText", "Same Device", { SCRWIDTH * 0.45f, SCRHEIGHT * 0.65f }, 4278190080);
		ui::UI().setNextSize(2, 2);
		ui::UI().text("LanText", "LAN", { SCRWIDTH * 0.55, SCRHEIGHT * 0.65f }, 4278190080);

		ui::UI().radioButton("MultiplayerMode1", danceMode, 0, { SCRWIDTH * 0.45f, SCRHEIGHT * 0.6f });
		ui::UI().radioButton("MultiplayerMode2", danceMode, 1, { SCRWIDTH * 0.55f, SCRHEIGHT * 0.6f });

		static const char* output = "";
		if (danceMode != 0)
		{
			ui::UI().setNextSize(3.0f, 3.0f);
			ui::UI().text("LobbyJoinText", "Input Lobby Code", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.52f }, 4278190080);//Black

			ui::UI().setNextSize(1.5f, 1.0f);
			ui::UI().inputText("LobbyJoinInputText", &output, { SCRWIDTH * 0.5f, SCRHEIGHT * 0.47f });
		}


		if (ui::UI().button("JoinButton", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.33f }, false, "assets/uiSprites/joinButton.png"))
		{
			if (danceMode != -1 && (std::strlen(output) != 0 || danceMode == 0))
			{
				std::string outputString(output);
				danceObj->Connect(static_cast<Dance::DanceMoves>(danceMode), CodeToIP(outputString).c_str());
				gameStatus = 5;
			}
		}

		if (ui::UI().button("BackButton", { 75,  75 }, false, "assets/uiSprites/backButton.png"))
		{
			gameStatus = 2;
		}
	}
		break;
	case 4: //Host menu

	{
		ui::UI().image("BackgroundImage", { 0,0 }, "assets/uiSprites/background.png", 0, 1, 1);

		ui::UI().setNextSize(2, 2);
		ui::UI().text("SameDeviceText", "Same Device", { SCRWIDTH * 0.45f, SCRHEIGHT * 0.58f }, 4278190080);
		ui::UI().setNextSize(2, 2);
		ui::UI().text("LanText", "LAN", { SCRWIDTH * 0.55, SCRHEIGHT * 0.58f }, 4278190080);

		ui::UI().radioButton("MultiplayerMode1", danceMode, 0, { SCRWIDTH * 0.45f, SCRHEIGHT * 0.55f });
		ui::UI().radioButton("MultiplayerMode2", danceMode, 1, { SCRWIDTH * 0.55f, SCRHEIGHT * 0.55f });

		if (ui::UI().button("CreateButton", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.35f }, false, "assets/uiSprites/createButton.png"))
		{
			if (danceMode != -1)
			{
				danceObj->Host(static_cast<Dance::DanceMoves>(danceMode), 15);
				std::string test = danceObj->getIP(false);
				std::string code = IPToCode(test);
				gameStatus = 5;
			}
		}

		if (ui::UI().button("BackButton", { 75,  75 }, false, "assets/uiSprites/backButton.png"))
		{
			gameStatus = 2;
		}
	}
		break;
	case 5: //wait lobby

		ui::UI().image("BackgroundImage", { 0,0 }, "assets/uiSprites/background.png", 0, 1, 1);

		if (danceObj->getIfHost())
		{
			//Display player amount
			std::string totalCons = "Players: ";
			totalCons += std::to_string(danceObj->getTotalConnections());
			ui::UI().setNextSize(3, 3);
			ui::UI().text("PlayerAmountText", totalCons.c_str(), {SCRWIDTH * 0.5, SCRHEIGHT * 0.55f}, 4278190080);

			if (ui::UI().button("StartGameButton", { SCRWIDTH * 0.5, SCRHEIGHT * 0.45f }, false, "assets/uiSprites/startButton.png"))
			{
				danceObj->CreatePackage("startMessage", true);
				danceObj->sendPackage("startMessage");
				gameStatus = 6;
			}

		}
		else //Client
		{
			ui::UI().setNextSize(3, 3);
			ui::UI().text("WaitingText", "Waiting For Host To Start Game", { SCRWIDTH * 0.5, SCRHEIGHT * 0.5f }, 4278190080);

			Dance::ReturnPackageInfo RPI;
			danceObj->getPackage("startMessage", true, &RPI);
			if (RPI.succeeded) //We got something
			{
				if (RPI.getVariable<bool>(0)) //We want to start
				{
					gameStatus = 6;
				}
			}
		}

		break;
	case 6:

		if (!gamestateInit)
		{
			static int randomNumber = 0;
			if (danceObj->getIfHost())
			{
				randomNumber = rand();
				danceObj->CreatePackage("PlayerSpawnRandomNumber", randomNumber);
				danceObj->sendPackage("PlayerSpawnRandomNumber");
			}
			else if (randomNumber == 0)
			{
				//Get randommizer number from host
				Dance::ReturnPackageInfo RP;
				danceObj->getPackage("PlayerSpawnRandomNumber", true, &RP);
				if (RP.succeeded)
				{
					randomNumber = RP.getVariable<int>(0);
				}
				else
				{
					break; //repeat until received number
				}
			}

			//Random spawn posses
			std::vector<int> spawnPosses;
			for (int i = 0; i < int(playerObj.spawnPositions.size()); i++) spawnPosses.push_back(i);
			std::shuffle(playerObj.spawnPositions.begin(), playerObj.spawnPositions.end(), std::mt19937(randomNumber)); //Shuffle spawn positions


			//Give each player a number as host
			if (danceObj->getIfHost())
			{
				danceObj->CreatePackage("PlayerNumber", 0);
				for (int i = 0; i < danceObj->getTotalConnections(); i++)
				{
					danceObj->AddDataToParameter("PlayerNumber", 0, i + 1);
					danceObj->sendPackageTo("PlayerNumber", i + 1, false);
				}
				Entity playerEntity = playerObj.createOwnPlayer(playerObj.spawnPositions[0], 0);
				physics::Physics().drawOrder.push_back(playerEntity);
				physics::Physics().drawOrder.push_back(Registry.get<playerStr>(playerEntity).gunEntity);
			}
			else
			{
				//Create own player with correct number
				Dance::ReturnPackageInfo RP;
				danceObj->getPackage("PlayerNumber", true, &RP);
				if (RP.succeeded)
				{
					Entity playerEntity = playerObj.createOwnPlayer(playerObj.spawnPositions[RP.getVariable<int>(0)], RP.getVariable<int>(0));
					physics::Physics().drawOrder.push_back(playerEntity);
					physics::Physics().drawOrder.push_back(Registry.get<playerStr>(playerEntity).gunEntity);
				}
				else
				{
					break; //repeat until received number
				}
			}
		
			//For each player, create new player instance
			for (int i = 0; i < danceObj->getTotalConnections() + 1; i++)
			{
				if (!playerObj.isOwnPlayer(i))
				{
					Entity playerEntity = player::createPlayerInstance(playerObj.spawnPositions[i], i);
					physics::Physics().drawOrder.push_back(playerEntity);
				}
			}




			gamestateInit = true;
		}

		physics::Physics().update(dt);





		playerObj.control(*danceObj, dt);
		playerObj.updateOthers(*danceObj, dt);

		//Debug
		physics::Physics().drawPolygons(*screenObj);
		physics::Physics().drawBodies(*screenObj);

		break;
	default:
		break;
	}
	//Things that always need to be updated regardless of gamestate

	//Weird camera things

	const float radius = 100.0f;
	static float cameraTime = 0.0f;
	cameraTime += dt;
	float camX = sin(cameraTime) * radius;
	float camZ = cos(cameraTime) * radius;
	float zoomLevel = 1.0f + sin(cameraTime) * 0.5f;
	camera::setCameraPos(glm::vec2(camX, camZ));
	camera::zoomCamera(glm::vec2(zoomLevel, zoomLevel));


	sprite::updateSpriteBuffer();

}

void gamesystem::drawSprites(Shader* shader)
{
	for (auto& Entity : physics::Physics().drawOrder)
	{
		auto& Sprite = Registry.get<spriteStr>(Entity);
		auto& transform = Registry.get<Stransform>(Entity);

		//if (Sprite.name != "TileMapSprite")
		{

			if (Sprite.bufferChanged)
			{
				Sprite.texture->CopyFrom(Sprite.afterBuffer);
				Sprite.bufferChanged = false;
			}
			shader->Bind();
			shader->SetInputTexture(0, "c", Sprite.texture);
			DrawQuadSprite(shader, transform, Sprite.texture, &Sprite);
			shader->Unbind();
		}
	}

}
