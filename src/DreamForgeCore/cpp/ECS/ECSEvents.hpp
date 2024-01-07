#pragma once
#include <vector>
#include "BiDirectionalTypeIntMap.hpp"

struct ECSEvent
{
    
};

class ECSEventBus
{
public:
    using NotifyFuncT = void (*)(const ECSEvent&);

    template <typename EventT>
    void notify(const EventT& evnt)
    {
        const auto& funcs = mNotifyFuncTable[0];

        for(const auto func : funcs)
        {
            func(evnt);
        }
    }

    template <typename EventT>
    void subscribe(NotifyFuncT fn)
    {
        mNotifyFuncTable[0].push_back(fn);
    }

private:
    std::vector<std::vector<NotifyFuncT>> mNotifyFuncTable;

};