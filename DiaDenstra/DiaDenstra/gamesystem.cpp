#include "sprite.h"
#include "player.h"
#include "gamesystem.h" 

#include "opengl.h"
#include "screen.h"
#include "register.hpp"
#include "spritetransform.h"
#include "input.hpp"
#include "common.h"
#include "tilemap.h"
#include "physics.h"

entt::registry Registry;

void gamesystem::init()
{
	playerObj = player();

	tilemap::initTileMap("assets/tilemap/ground.png", "assets/tilemap/Map1.csv", "assets/tilemap/vertices.txt");

	physics::Physics().init();
	physics::Physics().createBodyMap();


	Entity spriteEntity = sprite::createSpriteToRegistry("assets/background.png", "BackGround", -1.0f, 1, { SCRWIDTH * 0.5f, SCRHEIGHT * 0.5f }, {1,1});
	physics::Physics().drawOrder.push_back(spriteEntity); 

	Entity playerEntity = playerObj.createOwnPlayer();

	//Entity spriteEntity = sprite::createSpriteToRegistry("assets/Player.png", "Player", 5.0f, 1, { 480, 630 });
	//auto& Sprite = Registry.get<spriteStr>(spriteEntity);
	//Sprite.localSpriteTransform.setScale({ 0.5f, 0.5f });
	//sprite::updateSpriteBuffer();
	//auto& Player = CreateComponent<playerStr>(spriteEntity);
	//physics::Physics().createBody(spriteEntity, physics::capsule, physics::dynamicObj, { 480, 630 }, { {0,15},{0,-15},{15,15} });
	////physics::Physics().createBoxAroundSprite(spriteEntity, physics::dynamicObj);


	physics::Physics().drawOrder.push_back(playerEntity);
	physics::Physics().drawOrder.push_back(Registry.get<playerStr>(playerEntity).gunEntity);



	playerEntity = player::createPlayerInstance();

	physics::Physics().drawOrder.push_back(playerEntity);
	//physics::Physics().drawOrder.push_back(Registry.get<playerStr>(playerEntity).gunEntity);



	//for (int i = 0; i < 25; i++)
	//{
	//	Entity spriteEntity = sprite::createSpriteToRegistry("assets/Image.png", "OtherSmile", 1.0f, 1, { 460, 300 }, {1,1}, {30,30});
	//	physics::Physics().createBoxAroundSprite(spriteEntity, physics::dynamicObj);
	//	physics::Physics().drawOrder.push_back(spriteEntity);
	//}

	physics::Physics().sortSprites();

}



void gamesystem::running(float dt)
{
	physics::Physics().update(dt);
	test += dt;
	//screenObj->Clear(sprite::colorVecToInt(glm::vec3(glm::sin(test), glm::cos(test), glm::sin(test))));
	screenObj->Clear(0);
	for (const auto& [spriteEntity, transform, Sprite] : Registry.view<Stransform, spriteStr>().each())
	{
		if (Sprite.name != "TileMapSprite")
		{
			//transform.setRotation(test * 30);
			//transform.setScale(glm::vec2(glm::sin(test) + 4, glm::sin(test) + 4));
		}
		//printf("%f\n", glm::sin(test));
	}
	sprite::updateSpriteBuffer();
	playerObj.control(dt);

	physics::Physics().drawPolygons(*screenObj);
	physics::Physics().drawBodies(*screenObj);
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
