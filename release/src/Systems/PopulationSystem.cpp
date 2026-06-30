#include "PopulationSystem.h"
#include "../Map/Map.h"
#include "../Systems/ResourceManager.h"

PopulationSystem::PopulationSystem(Map* map, ResourceManager* resourceMgr)
    : map(map)
    , resourceManager(resourceMgr)
{
}

void PopulationSystem::Update(float deltaTime) {
    // Population is managed through events
}

void PopulationSystem::OnUnitCreated(int playerId) {
    resourceManager->AddPopulation(playerId, 1);
}

void PopulationSystem::OnUnitDestroyed(int playerId) {
    resourceManager->RemovePopulation(playerId, 1);
}

void PopulationSystem::OnFarmBuilt(int playerId) {
    resourceManager->IncreaseMaxPopulation(playerId, populationPerFarm);
}

void PopulationSystem::OnFarmDestroyed(int playerId) {
    resourceManager->DecreaseMaxPopulation(playerId, populationPerFarm);
}