#include "ComponentManager.hpp"
#include "EntityManager.hpp"
#include "Logging.hpp"
#include <exception>
#include <utility>

namespace DF
{

ComponentManager::ComponentManager()
{
    m_componentArrays[GetIDFromType<Transform>] = ArrayImpl<Transform>{};
}

template<class ComponentT>
ComponentManager::ArrayImpl<ComponentT>::ArrayImpl(ArrayImpl&& oldObj) noexcept
    : m_size{oldObj.m_size}, m_array{std::move(oldObj.m_array)}
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
    auto pos = m_array.get() + m_size;
    m_array[pos].~ComponentT();
    ::new(pos) ComponentT(std::forward<Args>(ctorArgs)...);
    ++m_size;
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
    m_array[m_size] = std::move(toInsert);
    ++m_size;
}

template<class ComponentT>
void ComponentManager::ArrayImpl<ComponentT>::insert(ComponentT const& toInsert,
    Entity const& entity)
{
#ifdef DF_DEBUG
    if(!insertDataCheck()) {return;}
#endif
    m_array[m_size] = toInsert;
    updateMapsOnInsert(entity, m_size);
    ++m_size;
}

//Helper method to reduce code repetition. Called in debug builds only.
template<class ComponentT>
bool ComponentManager::ArrayImpl<ComponentT>::insertDataCheck(Entity const& entity)
{
    if(m_entityToIdxMap.find(&entity) != m_entityToIdxMap.end())
    {
        Logger::get().stdoutError("attempted to add a component"
            "to an entity more than once");
        return false;
    }
    if(m_size >= getCapacity())
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
    m_entityToIdxMap.emplace(&entity, idxOfNewComponent);
    m_idxToEntityMap.emplace(idxOfNewComponent, &entity);
}

template<class ComponentT>
void ComponentManager::ArrayImpl<ComponentT>::erase(Entity const& entity)
{
#ifdef DF_DEBUG
    if(m_size == 0)
    {
        Logger::get().stdoutError("trying to call " 
            "ComponentManager::ArrayImpl::eraseAt() from an empty component array");
        return;
    }

    if(m_entityToIdxMap.find(&entity) == m_entityToIdxMap.end())
    {
        Logger::get().stdoutError("invalid entity supplied to "
            "ComponentManager::ArrayImpl::erase()");
        return;
    }
#endif
    //The ordering of the components doesnt matter, so I will 
    //just copy the last element to fill the component we are erasing.
    //None of the components benifit from a move, so just copy.
    auto lastIdx = m_size - 1;
    auto idxToRemove = m_entityToIdxMap[&entity];
    m_array[idxToRemove] = m_array[lastIdx];

    auto endComponentEntity = m_idxToEntityMap[lastIdx];
    m_entityToIdxMap[endComponentEntity] = idxToRemove;
    m_idxToEntityMap[idxToRemove] = endComponentEntity;

    m_entityToIdxMap.erase(&entity);
    m_idxToEntityMap.erase(lastIdx);

    --m_size;
}

template <typename ComponentT> [[nodiscard]]
NonOwningPtr<Entity> ComponentManager::ArrayImpl<ComponentT>::getComponent(Entity const& entity) const
{
#ifdef DF_DEBUG
    if(m_entityToIdxMap.find(&entity) == m_entityToIdxMap.end())
    {
        Logger::get().stdoutError("requesting a component from ComponentManager::ArrayImpl"
            "<ComponentT>::getComponent() with an entity that doesnt have this component");
        return std::nullopt;
    }
#endif
    return std::cref(m_array.get() + m_entityToIdxMap[&entity]);
}

}