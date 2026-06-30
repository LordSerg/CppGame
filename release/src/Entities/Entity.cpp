#include "Entity.h"
#include <algorithm>

Entity::Entity(int id, EntityType type, int ownerId)
    : id(id)
    , type(type)
    , ownerId(ownerId)
    , position(0, 0)
    , width(1)
    , height(1)
    , maxHealth(100)
    , currentHealth(100)
    , selected(false)
{
}

Point2D Entity::GetGridPosition() const {
    return Point2D((int)(position.x / 32.0f), (int)(position.y / 32.0f));
}

Rect Entity::GetBounds() const {
    return Rect(
        (int)(position.x / 32.0f),
        (int)(position.y / 32.0f),
        width,
        height
    );
}

void Entity::TakeDamage(int damage) {
    currentHealth = std::max(0, currentHealth - damage);
}

void Entity::Heal(int amount) {
    currentHealth = std::min(maxHealth, currentHealth + amount);
}

void Entity::Serialize(SaveSystem* saveSystem) {
    // Base serialization
}

void Entity::Deserialize(SaveSystem* saveSystem) {
    // Base deserialization
}