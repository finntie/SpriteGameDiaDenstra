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

#define CENTIMETER 10
#define INVERSECENTIMETER 0.1f


struct int2
{
	int x = 0;
	int y = 0;
};

class physics
{
public:

	enum shape
	{
		circle, capsule, segment, polygon, chainSegment
	};
	enum type
	{
		staticObj, kinematic, dynamicObj
	};
	//Nothing can be used as mask to collide with nothing
	enum categoryType
	{
		none = 1 << 0, bullet = 1 << 1, ground = 1 << 2, player = 1 << 3, nothing = 1 << 4, ownplayer = 1 << 5
	};
	struct AABBSensor
	{
		std::string name{};
		bool alive = true;
	};

	//Physics class object
	static physics& Physics()
	{
		static physics classObject;
		return classObject;
	}

	void init();

	void createBodyMap();

	/// <summary>Create a physics body for a shape, if body already exists, it adds a shape</summary>
	/// <param name="entity">ENTT entity to connect it to</param>
	/// <param name="Shape">Shape, check enum types</param>
	/// <param name="Type">body type check enum</param>
	/// <param name="pos">position of the body</param>
	/// <param name="points">Points that make up the shape, for capsule, points[0] is center1, points[1] is center2 and points[2].x is radius of capsule || for circle, its the same as capsule but without center 2.</param>
	/// <param name="category">Give this body a category</param>
	/// <param name="mask">Only collide with this category</param>
	/// <param name="groupIdx">If two collided objects have the same idx, they don't collide: https://phaser.io/tutorials/box2d-tutorials/collision-filtering</param>
	void createBody(const Entity& entity, const shape Shape, const type Type, glm::vec2 pos,  std::vector<glm::vec2> points, categoryType category = none, categoryType mask = none, const int groupIdx = 0);
	//To Simplify createBody(). width and height will be total length (width 5, height 5 = 5x5 box)
	void createBodyBox(Entity entity, type Type, glm::vec2 pos, float height, float width);
	/// <summary>Automaticly creates a box around the sprite.</summary>
	/// <param name="entity">Registry entity</param>
	/// <param name="Type">Type</param>
	void createBoxAroundSprite(Entity entity, type Type);

	void createSensor(const Entity entity, const categoryType Type, const categoryType targetMask, glm::vec2 pos, float size, int& customID);

	void createAABBboxSensor(const Entity entity, glm::vec2 pos, float width, float height, const uint64_t mask, int groupIndex, std::string name, int& customID);

	//For now only distance joint
	void attachJoint(const Entity entity1, const Entity entity2, float distance, glm::vec2 anchorPos1 = {0,0}, glm::vec2 anchorPos2 = {0,0});

	/// <summary>Check if the specified sensor in this entity touched the masked body</summary>
	/// <param name="entity">Entity which includes the sensor</param>
	/// <param name="customID">Custom ID set by sensor creation</param>
	/// <param name="Shape">Shape of sensor to check, default is polygon</param>
	/// <param name="frameChecked">Returns the physics frame that has currently be checked, could be used to check for duplicate checks</param>
	/// <returns>0 if nothing happened, 1 if touched body, 2 if left body</returns>
	int2 sensorTouchedBody(Entity entity, int customID, int& frameChecked, shape Shape = polygon);

	void getAllSensoredBodies(Entity entity, int customID, int& frameChecked, Entity* outputArray, int& outputSize, shape Shape = polygon);

	/// <summary>Check if sensor specified exist and which one it is<summary>
	/// <param name="Body">Body that includes the sensor</param>
	/// <param name="customID">Custom ID set by sensor creation</param>
	/// <param name="Shape">Shape of sensor to check, default is polygon</param>
	/// <param name="ShapeID">This will be filled with the correct b2ShapeId</param>
	/// <returns>If it was the sensor</returns>
	bool isSensor(const b2BodyId& Body, const int customID,  const shape Shape, b2ShapeId& ShapeID);

	bool getHitEntity(Entity entity1, Entity& entity2, int& frameChecked, bool& hitObject, uint64_t filter); //TODO: could add extra parameters to smaller the search

	void setRotOfEntity(Entity entity, glm::vec2 rotation);

	void createBullet(Entity entity, glm::vec2 startPos, float direction, float speed, float size, int playerNumber, int bulletHealth = 1);

	//If for example player is dead, it needs to be disabled and move away. Is expensive
	void moveEntityOut(Entity entity);

	//Enable body and move it to pos
	void moveEntityIn(Entity entity, glm::vec2 pos);

	void destroyBody(Entity entity);

	//Only use with static or kinematic bodies, this will smoothly set the position using velocity
	void setBodyPos(glm::vec2 pos, Entity entity);

	void update(float dt);

	void checkAABBSensor();

	void jump(Entity entity);

	void sortSprites();

	glm::vec2 getVelocity(Entity entity);

	//Debug
	void drawPolygons(screen& screenObj);
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
	float timepast = 0.0f;
	std::vector<std::vector<glm::vec2>> MapShapes;
	std::vector<std::vector<b2Vec2>> MapShapesB2;
	int framePhysicsStep = -1; //We need a count for the physics step since it does not update at the same time as other functions do.
	float setPosCooldown = 0.0f;

	Entity entitiesUpdatedInFrame[32]{};
	int entitiesUpdatedSize = 0;


};

