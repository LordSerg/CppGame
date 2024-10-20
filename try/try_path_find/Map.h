#ifndef MAP_H
#define MAP_H

#include "LandType.h"
#include "Cell.h"

class Map
{
    int h=50;
    int w=50;
    std::vector<std::vector<Cell*>> map{};
    float dist(int x1,int y1,int x2,int y2);
public:
    Map();
    void ShowMap();
    void SetElem(int x, int y, LandType newLandFillment);
    int getWidth();
    int getHeight();
    std::vector<Cell*> FindPath(int Xfrom, int Yfrom, int Xto, int Yto);
};

#endif