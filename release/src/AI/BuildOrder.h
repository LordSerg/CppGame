#ifndef BUILDORDER_H
#define BUILDORDER_H

#include "../Entities/Building.h"
#include <vector>
#include <string>

enum class BuildPriority {
    CRITICAL,
    HIGH,
    MEDIUM,
    LOW
};

struct BuildItem {
    BuildingType buildingType;
    BuildPriority priority;
    bool completed;
    int requiredWorkers;
    
    BuildItem(BuildingType type, BuildPriority prio = BuildPriority::MEDIUM)
        : buildingType(type)
        , priority(prio)
        , completed(false)
        , requiredWorkers(1)
    {}
};

class BuildOrder {
public:
    BuildOrder();
    
    void SetEarlyGameOrder();
    void SetMidGameOrder();
    void SetLateGameOrder();
    
    BuildItem* GetNextBuild();
    void MarkCompleted(BuildingType type);
    
    bool IsEmpty() const { return buildQueue.empty(); }
    
private:
    std::vector<BuildItem> buildQueue;
    
    void AddBuild(BuildingType type, BuildPriority priority = BuildPriority::MEDIUM);
    void ClearQueue();
};

#endif // BUILDORDER_H