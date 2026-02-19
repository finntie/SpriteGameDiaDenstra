#include "sprite.h"
#include "player.h"
#include "networking/dance.h"
#include "gamesystem.h" 
#include "worldStats.hpp"

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


#include <chrono>
#include <random>   


entt::registry Registry;

void gamesystem::init()
{
	//Set randomizer seed
	unsigned seed = int(std::chrono::system_clock::now().time_since_epoch().count());
	srand(seed);

	//camera
	camera::setCameraPos(glm::vec2(HALFSCRWIDTH, HALFSCRHEIGHT));

	playerObj = player();
	playerObj.init(*danceObj);

	spriteMap = tilemap::initTileMap("assets/tilemap/ground.png", "assets/tilemap/bigMap1.csv", "assets/tilemap/vertices.txt");

	playerObj.spawnPositions = tilemap::initSpawnLocations("assets/tilemap/bigMap1_PlayerSpawnsBigMap.csv", "assets/tilemap/ground.png");

	Entity spriteEntity = sprite::createSpriteToRegistry("assets/tilemap/backGroundTile.png", "BackGroundTile", 1.1f, 1, {0,0});
	sprite::updateSpriteBuffer(0);
	physics::Physics().drawOrder.push_back(spriteEntity);

	physics::Physics().init();
	physics::Physics().createBodyMap();
	ui::UI().setScreen(screenObj);

	danceObj->init(true, true); //We won't need to use ipv6 for LAN.

	spriteEntity = sprite::createSpriteToRegistry("assets/uiSprites/background.png", "BackGround", -1.0f, 1, { 0,0 }, { 1,1 }, { 0, 0 }, 0.0f, 1.0f);
	physics::Physics().drawOrder.push_back(spriteEntity); 
	
	//Packages
	danceObj->CreatePackage("startMessage", true);
	danceObj->CreatePackage("PlayerSpawnRandomNumber", 0);
	danceObj->CreatePackage("PlayerNumber", 0);
	danceObj->CreatePackage("PlayerName", 0, "None");
	danceObj->CreatePackage("GameTime", 0, 0.0f); //Start/End/Continueing, Time Left
	danceObj->CreatePackageCallBackFunction("GameTime", [&](const std::string& data) {getTimerData(data); });

	physics::Physics().sortSprites();
}



void gamesystem::running(float dt)
{
	screenObj->Clear(0);

	danceObj->KeepAlive(dt);

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
		static const char* playerName;
		ui::UI().setNextSize(3, 3);
		ui::UI().text("InsertNameLabel", "Player Name: ", { SCRWIDTH * 0.2f, SCRHEIGHT * 0.8f }, 4278190080);
		ui::UI().setNextSize(2, 2);
		if (ui::UI().inputText("NameInput", &playerName, { SCRWIDTH * 0.4f, SCRHEIGHT * 0.8f }))
		{
			player::writeName(playerName);
		}


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
		ui::UI().text("SameDeviceText", "Same Device", { SCRWIDTH * 0.40f, SCRHEIGHT * 0.65f }, 4278190080);
		ui::UI().setNextSize(2, 2);
		ui::UI().text("LanText", "LAN", { SCRWIDTH * 0.50, SCRHEIGHT * 0.65f }, 4278190080);
		ui::UI().setNextSize(2, 2);
		ui::UI().text("PublicText", "Public", { SCRWIDTH * 0.60, SCRHEIGHT * 0.65f }, 4278190080);

		ui::UI().radioButton("MultiplayerMode1", danceMode, 0, { SCRWIDTH * 0.40f, SCRHEIGHT * 0.6f });
		ui::UI().radioButton("MultiplayerMode2", danceMode, 1, { SCRWIDTH * 0.50f, SCRHEIGHT * 0.6f });
		ui::UI().radioButton("MultiplayerMode3", danceMode, 2, { SCRWIDTH * 0.60f, SCRHEIGHT * 0.6f });

		static const char* output = "";
		if (danceMode != 0)
		{
			ui::UI().setNextSize(3.0f, 3.0f);
			ui::UI().text("LobbyJoinText", "Input Lobby Code", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.52f }, 4278190080);//Black

			ui::UI().setNextSize(0.3f, 0.3f);
			if (ui::UI().button("PasteClipBoardButton", { SCRWIDTH * 0.35f,SCRHEIGHT * 0.42f }, false, "assets/uiSprites/pasteClipButton.png"))
			{
				std::string ipString = getFromClipBoard();
				ui::UI().inputText("LobbyJoinInputText", &output, { SCRWIDTH * 0.65f, SCRHEIGHT * 0.47f }, ipString.c_str(), false);
			}

			ui::UI().setNextSize(1.5f, 1.0f);
			ui::UI().inputText("LobbyJoinInputText", &output, { SCRWIDTH * 0.5f, SCRHEIGHT * 0.47f });
		}
		if (danceMode == 2)
		{
			if (ownPublicCode.length() < 4)
			{
				std::string IP = danceObj->getIP(true);
				ownPublicCode = IPToCode(IP);
				setToClipBoard(ownPublicCode);
			}

			//ForceIPV4
			ui::UI().setNextSize(2, 2);
			ui::UI().text("IPV4Text", "Use IPV4", { SCRWIDTH * 0.43f, SCRHEIGHT * 0.75f }, 4278190080);
			ui::UI().setNextSize(2, 2);
			ui::UI().text("IPV6Text", "Use IPV6", { SCRWIDTH * 0.57f, SCRHEIGHT * 0.75f }, 4278190080);
			static int forceIPV4RB = 0;
			if (ui::UI().radioButton("ForceIPV4But", forceIPV4RB, 0, { SCRWIDTH * 0.45f, SCRHEIGHT * 0.70f }))
			{
				forceIPV4 = true;
				danceObj->setForceIPV4(forceIPV4);
				std::string IP = danceObj->getIP(true);
				ownPublicCode = IPToCode(IP);
				setToClipBoard(ownPublicCode);
			}
			if (ui::UI().radioButton("ForceIPV6But", forceIPV4RB, 1, { SCRWIDTH * 0.55f, SCRHEIGHT * 0.70f }))
			{
				forceIPV4 = false;
				danceObj->setForceIPV4(forceIPV4);
				std::string IP = danceObj->getIP(true);
				ownPublicCode = IPToCode(IP);
				setToClipBoard(ownPublicCode);
			}

			//Display own code
			std::string gameCodeString = "Our Code: '";
			gameCodeString += ownPublicCode + "'";
			ui::UI().setNextSize(3, 3);
			ui::UI().text("ClientCodeNumberText", gameCodeString.c_str(), { SCRWIDTH * 0.5, SCRHEIGHT * 0.80f }, 4278190080);
		}


		if (ui::UI().button("JoinButton", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.33f }, false, "assets/uiSprites/joinButton.png"))
		{
			if (danceMode != -1 && (std::strlen(output) != 0 || danceMode == 0))
			{
				std::string outputString(output);
				if (danceMode >= 1)
				{
					outputString = CodeToIP(outputString);
				}

				if (danceObj->Connect(static_cast<Dance::DanceMoves>(danceMode), outputString.c_str(), 0, forceIPV4))
				{
					gameStatus = 5;
				}
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
		ui::UI().text("SameDeviceText", "Same Device", { SCRWIDTH * 0.40f, SCRHEIGHT * 0.58f }, 4278190080);
		ui::UI().setNextSize(2, 2);
		ui::UI().text("LanText", "LAN", { SCRWIDTH * 0.50, SCRHEIGHT * 0.58f }, 4278190080);
		ui::UI().setNextSize(2, 2);
		ui::UI().text("PublicText", "Public", { SCRWIDTH * 0.60, SCRHEIGHT * 0.58f }, 4278190080);

		ui::UI().radioButton("MultiplayerMode1", danceMode, 0, { SCRWIDTH * 0.40f, SCRHEIGHT * 0.55f });
		ui::UI().radioButton("MultiplayerMode2", danceMode, 1, { SCRWIDTH * 0.50f, SCRHEIGHT * 0.55f });
		ui::UI().radioButton("MultiplayerMode3", danceMode, 2, { SCRWIDTH * 0.60f, SCRHEIGHT * 0.55f });
		
		//We have to add all client IPs if public
		static const char* output = "";
		static std::vector<std::string> clientIPs;
		if (danceMode == 2)
		{
			if (ownPublicCode.length() < 4)
			{
				danceObj->setForceIPV4(forceIPV4);
				std::string IP = danceObj->getIP(true);
				ownPublicCode = IPToCode(IP);

				setToClipBoard(ownPublicCode);
			}

			//Display own code
			std::string gameCodeString = "Host Code: '";
			gameCodeString += ownPublicCode + "'";
			ui::UI().setNextSize(3, 3);
			ui::UI().text("HostCodeNumberText", gameCodeString.c_str(), { SCRWIDTH * 0.5, SCRHEIGHT * 0.75f }, 4278190080);

			//ForceIPV4
			ui::UI().setNextSize(2, 2);
			ui::UI().text("IPV4Text", "Use IPV4", { SCRWIDTH * 0.43f, SCRHEIGHT * 0.68f }, 4278190080);
			ui::UI().setNextSize(2, 2);
			ui::UI().text("IPV6Text", "Use IPV6", { SCRWIDTH * 0.57f, SCRHEIGHT * 0.68f }, 4278190080);
			static int forceIPV4RB = 0;
			if (ui::UI().radioButton("ForceIPV4But", forceIPV4RB, 0, { SCRWIDTH * 0.45f, SCRHEIGHT * 0.65f }))
			{
				forceIPV4 = true;
				danceObj->setForceIPV4(forceIPV4);
				std::string IP = danceObj->getIP(true);
				ownPublicCode = IPToCode(IP);
				setToClipBoard(ownPublicCode);
			}
			if (ui::UI().radioButton("ForceIPV6But", forceIPV4RB, 1, { SCRWIDTH * 0.55f, SCRHEIGHT * 0.65f }))
			{
				forceIPV4 = false;
				danceObj->setForceIPV4(forceIPV4);
				std::string IP = danceObj->getIP(true);
				ownPublicCode = IPToCode(IP);
				setToClipBoard(ownPublicCode);
			}

			ui::UI().setNextSize(2.0f, 2.0f);
			ui::UI().text("ClientIPsText", "Fill in code of clients 1 by 1", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.47f }, 4278190080);//Black

			ui::UI().setNextSize(0.3f, 0.3f);
			if (ui::UI().button("PasteClipBoardButton", { SCRWIDTH * 0.35f,SCRHEIGHT * 0.42f }, false, "assets/uiSprites/pasteClipButton.png"))
			{
				std::string ipString = getFromClipBoard();
				ui::UI().inputText("LobbyJoinInputText", &output, { SCRWIDTH * 0.65f, SCRHEIGHT * 0.42f }, ipString.c_str(), false);
			}

			ui::UI().setNextSize(1.5f, 1.0f);
			ui::UI().inputText("LobbyJoinInputText", &output, { SCRWIDTH * 0.5f, SCRHEIGHT * 0.42f });

			ui::UI().setNextSize(0.3f, 0.3f);
			if (ui::UI().button("AddIPbutton", { SCRWIDTH * 0.65f,SCRHEIGHT * 0.42f }, false, "assets/uiSprites/addClientButton.png"))
			{
				std::string ipString = output;
				clientIPs.push_back(CodeToIP(ipString));
				ui::UI().inputText("LobbyJoinInputText", &output, { SCRWIDTH * 0.65f, SCRHEIGHT * 0.42f }, 0, true);
			}
		}

		if (ui::UI().button("CreateButton", { SCRWIDTH * 0.5f,SCRHEIGHT * 0.30f }, false, "assets/uiSprites/createButton.png"))
		{
			if (danceMode != -1)
			{
				danceObj->Host(static_cast<Dance::DanceMoves>(danceMode), 15, 0, forceIPV4, clientIPs);
				std::string IP = danceObj->getIP(danceMode == 2);
				gameCode = IPToCode(IP);
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
			if (danceMode == 1)
			{
				//Display lobby code
				std::string gameCodeString = "Lobby Code: '";
				gameCodeString += gameCode + "'";
				ui::UI().setNextSize(3, 3);
				ui::UI().text("GameCodeText", gameCodeString.c_str(), { SCRWIDTH * 0.5, SCRHEIGHT * 0.65f }, 4278190080);
			}
			if (danceMode == 2)
			{
				ui::UI().text("HostHolePunchWaitText", "Waiting for everyone to connect", { SCRWIDTH * 0.5, SCRHEIGHT * 0.60f }, 4278190080);
			}


			//Display player amount
			std::string totalCons = "Players: ";
			totalCons += std::to_string(danceObj->getTotalConnections());
			ui::UI().setNextSize(3, 3);
			ui::UI().text("PlayerAmountText", totalCons.c_str(), {SCRWIDTH * 0.5, SCRHEIGHT * 0.55f}, 4278190080);

			if ((danceMode == 2 && danceObj->holePunchSucceeded() == 2) || danceMode != 2) //Only able to proceed if holepunching worked or we are not using public
			{
				if (ui::UI().button("StartGameButton", { SCRWIDTH * 0.5, SCRHEIGHT * 0.45f }, false, "assets/uiSprites/startButton.png"))
				{
					danceObj->AddDataToParameter("startMessage", 0, true);
					danceObj->sendPackage("startMessage", true);
					gameStatus = 6;
				}
			}
		}
		else //Client
		{
			ui::UI().setNextSize(3, 3);
			ui::UI().text("WaitingText", "Waiting For Host To Start Game", { SCRWIDTH * 0.5, SCRHEIGHT * 0.5f }, 4278190080);

			//Check if host did not disconnect
			if (danceObj->isDisconnected(0, true))
			{
				gameStatus = 2;
			}

			if (danceMode == 2)
			{
				//Again display own code	
				std::string gameCodeString = "Our Code: '";
				gameCodeString += ownPublicCode + "'";
				ui::UI().setNextSize(3, 3);
				ui::UI().text("ClientCodeNumberText", gameCodeString.c_str(), { SCRWIDTH * 0.5, SCRHEIGHT * 0.75f }, 4278190080);
				

				ui::UI().setNextSize(3, 3);
				if (danceObj->holePunchSucceeded() == 1)
				{
					ui::UI().text("ClientHolePunchWaitText", "Currently trying to connect to the host...", { SCRWIDTH * 0.5, SCRHEIGHT * 0.60f }, 4278190080);
				}
				else if (danceObj->holePunchSucceeded() == 2)
				{
					ui::UI().text("ClientHolePunchSuccesText", "Connected to the host", { SCRWIDTH * 0.5, SCRHEIGHT * 0.60f }, 4278190080);
				}
			}

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
			if (gamestateInitTrack == 0)
			{
				static int randomNumber = 0;
				if (danceObj->getIfHost())
				{
					randomNumber = rand();
					danceObj->AddDataToParameter("PlayerSpawnRandomNumber", 0, randomNumber);
					danceObj->sendPackage("PlayerSpawnRandomNumber", true);
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

				gamestateInitTrack++;
			}
			else if (gamestateInitTrack == 1)
			{
				
				//Give each player a number as host
				if (danceObj->getIfHost())
				{
					for (int i = 0; i < danceObj->getTotalConnections(); i++)
					{
						danceObj->AddDataToParameter("PlayerNumber", 0, i + 1);
						danceObj->sendPackageTo("PlayerNumber", i + 1, false, true);
					}
					Entity playerEntity = playerObj.createOwnPlayer(playerObj.spawnPositions[0], 0);
					physics::Physics().drawOrder.push_back(playerEntity);
					physics::Physics().drawOrder.push_back(Registry.get<playerStr>(playerEntity).gun.gunEntity);
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
						physics::Physics().drawOrder.push_back(Registry.get<playerStr>(playerEntity).gun.gunEntity);
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
						physics::Physics().drawOrder.push_back(Registry.get<playerStr>(playerEntity).gun.gunEntity);
					}
				}

				danceObj->AddDataToParameter("PlayerName", 0, playerObj.getOwnPlayerNumber());
				std::string name = player::getName(false);
				if (name == "None" || name.empty()) name = player::getName(true);
				playerObj.setName(playerObj.getOwnPlayerNumber(), name.c_str());
				danceObj->AddDataToParameter("PlayerName", 1, name);

				danceObj->sendPackage("PlayerName", true);
				printf("Send my own name to all other connections\n");

				gamestateInitTrack++;
			}
			else if (gamestateInitTrack == 2)
			{
				//Get names
				static std::vector<int> namesGot;

				Dance::ReturnPackageInfo RPI;
				danceObj->getPackage("PlayerName", true, &RPI);
				if (RPI.succeeded)
				{
					int playerNumber = RPI.getVariable<int>(0);
					for (int i = 0; i < int(namesGot.size()); i++)
					{
						if (playerNumber == namesGot[i]) break;
					}
					namesGot.push_back(playerNumber);
					playerObj.setName(playerNumber, RPI.getVariable<std::string>(1).c_str());
				}
				if (namesGot.size() < danceObj->getTotalConnections())
				{
					break;
				}

				//init worldStats
				{
					std::vector<std::string> names;
					for (const auto& [entity, Player] : Registry.view<playerStr>().each())
					{
						names.push_back(Player.playerName);
					}
					worldStats::WorldStats().init(danceObj->getTotalConnections() + 1, names);
				}

				gamestateInitTrack++;
			}
			if (gamestateInitTrack == 4)
			{
				gamestateInit = true;
			}
		}

		gameTimeLeft -= dt;
		//Send update every 60 seconds
		if (danceObj->getIfHost())
		{
			if (int(gameTimeLeft) % 60 == 0)
			{
				danceObj->AddDataToParameter("GameTime", 0, 2);
				danceObj->AddDataToParameter("GameTime", 1, gameTimeLeft);
				danceObj->sendPackage("GameTime", true);
			}
			if (gameTimeLeft <= 0.0f)
			{
				danceObj->AddDataToParameter("GameTime", 0, 1);
				danceObj->AddDataToParameter("GameTime", 1, 0.0f);
				danceObj->sendPackage("GameTime", true);
			}
		}

		if (gameTimeLeft > 0.0f)
		{
			physics::Physics().update(dt);

			playerObj.control(*danceObj, dt);
			playerObj.updateOthers(*danceObj, dt);

			//---------------------------------------------Show Time Left--------------------------------------------------
			{
				ui::UI().image("GameTimerImage", { SCRWIDTH * 0.5f, SCRHEIGHT - 32.0f }, "assets/uiSprites/textBackgroundHalf.png", 12);
				ui::UI().setNextSize(2.0f, 2.0f);
				ui::UI().text("GameTimer", std::to_string(int(gameTimeLeft)).c_str(), { SCRWIDTH * 0.5f, SCRHEIGHT - 32.0f });
			}
		}
		else
		{
			playerObj.endScreen();
		}

		worldStats::WorldStats().showUI(dt);

		//Debug
		//physics::Physics().drawPolygons(*screenObj);
		//physics::Physics().drawBodies(*screenObj);

		break;
	default:
		break;
	}
	//Things that always need to be updated regardless of gamestate

	//Weird camera things

	//const float radius = 100.0f;
	//static float cameraTime = 0.0f;
	//cameraTime += dt;
	//float camX = sin(cameraTime) * radius;
	//float camZ = cos(cameraTime) * radius;
	//float zoomLevel = 1.0f + sin(cameraTime) * 0.5f;
	//camera::setCameraPos(glm::vec2(camX, camZ));
	//camera::zoomCamera(glm::vec2(zoomLevel, zoomLevel));

	//printf("pos x %f, y %f\n", input::Input().getMousePosOffset().x, input::Input().getMousePosOffset().y);

	sprite::updateSpriteBuffer(dt);

}

void gamesystem::drawSprites(Shader* shader)
{
	for (int i = 0; i < int(physics::Physics().drawOrder.size()); i++)
	{
		auto& entity = physics::Physics().drawOrder[i];

		if (!Registry.valid(entity))
		{
			physics::Physics().drawOrder.erase(physics::Physics().drawOrder.begin() + i);  //Delete ourself
			i--;
			continue;
		}
		auto* Sprite = Registry.try_get<spriteStr>(entity);
		if (!Sprite)
		{
			//Sprite is probably a particle
			auto* ParticleSprite = Registry.try_get<spriteParticle>(entity);
			if (!ParticleSprite)
			{
				printf("Error, could not find entity %i to draw \n", static_cast<int>(entity));
				continue;
			}
			Sprite = &ParticleSprite->Sprite;
		}
		

		// Draw background tiles outside of the map.
		if (Sprite->name == "BackGroundTile")
		{
			drawBackgroundDirt(shader, entity);
			continue;
		}

		// Copy needed if not wanting to change current transform when moving with camera
		auto useTransform = Registry.get<Stransform>(entity);

		if (Sprite->moveWithCamera != 0.0f)
		{
			//Adjust transform based on current position and camera position
			b2Vec2 pos1{ 0.0f, 0.0f };
			glm::vec2 camPos = camera::getCameraPos();
			b2Vec2 pos2{ camPos.x, camPos.y };
			b2Vec2 posToCameraRaw = b2Lerp(pos1, pos2, Sprite->moveWithCamera);
			glm::vec2 posToCamera = { posToCameraRaw.x + useTransform.getTranslation().x, posToCameraRaw.y + useTransform.getTranslation().y };
			useTransform.setTranslation(posToCamera);
		}

		if (Sprite->bufferChanged)
		{
			Sprite->texture->CopyFrom(Sprite->afterBuffer);
			Sprite->bufferChanged = false;
		}
		shader->Bind();
		shader->SetInputTexture(0, "c", Sprite->texture);
		DrawQuadSprite(shader, useTransform, Sprite->texture, Sprite);
		shader->Unbind();

	}

}

void gamesystem::drawBackgroundDirt(Shader* shader, Entity& spriteEntity)
{
	// Draw all the tiles outside of the playing area but in view of the player
	auto* SpriteTile = Registry.try_get<spriteStr>(spriteEntity);
	auto* SpriteMap = Registry.try_get<spriteStr>(spriteMap);
	if (!SpriteMap || !SpriteTile) return;


	if (SpriteTile->bufferChanged)
	{
		SpriteTile->texture->CopyFrom(SpriteTile->afterBuffer);
		SpriteTile->bufferChanged = false;
	}

	glm::ivec2 sizeTile = glm::ivec2(SpriteTile->getAfterWidth(), SpriteTile->getAfterHeight());
	glm::ivec2 sizeMap = glm::ivec2(SpriteMap->getAfterWidth(), -SpriteMap->getAfterHeight());
	glm::ivec2 viewMin = glm::ivec2(camera::screenToView({ 0,0 }));
	glm::ivec2 viewMax = glm::ivec2(camera::screenToView({ SCRWIDTH, SCRHEIGHT }));
	// We need to offset viewMin and max due to having reversed the y inside screenToView
	viewMin.y = SCRHEIGHT - viewMin.y;
	viewMax.y = -(viewMax.y - SCRHEIGHT);
	// flooring the extra value to the nearest tile size
	glm::ivec2 exess = viewMin - (viewMin % sizeTile);
	auto tileTransform = Registry.get<Stransform>(spriteEntity);

	shader->Bind();
	shader->SetInputTexture(0, "c", SpriteTile->texture);

	// Loop over whole screen size, using exess we offset correcty for tiles at 0,0
	for (int y = exess.y + sizeTile.y; y > viewMax.y - sizeTile.y; y -= sizeTile.y)
	{
		for (int x = exess.x - sizeTile.x; x < viewMax.x + sizeTile.x; x += sizeTile.x)
		{
			// Only place tile outside of the map
			if (x <= -sizeTile.x || x >= sizeMap.x ||
				y >= 0 || y <= sizeMap.y - sizeTile.y)
			{
				tileTransform.setTranslation(glm::vec2(x + sizeTile.x / 2, y + sizeTile.y / 2));
				DrawQuadSprite(shader, tileTransform, SpriteTile->texture, SpriteTile);
			}
		}

	}
	shader->Unbind();
}

void gamesystem::getTimerData(const std::string& data)
{
	int timeData = Dance::dataToVariable<int>(data, 2);

	float timeLeft = Dance::dataToVariable<float>(data, 3);

	if (timeData == 2)
	{
		gameTimeLeft = timeLeft;
	}
	else if (timeData == 1)
	{
		gameTimeLeft = 0.0f;
	}
}

void gamesystem::setToClipBoard(const std::string& data)
{
	//Copy Code to clipboard:
	if (OpenClipboard(GW_HWNDFIRST))
	{
		//Using windows global memory.
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.length() + 1);
		if (hMem)
		{
			auto hMemLock = GlobalLock(hMem);
			if (hMemLock)
			{
				memcpy(hMemLock, data.c_str(), data.length());
				((char*)hMemLock)[data.length()] = '\0';
				GlobalUnlock(hMem);

				EmptyClipboard();
				SetClipboardData(CF_TEXT, hMem);
			}
			else
			{
				GlobalFree(hMem);
			}
		}
		CloseClipboard();
	}
}

std::string gamesystem::getFromClipBoard()
{
	//Paste clipboard data
	std::string output;
	if (OpenClipboard(GW_HWNDFIRST))
	{
		//Using windows global memory.
		HGLOBAL hMem = GetClipboardData(CF_TEXT);
		if (hMem)
		{
			char* outputClip = (char*)GlobalLock(hMem);
			if (outputClip)
			{
				output = outputClip;
				GlobalUnlock(hMem);
			}
		}
		CloseClipboard();
	}
	return output;
}
