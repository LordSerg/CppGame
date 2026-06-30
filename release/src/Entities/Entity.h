#ifndef ENTITY_H
#define ENTITY_H

#include "../Utils/Math.h"
#include <string>

enum class EntityType {
    UNIT,
    BUILDING,
    OBSTACLE
};

class Entity {
public:
    Entity(int id, EntityType type, int ownerId);
    virtual ~Entity() = default;
    
    virtual void Update(float deltaTime) = 0;
    virtual void Render(class Renderer* renderer) = 0;
    
    // Getters
    int GetId() const { return id; }
    EntityType GetType() const { return type; }
    int GetOwnerId() const { return ownerId; }
    Vector2 GetPosition() const { return position; }
    Point2D GetGridPosition() const;
    Rect GetBounds() const;
    
    int GetMaxHealth() const { return maxHealth; }
    int GetCurrentHealth() const { return currentHealth; }
    bool IsAlive() const { return currentHealth > 0; }
    
    // Setters
    void SetPosition(const Vector2& pos) { position = pos; }
    void SetPosition(float x, float y) { position = Vector2(x, y); }
    void SetOwnerId(int owner) { ownerId = owner; }
    
    // Health management
    void TakeDamage(int damage);
    void Heal(int amount);
    
    // Selection
    bool IsSelected() const { return selected; }
    void SetSelected(bool value) { selected = value; }
    
    // Serialization
    virtual void Serialize(class SaveSystem* saveSystem);
    virtual void Deserialize(class SaveSystem* saveSystem);
    
protected:
    int id;
    EntityType type;
    int ownerId; // -1 for neutral, 0+ for players
    
    Vector2 position;
    int width;  // in tiles
    int height; // in tiles
    
    int maxHealth;
    int currentHealth;
    
    bool selected;
    
    std::string spriteName;
};

#endif // ENTITY_H