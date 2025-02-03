#pragma once
#include <entt.hpp>

//Global entt object
extern entt::registry Registry;
using Entity = entt::entity;

template <typename T, typename... Args>
decltype(auto) CreateComponent(Entity entity, Args&&... args)
{
	return Registry.emplace<T>(entity, args...);
}