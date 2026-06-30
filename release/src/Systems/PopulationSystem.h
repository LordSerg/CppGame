#ifndef POPULATIONSYSTEM_H
#define POPULATIONSYSTEM_H

class Map;
class ResourceManager;

class PopulationSystem {
public:
    PopulationSystem(Map* map, ResourceManager* resourceMgr);
    
    void Update(float deltaTime);
    
    void OnUnitCreated(int playerId);
    void OnUnitDestroyed(int playerId);
    void OnFarmBuilt(int playerId);
    void OnFarmDestroyed(int playerId);
    
private:
    Map* map;
    ResourceManager* resourceManager;
    
    const int populationPerFarm = 5;
};

#endif // POPULATIONSYSTEM_H