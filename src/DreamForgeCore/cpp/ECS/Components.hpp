#pragma once
#include "HelpfulTypeAliases.hpp"
#include "glm/glm.hpp"
#include "BiDirectionalTypeIntMap.hpp"

struct Transform
{
    glm::vec2 position{};
    glm::vec2 scale{};
    F64 rotation{};
};

//Below are meta functions for a compile time type to integer map.
//see BiDirectionalTypeIntMap.hpp
//Remeber to update the macro when you add and remove component types.

#define TYPE_REGISTRY TypeRegistry< \
    Transform> \

//Get the type mapped to ID.
template <Index_t ID>
using GetTypeFromID = TYPE_REGISTRY::IndexedMap::type<ID>;

//Get the ID mapped to ComponentT.
template <typename ComponentT>
inline static Index_t GetIDFromType =
    TYPE_REGISTRY::IndexedMap::index<ComponentT>;

inline constexpr auto NUM_COMPONENT_TYPES = TYPE_REGISTRY::sNumTypes;
inline constexpr auto MAX_NUM_COMPONENT_TYPES = TYPE_REGISTRY::sMaxNumTypes;