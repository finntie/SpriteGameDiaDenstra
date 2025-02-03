#include "sprite.h"
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

	tilemap::initTileMap("assets/tilemap/ground.png", "assets/tilemap/Map1.csv", "assets/tilemap/vertices.txt");

	physics::Physics().init();
	physics::Physics().createBodyMap();

	Entity bodyEntity = Registry.create();
	physics::Physics().createBodyBox(bodyEntity, physics::dynamicObj, { 340, 300 }, 100, 100);
	bodyEntity = Registry.create();

	physics::Physics().createBodyBox(bodyEntity, physics::dynamicObj, { 340, 300 }, 100, 100);

	Entity spriteEntity = Registry.create();
	auto& Sprite = CreateComponent<spriteStr>(spriteEntity);
	Sprite.depth = 0.5f;
	auto& transform = CreateComponent<Stransform>(spriteEntity);
	transform.setScale(glm::vec2(1, 1));
	transform.setTranslation(glm::vec2(100, 110));
	transform.setRotation(0);
	physics::Physics().createBodyBox(spriteEntity, physics::kinematic, { 460, 300 }, 100, 100);
	sprite::initFromFile(&Sprite, "assets/Image7.png", 2, "Smile");
	physics::Physics().drawOrder.push_back(spriteEntity);
	

	for (int i = 0; i < 25; i++)
	{
		spriteEntity = Registry.create();
		auto& Sprite2 = CreateComponent<spriteStr>(spriteEntity);
		Sprite2.depth = 10.0f;
		auto& transform2 = CreateComponent<Stransform>(spriteEntity);
		transform2.setScale(glm::vec2(1, 1));
		transform2.setTranslation(glm::vec2(100 + i * 25, 100));
		transform2.setRotation(0);
		physics::Physics().createBodyBox(spriteEntity, physics::dynamicObj, { 460, 300 }, 100, 100);
		sprite::initFromFile(&Sprite2, "assets/Image.png", 1, "NoTSmile");
		physics::Physics().drawOrder.push_back(spriteEntity);
	}

	physics::Physics().sortSprites();

}



void gamesystem::running(float dt)
{
	test += dt;
	//screenObj->Clear(sprite::colorVecToInt(glm::vec3(glm::sin(test), glm::cos(test), glm::sin(test))));
	screenObj->Clear(0);
	for (const auto& [spriteEntity, transform, Sprite] : Registry.view<Stransform, spriteStr>().each())
	{
		if (Sprite.name != "TileMapSprite")
		{
			transform.setVel({ 0, 0 });

			//transform.setRotation(test * 30);
			//transform.setScale(glm::vec2(glm::sin(test) + 4, glm::sin(test) + 4));

			if (input::Input().getKeyDown(input::Key::ArrowUp))
			{
				transform.setVel({ 0, 1 });
			}
			if (input::Input().getKeyDown(input::Key::ArrowDown))
			{
				transform.setVel({ 0, -1 });

			}
			if (input::Input().getKeyDown(input::Key::ArrowRight))
			{
				//sprite::setFrameSprite(&Sprite, 0);
				transform.setVel({ 1, 0 });

			}
			if (input::Input().getKeyDown(input::Key::ArrowLeft))
			{
				//sprite::setFrameSprite(&Sprite, 1);
				transform.setVel({-1, 0 });
			}
		}
		//printf("%f\n", glm::sin(test));
	}
	sprite::drawSprites(*screenObj);

	physics::Physics().drawTriangles(*screenObj);
	physics::Physics().drawBodies(*screenObj);
	physics::Physics().update();
}

void gamesystem::drawSprites(Shader* shader)
{
	for (auto& Entity : physics::Physics().drawOrder)
	{
		auto& Sprite = Registry.get<spriteStr>(Entity);
		auto& transform = Registry.get<Stransform>(Entity);

		if (Sprite.name != "TileMapSprite")
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
