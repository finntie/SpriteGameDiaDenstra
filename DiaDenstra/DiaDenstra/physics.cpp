#include "physics.h"
#include "register.hpp"
#include "common.h"
#include "spritetransform.h"
#include "screen.h"
#include "sprite.h"

void physics::init()
{
	worldDef = b2DefaultWorldDef();
	worldDef.gravity = { 0.0f, -10.0f };
	worldId = b2CreateWorld(&worldDef);
	
}



void physics::createBodyMap()
{
	//from glm::vec2 to b2vec2
	std::vector<b2Vec2> MapShapeB2;
	for (int i = 0; i < int(MapShapes.size()); i++)
	{
		MapShapeB2.clear();
		for (int j = 0; j < int(MapShapes[i].size()); j++)
		{
			MapShapeB2.push_back({ MapShapes[i][j].x, SCRHEIGHT - MapShapes[i][j].y});
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

		chaindef.count = int(MapShapes[i].size());
		chaindef.points = &MapShapesB2[i][0];
		b2CreateChain(BodyID, &chaindef);
		//Chain ID can be obtained through createChain if wanted.
	}
}



void physics::createBody(Entity entity, shape Shape, type Type, glm::vec2 pos, std::vector<glm::vec2> points)
{
	auto& bodyID = CreateComponent<b2BodyId>(entity);

	if (Type == staticObj)
	{
		b2BodyDef groundBodyDef = b2DefaultBodyDef();
		groundBodyDef.position = b2Vec2{ pos.x, pos.y };
		bodyID = b2CreateBody(worldId, &groundBodyDef);
		b2Polygon groundBox = b2MakeBox(50.0f, 10.0f);
		b2ShapeDef groundShapeDef = b2DefaultShapeDef();
		b2CreatePolygonShape(bodyID, &groundShapeDef, &groundBox);
	}
	else
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = static_cast<b2BodyType>(Type);
		bodyDef.position = b2Vec2{ pos.x, pos.y };
		bodyID = b2CreateBody(worldId, &bodyDef);
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = 1.0f;
		shapeDef.friction = 0.3f;
		//Create shape
		b2Polygon bodyShape;
		b2ShapeId shapeID;
		if (Shape != circle)
		{
			//glm::vec2 to b2vec2
			std::vector<b2Vec2> pointsb2;
			for (int i = 0; i < int(points.size()); i++)
			{
				pointsb2.push_back({points[i].x, points[i].y});
			}


			b2Hull CHull = b2ComputeHull(&pointsb2[0], int(pointsb2.size()));
			bodyShape = b2MakePolygon(&CHull, 0.5f);
			shapeID = b2CreatePolygonShape(bodyID, &shapeDef, &bodyShape);
		}

		b2CreatePolygonShape(bodyID, &shapeDef, &bodyShape);
	}
	//Pass bodyID to registry
}

void physics::createBodyBox(Entity entity, type Type, glm::vec2 pos, float height, float width)
{
	height *= 0.5f;
	width *= 0.5f;
	createBody(entity, box, Type, pos, { {-width, height}, {-width, -height}, {width, -height}, {width, height} });//Counter clock-wise
}

void physics::update()
{
	b2World_Step(worldId, timeStep, subStepCount);
	for (const auto& [spriteEntity, body, transform, Sprite] : Registry.view<b2BodyId, Stransform, spriteStr>().each())
	{
		b2Vec2 pos = b2Body_GetPosition(body);
		printf("x %f, y %f\n", pos.x, pos.y);
		
		
		//Set position
		transform.setTranslation({ pos.x - Sprite.getAfterWidth() * 0.5f, pos.y - Sprite.getAfterHeight() * 0.5f });

		if (b2Body_GetType(body) == b2BodyType::b2_kinematicBody)
		{
			
			b2Body_SetLinearVelocity(body, { transform.getVel().x * 1000, transform.getVel().y * 1000 });
		}
	}
}

void physics::sortSprites()
{
	std::sort(drawOrder.begin(), drawOrder.end(), [](const auto& a, const auto& b)
	{
		return Registry.get<spriteStr>(a).depth < Registry.get<spriteStr>(b).depth; // Lower depth renders first
	});
}


void physics::drawTriangles(screen& screenObj)
{
	//DebugDraw onto background 
	for (int i = 0; i < int(Triangles.triangles.size()); i++)
	{
		for (int j = 1; j < int(Triangles.triangles[i].vertices.size()); j++)
		{
			screenObj.Line(float(Triangles.vertices[Triangles.triangles[i].vertices[j - 1]].x), float(Triangles.vertices[Triangles.triangles[i].vertices[j - 1]].y), float(Triangles.vertices[Triangles.triangles[i].vertices[j]].x), float(Triangles.vertices[Triangles.triangles[i].vertices[j]].y), (255 << 24) + 255);
		}
		screenObj.Line(float(Triangles.vertices[Triangles.triangles[i].vertices[2]].x), float(Triangles.vertices[Triangles.triangles[i].vertices[2]].y), float(Triangles.vertices[Triangles.triangles[i].vertices[0]].x), float(Triangles.vertices[Triangles.triangles[i].vertices[0]].y), (255 << 24) + 255);

		//Engine.DebugRenderer().AddLine(
		//	DebugCategory::Detailed,
		//	glm::vec2(Triangles.vertices[Triangles.triangles[i].vertices[2]].x, Triangles.vertices[Triangles.triangles[i].vertices[2]].y),
		//	glm::vec2(Triangles.vertices[Triangles.triangles[i].vertices[0]].x, Triangles.vertices[Triangles.triangles[i].vertices[0]].y),
		//	glm::vec4(col, 1.0));
	}
}

void physics::drawBodies(screen& screenObj)
{
	for (const auto& [spriteEntity, bodies, transform ] : Registry.view<b2BodyId, Stransform>().each())
	{
		b2Vec2 position = b2Body_GetPosition(bodies);
		//b2Rot rotation = b2Body_GetRotation(bodies);
		//printf("%4.2f %4.2f %4.2f\n", position.x, position.y, b2Rot_GetAngle(rotation));

		//Inverse y
		position.y = SCRHEIGHT - position.y;


		float boxSize = 50.0f;
		screenObj.Box(int(position.x - boxSize), int(position.y - boxSize), int(position.x + boxSize), int(position.y + boxSize), (255 << 24) + (255 << 8));
	}
}
