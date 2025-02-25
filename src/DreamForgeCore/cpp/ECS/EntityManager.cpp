#include "EntityManager.hpp"
#include "errorHandling.hpp"
#include "Logging.hpp"
#include <cassert>

namespace DF {

Expect<Entity> EntityManager::makeEntity()
{
    //If we are already at maximum entities.
    if(haveReachedMaxEntities())
        return std::unexpected(Error::Code::MAX_ENTITIES_REACHED);

    mEntities[mCurrentEntityCount].setID(mCurrentEntityCount++);

    //If we have reached maximum entities after making this one.
    if(haveReachedMaxEntities())
    {
        Logger::get().fmtStdoutWarn("max entites reached {}/{}",
            mCurrentEntityCount, mCurrentEntityCount);
    }

    return mEntities[mCurrentEntityCount];
}

void EntityManager::removeEntity()
{
    
}

}