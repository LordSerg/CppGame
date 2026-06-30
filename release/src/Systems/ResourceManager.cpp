#include "ResourceManager.h"

ResourceManager::ResourceManager() {
}

void ResourceManager::InitializePlayer(int playerId) {
    if (playerResources.find(playerId) == playerResources.end()) {
        playerResources[playerId] = PlayerResources();
    }
}

bool ResourceManager::CanAfford(int playerId, const ResourceCost& cost) const {
    auto it = playerResources.find(playerId);
    if (it == playerResources.end()) return false;
    
    const PlayerResources& res = it->second;
    
    if (res.wood < cost.wood) return false;
    if (res.metal < cost.metal) return false;
    if (res.food < cost.food) return false;
    if (res.currentPopulation + cost.population > res.maxPopulation) return false;
    
    return true;
}

bool ResourceManager::SpendResources(int playerId, const ResourceCost& cost) {
    if (!CanAfford(playerId, cost)) return false;
    
    auto it = playerResources.find(playerId);
    if (it == playerResources.end()) {
        InitializePlayer(playerId);
        it = playerResources.find(playerId);
    }
    
    PlayerResources& res = it->second;
    
    res.wood -= cost.wood;
    res.metal -= cost.metal;
    res.food -= cost.food;
    res.currentPopulation += cost.population;
    
    return true;
}

void ResourceManager::AddResources(int playerId, int wood, int metal, int food) {
    auto it = playerResources.find(playerId);
    if (it == playerResources.end()) {
        InitializePlayer(playerId);
        it = playerResources.find(playerId);
    }
    
    PlayerResources& res = it->second;
    
    res.wood = std::min(res.maxWood, res.wood + wood);
    res.metal = std::min(res.maxMetal, res.metal + metal);
    res.food = std::min(res.maxFood, res.food + food);
}

const PlayerResources& ResourceManager::GetPlayerResources(int playerId) const {
    static PlayerResources empty;
    auto it = playerResources.find(playerId);
    if (it == playerResources.end()) return empty;
    return it->second;
}

int ResourceManager::GetWood(int playerId) const {
    return GetPlayerResources(playerId).wood;
}

int ResourceManager::GetMetal(int playerId) const {
    return GetPlayerResources(playerId).metal;
}

int ResourceManager::GetFood(int playerId) const {
    return GetPlayerResources(playerId).food;
}

int ResourceManager::GetCurrentPopulation(int playerId) const {
    return GetPlayerResources(playerId).currentPopulation;
}

int ResourceManager::GetMaxPopulation(int playerId) const {
    return GetPlayerResources(playerId).maxPopulation;
}

void ResourceManager::AddPopulation(int playerId, int amount) {
    auto it = playerResources.find(playerId);
    if (it == playerResources.end()) {
        InitializePlayer(playerId);
        it = playerResources.find(playerId);
    }
    
    it->second.currentPopulation += amount;
}

void ResourceManager::RemovePopulation(int playerId, int amount) {
    auto it = playerResources.find(playerId);
    if (it != playerResources.end()) {
        it->second.currentPopulation = std::max(0, it->second.currentPopulation - amount);
    }
}

void ResourceManager::IncreaseMaxPopulation(int playerId, int amount) {
    auto it = playerResources.find(playerId);
    if (it == playerResources.end()) {
        InitializePlayer(playerId);
        it = playerResources.find(playerId);
    }
    
    it->second.maxPopulation += amount;
}

void ResourceManager::DecreaseMaxPopulation(int playerId, int amount) {
    auto it = playerResources.find(playerId);
    if (it != playerResources.end()) {
        it->second.maxPopulation = std::max(0, it->second.maxPopulation - amount);
    }
}

void ResourceManager::IncreaseStorageCapacity(int playerId, int amount) {
    auto it = playerResources.find(playerId);
    if (it == playerResources.end()) {
        InitializePlayer(playerId);
        it = playerResources.find(playerId);
    }
    
    it->second.maxWood += amount;
    it->second.maxMetal += amount;
    it->second.maxFood += amount;
}

void ResourceManager::DecreaseStorageCapacity(int playerId, int amount) {
    auto it = playerResources.find(playerId);
    if (it != playerResources.end()) {
        it->second.maxWood = std::max(1000, it->second.maxWood - amount);
        it->second.maxMetal = std::max(1000, it->second.maxMetal - amount);
        it->second.maxFood = std::max(1000, it->second.maxFood - amount);
    }
}

ResourceCost ResourceManager::GetUnitCost(UnitType unitType) {
    switch (unitType) {
        case UnitType::PEASANT:
            return ResourceCost(50, 0, 10, 1);
        case UnitType::WARRIOR:
            return ResourceCost(100, 50, 20, 1);
        case UnitType::VETERAN:
            return ResourceCost(0, 0, 0, 0); // Upgraded, not bought
        case UnitType::PALADIN:
            return ResourceCost(0, 0, 0, 0); // Upgraded, not bought
        default:
            return ResourceCost();
    }
}

ResourceCost ResourceManager::GetBuildingCost(BuildingType buildingType) {
    switch (buildingType) {
        case BuildingType::FARM:
            return ResourceCost(100, 0, 0, 0);
        case BuildingType::SAWMILL:
            return ResourceCost(150, 0, 0, 0);
        case BuildingType::HUT:
            return ResourceCost(80, 0, 0, 0);
        case BuildingType::STORAGE:
            return ResourceCost(120, 0, 0, 0);
        case BuildingType::CRAFTSMAN_GUILD:
            return ResourceCost(200, 50, 0, 0);
        case BuildingType::FORGE:
            return ResourceCost(150, 100, 0, 0);
        case BuildingType::BARRACKS:
            return ResourceCost(200, 100, 0, 0);
        case BuildingType::ROAD:
            return ResourceCost(10, 0, 0, 0);
        case BuildingType::WALL:
            return ResourceCost(50, 20, 0, 0);
        case BuildingType::GATE:
            return ResourceCost(80, 30, 0, 0);
        case BuildingType::MINE:
            return ResourceCost(150, 50, 0, 0);
        case BuildingType::TOWER:
            return ResourceCost(150, 100, 0, 0);
        case BuildingType::TRAINING_GROUND:
            return ResourceCost(200, 80, 0, 0);
        default:
            return ResourceCost();
    }
}

ResourceCost ResourceManager::GetTechCost(const std::string& techName) {
    // Define costs for each technology
    if (techName == "Axes") return ResourceCost(50, 0, 0, 0);
    if (techName == "Urban planning") return ResourceCost(100, 50, 0, 0);
    if (techName == "Mining") return ResourceCost(150, 50, 0, 0);
    if (techName == "Military") return ResourceCost(150, 100, 0, 0);
    if (techName == "Roads") return ResourceCost(80, 30, 0, 0);
    if (techName == "Walls") return ResourceCost(100, 50, 0, 0);
    if (techName == "Metal Processing") return ResourceCost(120, 80, 0, 0);
    if (techName == "Pickaxes") return ResourceCost(60, 40, 0, 0);
    if (techName == "Sledgehammers") return ResourceCost(60, 40, 0, 0);
    if (techName == "Shields1") return ResourceCost(80, 60, 0, 0);
    if (techName == "Shields2") return ResourceCost(120, 100, 0, 0);
    if (techName == "Shields3") return ResourceCost(180, 150, 0, 0);
    if (techName == "Swords1") return ResourceCost(80, 60, 0, 0);
    if (techName == "Swords2") return ResourceCost(120, 100, 0, 0);
    if (techName == "Swords3") return ResourceCost(180, 150, 0, 0);
    if (techName == "Tools1") return ResourceCost(100, 80, 0, 0);
    if (techName == "Tools2") return ResourceCost(150, 120, 0, 0);
    if (techName == "Tools3") return ResourceCost(200, 180, 0, 0);
    if (techName == "Health1") return ResourceCost(100, 50, 0, 0);
    if (techName == "Health2") return ResourceCost(150, 100, 0, 0);
    if (techName == "Health3") return ResourceCost(200, 150, 0, 0);
    
    return ResourceCost();
}

ResourceCost ResourceManager::GetUpgradeCost(UnitType fromType, UnitType toType) {
    if (fromType == UnitType::PEASANT && toType == UnitType::WARRIOR) {
        return ResourceCost(50, 100, 30, 0);
    }
    if (fromType == UnitType::WARRIOR && toType == UnitType::VETERAN) {
        return ResourceCost(100, 150, 50, 0);
    }
    if (fromType == UnitType::VETERAN && toType == UnitType::PALADIN) {
        return ResourceCost(150, 200, 80, 0);
    }
    
    return ResourceCost();
}

void ResourceManager::Serialize(SaveSystem* saveSystem) {
    // Serialize all player resources
}

void ResourceManager::Deserialize(SaveSystem* saveSystem) {
    // Deserialize all player resources
}