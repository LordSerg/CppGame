#ifndef CELL_H
#define CELL_H
#include "LandType.h"

class Cell
{
    LandType land = none;
    std::vector<Cell*> nghbrs;
    int x,y;
public:
    Cell();
    Cell(LandType land_filling, int X, int Y);
    LandType getLand();
    void setLand(LandType land_filling);
    void AddNeighbour(Cell *N);
    int getX();
    int getY();
    std::vector<Cell*> neighbours();
    /*navigational things*/
    bool checked=false; //if the cell was visited
    float globalDist;
    float localDist;
    Cell* from;
    void resetNavigationalThings();
};
#endif