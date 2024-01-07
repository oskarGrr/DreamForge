#pragma once
#include "HelpfulTypeAliases.hpp"
#include <utility>//std::integer_sequence, std::make_index_sequence
#include <type_traits>//std::integral_constant

//Below is a bi-directional compile time type to integer map.
//Use like this to combine types to specific integers:
/*
 
#define TYPE_REGISTRY_COMBINE \
        TypeRegistry <int, char, bool, float > :: \    <- the types in the map
        CombineWith  <4,   12,    4,   13    >         <- the integers they are paired with

//Get the type mapped to ID.
template <U64 ID>
using GetType = TYPE_REGISTRY_COMBINE::Map::type<ID>

//Get the ID mapped to T.
template <typename T>
static U64 getID = TYPE_REGISTRY_COMBINE::Map::index<T>

*/

//Or use like this to automatically combine types with 
//indicies / integer sequence 0, 1, 2, 4 etc:
/*

//int will automatically be paired with 0, 
//std::vector<int> will be paired with 1, bool with 2, and unsigned int with 3.
#define TYPE_REGISTRY TypeRegistry<int, std::vector<int>, bool, unsigned>

//Get the type mapped to the given index
template <U64 idx>
using GetIndex = TYPE_REGISTRY::IndexedMap::type<idx>

//Get the index mapped to the given type
template <typename T>
static constexpr U64 GetIndex = TYPE_REGISTRY::IndexedMap::index<T>

*/

template <U64 integer, typename T> 
struct IntegerTypePair
{
    using type = T;
    static constexpr U64 sInteger = integer;

    static IntegerTypePair getPairFromIndex(
        std::integral_constant<U64, integer>) {return {};}

    static IntegerTypePair getPairFromType(T) {return {};}
};

template <typename ...IDTypePairs> 
struct BiDirectionalMap : IDTypePairs...
{
    using IDTypePairs::getPairFromIndex...;
    using IDTypePairs::getPairFromType...;

    template <U64 ID>
    using type = typename decltype(
        getPairFromIndex(std::integral_constant<U64, ID>{}))::type;

    template <typename T>
    static constexpr U64 index = decltype(getPairFromType(T{}))::sInteger;
};

template <typename ...Ts>
struct TypeRegistry
{
    static constexpr size_t sMaxNumTypes{64};
    static constexpr size_t sNumTypes{sizeof...(Ts)};

    static_assert(sizeof...(Ts) < sMaxNumTypes,
        "Exceeded maximum number of types in the TypeRegistry");

    template <U64 ...IDs>
    struct CombineWith
    {
        static_assert(sizeof...(Ts) == sizeof...(IDs), 
            "There are mismatching Types to IDs in the TypeRegistry");

        using Map = BiDirectionalMap<IntegerTypePair<IDs, Ts>...>;
    };

    template <typename T, T... indecies>
    static auto createIndexedMap(std::integer_sequence<T, indecies...>)
        -> BiDirectionalMap<IntegerTypePair<indecies, Ts>...> {return {};}

    using IndexedMap = decltype(createIndexedMap(
        std::make_index_sequence<sizeof...(Ts)>{}));
};