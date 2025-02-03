#pragma once
#include "screen.h"
#pragma warning(push) 
#pragma warning(disable: 4267)
#include "CDT.h"
#pragma warning(pop)

#include <glm.hpp>
#include <vector>
#include <box2d.h>
#include "register.hpp"




class physics
{
public:

	enum shape
	{
		triangle, box, polygon, circle
	};
	enum type
	{
		staticObj, kinematic, dynamicObj
	};


	//Physics class object
	static physics& Physics()
	{
		static physics classObject;
		return classObject;
	}

	void init();

	void createBodyMap();

	//Polygon can not have more than 8 vertices
	void createBody(Entity entity, shape Shape, type Type, glm::vec2 pos, std::vector<glm::vec2> points);
	//To Simplify createBody(). width and height will be total length (width 5, height 5 = 5x5 box)
	void createBodyBox(Entity entity, type Type, glm::vec2 pos, float height, float width);

	void update();

	void sortSprites();

	//Debug
	void drawTriangles(screen& screenObj);
	void drawBodies(screen& screenObj);


	//Idk where else to place this
	std::vector<Entity> drawOrder;

	friend class tilemap;

private:

	physics() {};
	~physics() { b2DestroyWorld(worldId);}

	b2WorldDef worldDef{};
	b2WorldId worldId{};
	b2DebugDraw debugdraw{};
	float timeStep = 1.0f / 60.0f;
	int subStepCount = 4;
	CDT::Triangulation<double> Triangles;
	std::vector<std::vector<glm::vec2>> MapShapes;
	std::vector<std::vector<b2Vec2>> MapShapesB2;



};

