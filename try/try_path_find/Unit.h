#ifndef UNIT_H
#define UNIT_H

#include "Map.h"

class Unit
{
    int x,y;
    int targetX,targetY;
    Map *theMap;
    std::vector<Cell*> path;
    void findPath();
public:
    Unit();
    Unit(int X, int Y, Map *map);
    int X();
    int Y();
    void goTo(int newTargetX, int newTargetY);
    void ShowPath();
};

#endif