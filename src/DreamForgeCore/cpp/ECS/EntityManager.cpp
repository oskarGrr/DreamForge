#include "EntityManager.hpp"
#include "errorHandling.hpp"
#include "Logging.hpp"
#include <cassert>

DFExpect<Entity> EntityManager::makeEntity()
{
    //If we are already at maximum entities.
    if(haveReachedMaxEntities())
        return std::unexpected(DFError::Code::MAX_ENTITIES_REACHED);

    m_entities[m_currentEntityCount].setID(m_currentEntityCount++);

    //If we have reached maximum entities after making this one.
    if(haveReachedMaxEntities())
    {
        DFLog::get().fmtStdoutWarn("max entites reached {}/{}",
            m_currentEntityCount, m_currentEntityCount);
    }

    return m_entities[m_currentEntityCount];
}

void EntityManager::removeEntity()
{
    
}