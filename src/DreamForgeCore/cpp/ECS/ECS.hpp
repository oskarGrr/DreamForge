#pragma once
#include <memory>
#include "EntityManager.hpp"
#include "ComponentManager.hpp"
#include "Logging.hpp"
#include "ECSEvents.hpp"

namespace DF 
{

class ECS
{
public:
    static auto& get()
    {
        static ECS ecs;
        return ecs;
    }
    
    //Add an entity and optionally give it any number of components.
    template <typename ...ComponentTs>
    Expect<Entity> addEntity()
    {
        auto maybeEntity = mEntityManager.makeEntity();
        if(!maybeEntity)
        {
            Logger::get().stdoutError(maybeEntity.error().getStr());
            return maybeEntity;
        }

        addComponents<ComponentTs...>(*maybeEntity);

        return maybeEntity;
    }
    
    //Remove an entity from the ECS.
    void removeEntity(Entity const& entity)
    {
        mEntityManager.removeEntity();
        mComponentManager.removeComponents(entity);
    }
    
    //Add any number of components to an already existing entity.
    template <typename ...ComponentTs>
    void addComponents(Entity const& entity)
    {
        (entity.setSignature<ComponentTs>(), ...);
        mComponentManager.insertComponents<ComponentTs...>(entity);
    }
    
    //Remove any number of components from an entity.
    template <typename ...ComponentTs>
    void removeComponents(Entity const& entity)
    {
        (mComponentManager.removeComponents<ComponentTs>(entity), ...);
    }

private:
    
    ECS()=default;
    ~ECS()=default;

    ECS(const ECS&)=delete;
    ECS(ECS&&)=delete;
    ECS& operator=(ECS const&)=delete;
    ECS& operator=(ECS&&)=delete;

    EntityManager mEntityManager;
    ComponentManager mComponentManager;
    ECSEventBus mEventBus;
};

}