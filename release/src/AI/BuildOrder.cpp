#include "BuildOrder.h"
#include <algorithm>

BuildOrder::BuildOrder() {
    SetEarlyGameOrder();
}

void BuildOrder::SetEarlyGameOrder() {
    ClearQueue();
    
    // Early game focuses on economy
    AddBuild(BuildingType::HUT, BuildPriority::CRITICAL);
    AddBuild(BuildingType::FARM, BuildPriority::CRITICAL);
    AddBuild(BuildingType::SAWMILL, BuildPriority::HIGH);
    AddBuild(BuildingType::STORAGE, BuildPriority::HIGH);
    AddBuild(BuildingType::FARM, BuildPriority::HIGH);
    AddBuild(BuildingType::HUT, BuildPriority::MEDIUM);
}

void BuildOrder::SetMidGameOrder() {
    ClearQueue();
    
    // Mid game expands and adds military
    AddBuild(BuildingType::CRAFTSMAN_GUILD, BuildPriority::CRITICAL);
    AddBuild(BuildingType::MINE, BuildPriority::HIGH);
    AddBuild(BuildingType::FARM, BuildPriority::HIGH);
    AddBuild(BuildingType::FORGE, BuildPriority::HIGH);
    AddBuild(BuildingType::BARRACKS, BuildPriority::CRITICAL);
    AddBuild(BuildingType::STORAGE, BuildPriority::MEDIUM);
    AddBuild(BuildingType::TRAINING_GROUND, BuildPriority::MEDIUM);
}

void BuildOrder::SetLateGameOrder() {
    ClearQueue();
    
    // Late game focuses on military and defense
    AddBuild(BuildingType::BARRACKS, BuildPriority::HIGH);
    AddBuild(BuildingType::TOWER, BuildPriority::HIGH);
    AddBuild(BuildingType::TRAINING_GROUND, BuildPriority::HIGH);
    AddBuild(BuildingType::FORGE, BuildPriority::MEDIUM);
    AddBuild(BuildingType::FARM, BuildPriority::MEDIUM);
}

BuildItem* BuildOrder::GetNextBuild() {
    if (buildQueue.empty()) return nullptr;
    
    // Find highest priority uncompleted item
    auto it = std::find_if(buildQueue.begin(), buildQueue.end(),
        [](const BuildItem& item) { return !item.completed; });
    
    if (it != buildQueue.end()) {
        return &(*it);
    }
    
    return nullptr;
}

void BuildOrder::MarkCompleted(BuildingType type) {
    for (auto& item : buildQueue) {
        if (item.buildingType == type && !item.completed) {
            item.completed = true;
            break;
        }
    }
}

void BuildOrder::AddBuild(BuildingType type, BuildPriority priority) {
    buildQueue.push_back(BuildItem(type, priority));
}

void BuildOrder::ClearQueue() {
    buildQueue.clear();
}