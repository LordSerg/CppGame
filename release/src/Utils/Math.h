#ifndef MATH_H
#define MATH_H

#include <cmath>
#include <algorithm>

struct Vector2 {
    float x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(float _x, float _y) : x(_x), y(_y) {}
    
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }
    
    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }
    
    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
    
    float Length() const {
        return std::sqrt(x * x + y * y);
    }
    
    Vector2 Normalized() const {
        float len = Length();
        if (len > 0) {
            return Vector2(x / len, y / len);
        }
        return Vector2(0, 0);
    }
    
    float Distance(const Vector2& other) const {
        return (*this - other).Length();
    }
};

struct Point2D {
    int x, y;
    
    Point2D() : x(0), y(0) {}
    Point2D(int _x, int _y) : x(_x), y(_y) {}
    
    bool operator==(const Point2D& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Point2D& other) const {
        return !(*this == other);
    }
    
    bool operator<(const Point2D& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

struct Rect {
    int x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), width(_w), height(_h) {}
    
    bool Contains(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    bool Contains(const Point2D& p) const {
        return Contains(p.x, p.y);
    }
    
    bool Intersects(const Rect& other) const {
        return !(x >= other.x + other.width || 
                 x + width <= other.x ||
                 y >= other.y + other.height ||
                 y + height <= other.y);
    }
};

namespace Math {
    inline float Clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }
    
    inline int Clamp(int value, int min, int max) {
        return std::max(min, std::min(max, value));
    }
    
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
}

#endif // MATH_H