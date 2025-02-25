#pragma once
#include "Components.hpp"
#include "EntityManager.hpp"
#include <variant>
#include <array>
#include <cassert>
#include <memory>
#include <unordered_map>

namespace DF
{

//component manager
class ComponentManager
{
public:

    ComponentManager();
    ~ComponentManager()=default;

    ComponentManager(ComponentManager const&)=delete;
    ComponentManager(ComponentManager&&)=delete;
    ComponentManager& operator=(ComponentManager const&)=delete;
    ComponentManager& operator=(ComponentManager&&)=delete;

    template <typename ...ComponentTs>
    void insertComponents(Entity const& entity)
    {
        (mComponentArrays[GetIDFromType<ComponentTs>].emplace(entity), ...);
    }

    template <typename ...ComponentTs>
    void removeComponents(Entity const& entity)
    {
        (mComponentArrays[GetIDFromType<ComponentTs>].erase(entity), ...);
    }
    
private:

    //Component array implementation only used by the enclosing 
    //component manager class (not resizeable)
    template <class ComponentT> class ArrayImpl
    {
    public:
        ArrayImpl()=default;
        ~ArrayImpl()=default;

        ArrayImpl(ArrayImpl const&)=delete;
        ArrayImpl& operator=(ArrayImpl const&)=delete;
        ArrayImpl(ArrayImpl&&)noexcept;
        ArrayImpl& operator=(ArrayImpl&&)noexcept;

        inline constexpr auto getSize() const {return mSize;}
        inline constexpr auto getCapacity() const {return m_array.size();}
        [[nodiscard]] NonOwningPtr<Entity> getComponent(Entity const& entity) const;
        void insert(ComponentT const&, Entity const&);
        void erase(Entity const&);

        //None of my component types can benefit from a move,
        //but I will put this here in case that changes.
        void insert(ComponentT&&, Entity const&);

        //Construct a ComponentType in the array instead of 
        //copying/moving from an already existing ComponentType into the array.
        template<class ...Args> 
        void emplace(Entity const&, Args&& ...ctorArgs);

    private:
        std::unique_ptr<ComponentT[]> m_array{nullptr};

        //How many components are currently being stored
        size_t mSize{0};

        size_t mCapacity{EntityManager::getMaxEntityCount()};

        //TODO change from const Entity* to reference_wrapper
        std::unordered_map<const Entity*, Index_t> mEntityToIdxMap;
        std::unordered_map<Index_t, const Entity*> mIdxToEntityMap;

        //Helper method to reduce code repetition. Called in debug builds only.
        bool insertDataCheck(Entity const&);

        //Another helper method to reduce code repitition. 
        //Called in copy/move insert and emplace.
        void updateMapsOnInsert(Entity const&, Index_t idxOfNewComponent);

    };//class ArrayImpl

    using ComponentArray_t = std::variant
    <
        ArrayImpl<Transform>
    >;

    //The array of component arrays.
    std::array<ComponentArray_t, NUM_COMPONENT_TYPES> mComponentArrays;
};

}