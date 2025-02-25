#include "ComponentManager.hpp"
#include "EntityManager.hpp"
#include "Logging.hpp"
#include <exception>
#include <utility>

namespace DF
{

ComponentManager::ComponentManager()
{
    mComponentArrays[GetIDFromType<Transform>] = ArrayImpl<Transform>{};
}

template<class ComponentT>
ComponentManager::ArrayImpl<ComponentT>::ArrayImpl(ArrayImpl&& oldObj) noexcept
    : mSize{oldObj.mSize}, m_array{std::move(oldObj.m_array)}
{
    oldObj.m_array.reset();
}

template <class ComponentT> ComponentManager::ArrayImpl<ComponentT>& 
ComponentManager::ArrayImpl<ComponentT>::operator=
(ComponentManager::ArrayImpl<ComponentT>&& rhs) noexcept
{
    m_array.reset();
    m_array = std::move(rhs.m_array);
    rhs.m_array.reset();
    return *this;
}

template<class ComponentT> template<class ...Args>
void ComponentManager::ArrayImpl<ComponentT>::emplace(
    Entity const& entity, Args&& ...ctorArgs)
{
#ifdef DF_DEBUG
    if(!insertDataCheck()) {return;}
#endif
    
    //Since the destructors are trivial for component types, I don't think I need to
    //destruct them before calling placement new (they dont do anything...)
    //I am going to do it anyway just in case, to appease the abstract machine gods.
    auto pos = m_array.get() + mSize;
    m_array[pos].~ComponentT();
    ::new(pos) ComponentT(std::forward<Args>(ctorArgs)...);
    ++mSize;
}

//None of my component types can benefit from a move,
//but I will put this here in case that changes later
template <class ComponentT>
void ComponentManager::ArrayImpl<ComponentT>::insert(ComponentT&& toInsert, 
    Entity const& entity)
{
#ifdef DF_DEBUG
    if(!insertDataCheck()) {return;}
#endif
    m_array[mSize] = std::move(toInsert);
    ++mSize;
}

template<class ComponentT>
void ComponentManager::ArrayImpl<ComponentT>::insert(ComponentT const& toInsert,
    Entity const& entity)
{
#ifdef DF_DEBUG
    if(!insertDataCheck()) {return;}
#endif
    m_array[mSize] = toInsert;
    updateMapsOnInsert(entity, mSize);
    ++mSize;
}

//Helper method to reduce code repetition. Called in debug builds only.
template<class ComponentT>
bool ComponentManager::ArrayImpl<ComponentT>::insertDataCheck(Entity const& entity)
{
    if(mEntityToIdxMap.find(&entity) != mEntityToIdxMap.end())
    {
        Logger::get().stdoutError("attempted to add a component"
            "to an entity more than once");
        return false;
    }
    if(mSize >= getCapacity())
    {
        Logger::get().stdoutError("attempted to insert into a full component array");
        return false;
    }

    return true;
}

//Another helper method to reduce code repitition. 
//Called in copy/move insert and emplace after the insertion happened.
template<class ComponentT>
void ComponentManager::ArrayImpl<ComponentT>::updateMapsOnInsert(
    Entity const& entity, Index_t idxOfNewComponent)
{
    mEntityToIdxMap.emplace(&entity, idxOfNewComponent);
    mIdxToEntityMap.emplace(idxOfNewComponent, &entity);
}

template<class ComponentT>
void ComponentManager::ArrayImpl<ComponentT>::erase(Entity const& entity)
{
#ifdef DF_DEBUG
    if(mSize == 0)
    {
        Logger::get().stdoutError("trying to call " 
            "ComponentManager::ArrayImpl::eraseAt() from an empty component array");
        return;
    }

    if(mEntityToIdxMap.find(&entity) == mEntityToIdxMap.end())
    {
        Logger::get().stdoutError("invalid entity supplied to "
            "ComponentManager::ArrayImpl::erase()");
        return;
    }
#endif
    //The ordering of the components doesnt matter, so I will 
    //just copy the last element to fill the component we are erasing.
    //None of the components benifit from a move, so just copy.
    auto lastIdx = mSize - 1;
    auto idxToRemove = mEntityToIdxMap[&entity];
    m_array[idxToRemove] = m_array[lastIdx];

    auto endComponentEntity = mIdxToEntityMap[lastIdx];
    mEntityToIdxMap[endComponentEntity] = idxToRemove;
    mIdxToEntityMap[idxToRemove] = endComponentEntity;

    mEntityToIdxMap.erase(&entity);
    mIdxToEntityMap.erase(lastIdx);

    --mSize;
}

template <typename ComponentT> [[nodiscard]]
NonOwningPtr<Entity> ComponentManager::ArrayImpl<ComponentT>::getComponent(Entity const& entity) const
{
#ifdef DF_DEBUG
    if(mEntityToIdxMap.find(&entity) == mEntityToIdxMap.end())
    {
        Logger::get().stdoutError("requesting a component from ComponentManager::ArrayImpl"
            "<ComponentT>::getComponent() with an entity that doesnt have this component");
        return std::nullopt;
    }
#endif
    return std::cref(m_array.get() + mEntityToIdxMap[&entity]);
}

}