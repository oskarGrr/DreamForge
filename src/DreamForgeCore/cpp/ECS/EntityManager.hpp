#pragma once
#include <array>
#include <cstdint>
#include <bitset>
#include "Components.hpp"
#include "errorHandling.hpp"
#include "HelpfulTypeAliases.hpp"

namespace DF
{

class Entity
{
public:

    template <typename T> 
    void setSignature()
    {
        m_signature.set(GetIDFromType<T>);
    }

    void zeroSignatureBits() {m_signature.reset();}
    auto getID() const {return m_entityID;}
    void setID(U64 id) {m_entityID = id;}
private:
    U64 m_entityID{0};
    std::bitset<64> m_signature{};
};

class EntityManager
{
public:
    EntityManager()=default;
    ~EntityManager()=default;

    EntityManager(EntityManager const&)=delete;
    EntityManager(EntityManager&&)=delete;
    EntityManager& operator=(EntityManager const&)=delete;
    EntityManager& operator=(EntityManager&&)=delete;

    inline static constexpr auto getMaxEntityCount() {return s_maxEntities;}
    inline auto getCurrentEntityCount() const {return m_currentEntityCount;}
    inline auto haveReachedMaxEntities() const {return m_currentEntityCount >= getMaxEntityCount();}
    [[nodiscard("dont forget your entity")]] Expect<Entity> makeEntity();

    void removeEntity();

private:
    inline constexpr static size_t s_maxEntities{1000};
    std::array<Entity, s_maxEntities> m_entities{};
    size_t m_currentEntityCount{0};
};

}