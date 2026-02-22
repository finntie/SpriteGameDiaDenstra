#include "physics.h"
#include "register.hpp"
#include "common.h"
#include "spritetransform.h"
#include "screen.h"
#include "sprite.h"
#include "player.h"

void physics::init()
{
	worldDef = b2DefaultWorldDef();
	worldDef.hitEventThreshold = 10.0f;
	worldDef.gravity = { 0.0f, -9.81f };
	worldId = b2CreateWorld(&worldDef);
	
}



void physics::createBodyMap()
{
	//from glm::vec2 to b2vec2
	//Also dividing by 100 so that physics are not pixels to meters but pixels to centimeter.
	std::vector<b2Vec2> MapShapeB2;
	for (int i = 0; i < int(MapShapes.size()); i++)
	{
		MapShapeB2.clear();
		for (int j = 0; j < int(MapShapes[i].size()); j++)
		{
			MapShapeB2.push_back({ MapShapes[i][j].x * INVERSECENTIMETER, (-MapShapes[i][j].y) * INVERSECENTIMETER });
		}
		MapShapesB2.push_back(MapShapeB2);
	}

	//For each obstacle, create closed loop chain
	for (int i = 0; i < int(MapShapesB2.size()); i++)
	{
		b2BodyDef BodyDef = b2DefaultBodyDef();
		BodyDef.position = { 0.0f, 0.0f };
		b2BodyId BodyID = b2CreateBody(worldId, &BodyDef);
		b2ChainDef chaindef = b2DefaultChainDef();
		chaindef.isLoop = true;
		chaindef.filter.categoryBits = ground;
		chaindef.count = int(MapShapes[i].size());
		chaindef.points = &MapShapesB2[i][0];
		b2CreateChain(BodyID, &chaindef);
		//Chain ID can be obtained through createChain if wanted.
	}
}



void physics::createBody(const Entity& entity, const shape Shape, const type Type, glm::vec2 pos, std::vector<glm::vec2> points, categoryType category, categoryType mask, const int groupIdx)
{
	//Convert Pos 
	pos *= INVERSECENTIMETER;
	
	//Check if there already exists a body on this entity
	b2BodyId* bodyID = nullptr;
	bodyID = Registry.try_get<b2BodyId>(entity);

	//If it does not exist, create a new one
	if (bodyID == nullptr)
	{
		bodyID = &CreateComponent<b2BodyId>(entity);
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = static_cast<b2BodyType>(Type);
		bodyDef.position = b2Vec2{ pos.x, pos.y };
		bodyDef.fixedRotation = true;
		bodyDef.linearDamping = 0.0f;
		bodyDef.gravityScale = 10.0f; //Scale gravity lower
		*bodyID = b2CreateBody(worldId, &bodyDef);
	}

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 5.0f; // Surface area * density = mass
	shapeDef.friction = 0.0f;

	if (category != none) shapeDef.filter.categoryBits = category; 
	if (mask != none) shapeDef.filter.maskBits = mask;
	if (groupIdx != -1) shapeDef.filter.groupIndex = groupIdx * -1;

	//Create shape
	b2ShapeId shapeID;
	if (Shape != circle && Shape != capsule)
	{
		b2Polygon bodyShape;
		//glm::vec2 to b2vec2 and convert
		std::vector<b2Vec2> pointsb2;
		for (int i = 0; i < int(points.size()); i++)
		{
			pointsb2.push_back({ points[i].x * INVERSECENTIMETER, points[i].y * INVERSECENTIMETER });
		}


		b2Hull CHull = b2ComputeHull(&pointsb2[0], int(pointsb2.size()));
		bodyShape = b2MakePolygon(&CHull, 0.005f);
		bodyShape.radius = 0.005f; //Round edges
		shapeID = b2CreatePolygonShape(*bodyID, &shapeDef, &bodyShape);
	}
	else if (Shape == capsule)
	{
		//glm::vec2 to b2vec2 and convert
		std::vector<b2Vec2> pointsb2;
		for (int i = 0; i < int(points.size()); i++)
		{
			pointsb2.push_back({ points[i].x * INVERSECENTIMETER, points[i].y * INVERSECENTIMETER });
		}
		b2Capsule bodyShape;
		bodyShape.center1 = pointsb2[0];
		bodyShape.center2 = pointsb2[1];
		bodyShape.radius = pointsb2[2].x;
		shapeID = b2CreateCapsuleShape(*bodyID, &shapeDef, &bodyShape);
	}
	else if (Shape == circle)
	{
		//glm::vec2 to b2vec2 and convert
		std::vector<b2Vec2> pointsb2;
		for (int i = 0; i < int(points.size()); i++)
		{
			pointsb2.push_back({ points[i].x * INVERSECENTIMETER, points[i].y * INVERSECENTIMETER });
		}
		b2Circle bodyShape;
		bodyShape.center = pointsb2[0];
		bodyShape.radius = pointsb2[1].x;
		shapeID = b2CreateCircleShape(*bodyID, &shapeDef, &bodyShape);
	}


	//Pass bodyID to registry
}

void physics::createBodyBox(Entity entity, type Type, glm::vec2 pos, float height, float width)
{
	height *= 0.5f;
	width *= 0.5f;
	createBody(entity, polygon, Type, pos, { {-width, height}, {-width, -height}, {width, -height}, {width, height} });//Counter clock-wise
}

void physics::createBoxAroundSprite(Entity entity, type Type)
{
	spriteStr* Sprite = nullptr;
	Stransform* transform = nullptr;
	Sprite = Registry.try_get<spriteStr>(entity);
	transform = Registry.try_get<Stransform>(entity);

	if (Sprite != nullptr && transform != nullptr)
	{
		float width =  float(Sprite->getAfterWidth()) * 0.5f;
		float height = float(Sprite->getAfterHeight()) * 0.5f;
		
		createBody(entity, polygon, Type, transform->getTranslation(), {{-width, height}, {-width, -height}, {width, -height}, {width, height}});//Counter clock-wise
	}
	else
	{
		printf("Error: Tried creating bounding box for non-existing sprite/transform\n");
	}
}

void physics::createSensor(const Entity entity, const categoryType type, const categoryType mask, glm::vec2 pos, float size, int& customID)
{
	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);

	if (Body != nullptr)
	{
		b2Vec2 position = { pos.x * INVERSECENTIMETER, pos.y * INVERSECENTIMETER };

		b2Polygon sensorShape = b2MakeOffsetBox(size * INVERSECENTIMETER * 0.5f, size * INVERSECENTIMETER * 0.5f, position, {1,1});
		b2ShapeDef sensorDef = b2DefaultShapeDef();
		sensorDef.isSensor = true;
		sensorDef.filter.categoryBits = type;
		sensorDef.filter.maskBits = mask; //Set mask so that only when hit this mask an event will be called
		b2ShapeId shapeID = b2CreatePolygonShape(*Body, &sensorDef, &sensorShape); //Hopefully the shape is added and not overrun?	
		customID = shapeID.index1;
	}
}

void physics::createAABBboxSensor(const Entity entity, glm::vec2 pos, float width, float height, const uint64_t mask, int groupIndex, std::string name, int& customID)
{
	//Convert Pos 
	pos *= INVERSECENTIMETER;
	width *= INVERSECENTIMETER * 0.5f;
	height *= INVERSECENTIMETER * 0.5f;

	//Check if there already exists a body on this entity
	b2BodyId* bodyID = nullptr;
	bodyID = Registry.try_get<b2BodyId>(entity);

	//If it does not exist, create a new one
	if (bodyID == nullptr)
	{
		bodyID = &CreateComponent<b2BodyId>(entity);
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_staticBody;
		bodyDef.position = b2Vec2{ pos.x, pos.y };
		bodyDef.fixedRotation = true;
		*bodyID = b2CreateBody(worldId, &bodyDef);
	}

	auto& AABB = CreateComponent<AABBSensor>(entity);
	AABB.alive = true;
	AABB.name = name;

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.isSensor = true;
	shapeDef.filter.categoryBits = bullet;
	shapeDef.filter.maskBits = player | ownplayer; //Ownplayer because the groupfilter will mask out the own bullets
	shapeDef.filter.groupIndex = groupIndex * -1;

	b2Polygon bodyShape = b2MakeOffsetBox(width * 0.5f, height * 0.5f, { 0.0f, 0.0f }, { 1,1 });
	bodyShape.radius = 0.005f; //Round edges
	//bodyShape.centroid = { pos.x,pos.y };
	b2ShapeId shapeID = b2CreatePolygonShape(*bodyID, &shapeDef, &bodyShape);
	customID = shapeID.index1;

}


void physics::attachJoint(const Entity entity1, const Entity entity2, float distance, glm::vec2 anchorPos1, glm::vec2 anchorPos2)
{
	b2BodyId* Body1 = nullptr;
	b2BodyId* Body2 = nullptr;
	Body1 = Registry.try_get<b2BodyId>(entity1);
	Body2 = Registry.try_get<b2BodyId>(entity2);
	

	if (Body1 != nullptr && Body2 != nullptr)
	{
		b2DistanceJointDef jointDef = b2DefaultDistanceJointDef();
		jointDef.bodyIdA = *Body1;
		jointDef.bodyIdB = *Body2;
		b2Vec2 APos1 = { anchorPos1.x * INVERSECENTIMETER, anchorPos1.y * INVERSECENTIMETER };
		b2Vec2 APos2 = { anchorPos2.x * INVERSECENTIMETER, anchorPos2.y * INVERSECENTIMETER };
		jointDef.localAnchorA = APos1;
		jointDef.localAnchorB = APos2;
		jointDef.length = distance * INVERSECENTIMETER;
		b2JointId jointID = b2CreateDistanceJoint(worldId, &jointDef);
	}
}



int2 physics::sensorTouchedBody(Entity entity, int customID, int& frameChecked, shape Shape)
{
	if (frameChecked == framePhysicsStep) return { 0,0 };

	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		int2 output = { 0,0 };
		frameChecked = framePhysicsStep;

		b2ShapeId ShapeID;
		if (!isSensor(*Body, customID, Shape, ShapeID))
		{
			return { 0,0 };
		}
		b2SensorEvents sensorEvents = b2World_GetSensorEvents(b2Body_GetWorld(*Body));
		
		

		for (int i = 0; i < sensorEvents.beginCount; ++i) //Loop over all sensor events
		{
			b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i; //Grab begin touch event

			//Check if our sensor did anything
			if (B2_ID_EQUALS(beginTouch->sensorShapeId, ShapeID)) //Checks index, worlds and revision
			{
				//If mask is set up correctly, this should indicate that we have begin touching.
				b2ShapeType type = b2Shape_GetType(beginTouch->sensorShapeId);
				output.x++;
			}
		}
		for (int i = 0; i < sensorEvents.endCount; ++i) //Loop over all sensor events
		{
			//Also check if left body
			b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i; //Grab begin touch event

			//Check if our sensor did anything
			if (B2_ID_EQUALS(endTouch->sensorShapeId, ShapeID)) //Checks index, worlds and revision
			{
				//If mask is set up correctly, this should indicate that we have ended touching
				b2ShapeType type = b2Shape_GetType(endTouch->sensorShapeId);
				output.y++;
			}
		}
		
		return output;
	}
	return { 0,0 };
}

void physics::getAllSensoredBodies(Entity entity, int customID, int& frameChecked, Entity* outputArray, int& outputSize, shape Shape)
{
	outputSize = 0;
	if (frameChecked == framePhysicsStep) return;

	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		frameChecked = framePhysicsStep;

		b2ShapeId ShapeID;
		if (!isSensor(*Body, customID, Shape, ShapeID))
		{
			return;
		}
		b2SensorEvents sensorEvents = b2World_GetSensorEvents(b2Body_GetWorld(*Body));
		

		for (int i = 0; i < sensorEvents.beginCount; ++i) //Loop over all sensor events
		{
			b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i; //Grab begin touch event

			//Check if our sensor did anything
			if (B2_ID_EQUALS(beginTouch->sensorShapeId, ShapeID)) //Checks index, worlds and revision
			{
				//If mask is set up correctly, this should indicate that we have begin touching.
				b2ShapeType type = b2Shape_GetType(beginTouch->sensorShapeId);
				b2BodyId otherBody = b2Shape_GetBody(beginTouch->visitorShapeId);
				for (const auto& [objectEntity, transform, BodyCheck] : Registry.view<Stransform, b2BodyId>().each())
				{
					if (B2_ID_EQUALS(otherBody, BodyCheck))
					{
						outputArray[outputSize] = objectEntity;
						outputSize++;
						break;
					}
				}
			}
		}
	}
}

bool physics::isSensor(const b2BodyId& Body, const int customID, const shape Shape, b2ShapeId& ShapeID)
{
	//Try get shape
	b2ShapeId Shapes[8]; //There probably won't be more than 8 shapes
	int size = b2Body_GetShapes(Body, Shapes, 8);

	//Loop over every shape to see if it is our shape
	for (int i = 0; i < size; i++)
	{
		if (b2Shape_IsSensor(Shapes[i]) && b2Shape_GetType(Shapes[i]) == static_cast<b2ShapeType>(Shape) && Shapes[i].index1 == customID)
		{
			ShapeID = Shapes[i];
			return true;
		}
	}

	printf("Can't find sensor in specified entity\n");
	return false;
}

bool physics::getHitEntity(Entity entity, Entity& outputEntity, int& frameChecked, bool& hitObject, uint64_t filter)
{

	hitObject = false;
	if (frameChecked == framePhysicsStep) return false;

	frameChecked = framePhysicsStep;

	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		//printf("vel.x: %f\n", b2Body_GetLinearVelocity(*Body).x);


		//Get shape
		b2ShapeId Shapes[8]; //There probably won't be more than 8 shapes
		int size = b2Body_GetShapes(*Body, Shapes, 8);
		b2ContactEvents contactEvent = b2World_GetContactEvents(worldId);
		b2BodyId Body2;
		bool isShape = false;

		for (int i = 0; i < contactEvent.hitCount; i++) //Loop over hit events
		{
			b2ContactHitEvent hitEvent = contactEvent.hitEvents[i]; //Get hit event
			//Check if our body has a shape that hit
			int j = 0;
			while (j < size) //Loop over hit events
			{
				if (B2_ID_EQUALS(Shapes[j], hitEvent.shapeIdB))
				{
					isShape = true;
					Body2 = b2Shape_GetBody(hitEvent.shapeIdA);
					break;
				}
				j++;
			}
			if (isShape) //A shape of ours has an hit event
			{
				//Check if shape is one of our set filters using AND bit-operator
				if (b2Shape_GetFilter(hitEvent.shapeIdA).categoryBits & filter) hitObject = true;				

				//Check to which entity the other shape belongs
				for (const auto& [objectEntity, transform, BodyCheck] : Registry.view<Stransform, b2BodyId>().each())
				{
					if (B2_ID_EQUALS(Body2, BodyCheck))
					{
						outputEntity = objectEntity;
						return true;
					}
				}
			}
			else
			{
				outputEntity = Entity{};
			}
		}
	}
	return false;
}

void physics::setRotOfEntity(Entity entity, glm::vec2 dir)
{
	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		b2Body_SetAngularVelocity(*Body,  dir.x);
	}

}

void physics::createBullet(Entity entity, glm::vec2 startPos, float rotation, float speed, float size, int playerNumber, int health)
{
	//Shape is of course a circle

	b2BodyId& bodyID = CreateComponent<b2BodyId>(entity);
	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = b2Vec2{ startPos.x * INVERSECENTIMETER, startPos.y * INVERSECENTIMETER };
	bodyDef.fixedRotation = false;
	bodyDef.isBullet = true;
	//bodyDef.gravityScale = 0.3f; //Scale gravity lower
	b2Vec2 rot;
	rot.x = std::cos(glm::radians(rotation));
	rot.y = std::sin(glm::radians(rotation));
	bodyDef.rotation.c = rot.x;
	bodyDef.rotation.s = rot.y;
	bodyID = b2CreateBody(worldId, &bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.8f;
	shapeDef.friction = 0.0f;
	shapeDef.restitution = 1.0f;
	shapeDef.filter.categoryBits = bullet;
	shapeDef.filter.maskBits = ground | player | ownplayer; //Ownplayer because the groupfilter will mask out the own bullets
	shapeDef.filter.groupIndex = playerNumber * -1;

	//Create shape
	b2ShapeId shapeID;

	b2Circle bodyShape;
	bodyShape.center = { 0,0 };
	bodyShape.radius = size * INVERSECENTIMETER;
	shapeID = b2CreateCircleShape(bodyID, &shapeDef, &bodyShape);
	b2Shape_EnableHitEvents(shapeID, true); //Enable to receive hit events

	//Add velocity to body
	b2Body_ApplyLinearImpulseToCenter(bodyID, { rot.x * speed, rot.y * speed }, true);

}

void physics::moveEntityOut(Entity entity)
{
	b2BodyId* Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		b2Body_SetTransform(*Body, { -1000, -1000 }, b2Body_GetRotation(*Body));
		b2Body_Disable(*Body);
	}
}

void physics::moveEntityIn(Entity entity, glm::vec2 pos)
{
	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		b2Body_SetTransform(*Body, { pos.x * INVERSECENTIMETER, pos.y * INVERSECENTIMETER }, b2Body_GetRotation(*Body));
		b2Body_Enable(*Body);
	}
}

void physics::destroyBody(Entity entity)
{
	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		b2DestroyBody(*Body);
	}

}

void physics::setBodyPos(glm::vec2 pos, Entity entity)
{
	//Check if entity already is updated in this frame
	int i = 0;
	for (i; i < entitiesUpdatedSize; i++)
	{
		if (entity == entitiesUpdatedInFrame[i]) return;
	}
	if (entitiesUpdatedSize < 31)
	{
		entitiesUpdatedInFrame[i + 1] = entity;
		entitiesUpdatedSize++;


		b2BodyId* Body = nullptr;
		Body = Registry.try_get<b2BodyId>(entity);
		if (Body != nullptr)
		{
			b2Vec2 targetPosition = { pos.x * INVERSECENTIMETER, pos.y * INVERSECENTIMETER };

			b2Body_SetTransform(*Body, targetPosition, b2Body_GetRotation(*Body));
		}
	}
	else
	{
		printf("Issue: Max Enities Updated\n");
	}
}

void physics::update(float dt)
{
	timepast -= dt;
	if (timepast <= 0.0f)
	{
		framePhysicsStep++; 
		if (framePhysicsStep > (255 << 8)) framePhysicsStep = 0;
		timepast = timeStep + timepast;
		b2World_Step(worldId, timeStep, subStepCount);
		entitiesUpdatedSize = 0;
		for (const auto& [spriteEntity, body, transform, Sprite] : Registry.view<b2BodyId, Stransform, spriteStr>().each())
		{

			b2Vec2 pos = b2Body_GetPosition(body);
			pos *= CENTIMETER;
			//Set position to physics object
			transform.setTranslation({ pos.x, pos.y });

			//Set player physics
			playerStr* Player = nullptr;
			Player = Registry.try_get<playerStr>(spriteEntity);
			if (Player != nullptr) //TODO: check if it our own player
			{
				// Using method from box2D tutorial https://www.iforce2d.net/b2dtut/constant-speed

				b2Vec2 vel = b2Body_GetLinearVelocity(body);
				float velChange = transform.getVel().x - vel.x;
				float impulse = b2Body_GetMass(body) * velChange;
				b2Body_ApplyLinearImpulseToCenter(body, { impulse, 0 }, true);
			}

			checkAABBSensor();
		}
	}
}

void physics::checkAABBSensor()
{
	for (const auto& [spriteEntity, body, AABB] : Registry.view<b2BodyId, AABBSensor>().each())
	{
		if (AABB.alive) AABB.alive = false;
		else
		{
			//Delete sensor (it lived already 1 frame)
			//b2DestroyBody(body);
			//Registry.destroy(spriteEntity, 1);
			//printf("destroyed sensor\n");
		}
	}
}

void physics::jump(Entity entity)
{
	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		float mass = b2Body_GetMass(*Body);
		float jumpVelocity = 50.0f;
		
		//First make sure that our y velocity is 0 and then apply velocity, else we possibly bounce or jump lower
		b2Body_SetLinearVelocity(*Body, { b2Body_GetLinearVelocity(*Body).x, 0.0f });
		b2Body_ApplyLinearImpulseToCenter(*Body, { 0, mass * jumpVelocity}, true);
	}
	else
	{
		printf("Player has no PhysicsBody\n");
	}

}

void physics::sortSprites()
{
	std::sort(drawOrder.begin(), drawOrder.end(), [](const auto& a, const auto& b)
	{
		return Registry.get<spriteStr>(a).depth < Registry.get<spriteStr>(b).depth; // Lower depth renders first
	});
}

glm::vec2 physics::getVelocity(Entity entity)
{
	b2BodyId* Body = nullptr;
	Body = Registry.try_get<b2BodyId>(entity);
	if (Body != nullptr)
	{
		b2Vec2 vel = b2Body_GetLinearVelocity(*Body);
		return { vel.x, vel.y };
	}

	return glm::vec2();
}


void physics::drawPolygons(screen& screenObj)
{
	for (int i = 0; i < int(MapShapes.size()); i++)
	{
		for (int j = 0; j < int(MapShapes[i].size()); j++)
		{
			screenObj.Line(MapShapes[i][j].x, MapShapes[i][j].y, MapShapes[i][(j + 1) % MapShapes[i].size()].x, MapShapes[i][(j + 1) % MapShapes[i].size()].y, (255 << 24) + 255);
		}
	}
	
}

void physics::drawBodies(screen& screenObj)
{
	for (const auto& [spriteEntity, body, transform ] : Registry.view<b2BodyId, Stransform>().each())
	{
		b2Vec2 position = b2Body_GetPosition(body);
		position *= CENTIMETER;
		//b2Rot rotation = b2Body_GetRotation(bodies);
		//printf("%4.2f %4.2f %4.2f\n", position.x, position.y, b2Rot_GetAngle(rotation));

		//Inverse y
		position.y = SCRHEIGHT - position.y;
		b2ShapeId Shapes[5];
		int size = b2Body_GetShapes(body, Shapes, 5); 
		b2ShapeType shapeType = b2Shape_GetType(Shapes[0]);

		if (size != 0)
		{
			if (shapeType == b2_polygonShape)
			{
				//TODO: draw lines for each vertex instead of a box

				b2Polygon polygon = b2Shape_GetPolygon(Shapes[0]);

				b2Vec2 pos1 = polygon.vertices[0] * CENTIMETER + position;
				b2Vec2 pos2 = polygon.vertices[2] * CENTIMETER + position;

				screenObj.Box(int(pos1.x), int(pos1.y), int(pos2.x), int(pos2.y), (255 << 24) + (255 << 8));
			}
			else if (shapeType == b2_capsuleShape)
			{
				b2Capsule capsule = b2Shape_GetCapsule(Shapes[0]);
				b2Vec2 pos1 = capsule.center1 * CENTIMETER + position;
				b2Vec2 pos2 = capsule.center2 * CENTIMETER + position;
				screenObj.Circle(int(pos1.x), int(pos1.y), int(capsule.radius * CENTIMETER), (255 << 24) + (255 << 8));
				screenObj.Circle(int(pos2.x), int(pos2.y), int(capsule.radius * CENTIMETER), (255 << 24) + (255 << 8));

			}
			else if (shapeType == b2_circleShape)
			{
				b2Circle circle = b2Shape_GetCircle(Shapes[0]);
				b2Vec2 pos1 = circle.center * CENTIMETER + position;
				screenObj.Circle(int(pos1.x), int(pos1.y), int(circle.radius * CENTIMETER), (255 << 24) + (255 << 8));


			}
		}
	}
}
