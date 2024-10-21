#include <iostream>
#include <stack>
#include <vector>
#include <algorithm>
#include <list>
#include <math.h>
//every unit can do some tasks, like go to the point, fight, build ect.
//to manage actions of units and update it on map - use class "task".
//every unit is connected to its task
//or rather every task has its unit.

//these things are needed for main game loop.


//-----------------------stuff that was in "try_path_find"-----------------------
enum LandType
{
    none,
    obstacle,
    unit,
    building,
    mine
};

class Cell
{
    LandType land = none;
    std::vector<Cell*> nghbrs;
    int x,y;
public:
    Cell();
    Cell(LandType land_filling, int X, int Y);
    ~Cell();
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

class Map
{
    int h=30;
    int w=30;
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

class Unit
{
    int x,y;
    int targetX,targetY;
    Map *theMap;
    void findPath();
public:
    std::vector<Cell*> path;
    Unit();
    Unit(int X, int Y, Map *map);
    int X();
    int Y();
    void goTo(int newTargetX, int newTargetY);
    void ShowPath();
    void SetLocation(int newX, int newY);
    //std::vector<Cell*> GetPath();
};

Cell::Cell()
{
    land=LandType::none;
    x=y=0;
    nghbrs.clear();
}
Cell::Cell(LandType land_filling, int X, int Y)
{
    land = land_filling;
    x=X;
    y=Y;
}
Cell::~Cell()
{
    //free memory
}
LandType Cell::getLand()
{
    return land;
}
void Cell::setLand(LandType land_filling)
{
    land = land_filling;
}
void Cell::AddNeighbour(Cell *N)
{
    if(std::find(nghbrs.begin(),nghbrs.end(),N)==nghbrs.end())
    {
        nghbrs.push_back(N);
    }
}
int Cell::getX(){return x;}
int Cell::getY(){return y;}
std::vector<Cell*> Cell::neighbours(){return nghbrs;}

void Cell::resetNavigationalThings()
{
    checked = false;
    globalDist = std::numeric_limits<float>::max();
    localDist = std::numeric_limits<float>::max();
    from = nullptr;
}

float Map::dist(int x1,int y1,int x2,int y2)//for heuristic
{
    return static_cast<float>(std::sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)));
}

Map::Map()
{
    for(int i=0;i<h;++i)
        map.push_back(std::vector<Cell*>());
    
    for(int i=0;i<h;++i)
    {
        for(int j=0;j<w;++j)
        {
            map[i].push_back(new Cell(LandType::none, j, i));
        }
    }
    //setting the map:
    
    //making connections of the map:

    for(int i=0;i<h;++i)
    {
        for(int j=0;j<w;++j)
        {
            if(i>0)
                map[i][j]->AddNeighbour(map[i-1][j]);
            if(j>0)
                map[i][j]->AddNeighbour(map[i][j-1]);
            if(i<h-1)
                map[i][j]->AddNeighbour(map[i+1][j]);
            if(j<w-1)
                map[i][j]->AddNeighbour(map[i][j+1]);
            if(i>0&&j>0)
                map[i][j]->AddNeighbour(map[i-1][j-1]);
            if(i>0&&j<w-1)
                map[i][j]->AddNeighbour(map[i-1][j+1]);
            if(i<h-1&&j>0)
                map[i][j]->AddNeighbour(map[i+1][j-1]);
            if(i<h-1&&j<w-1)
                map[i][j]->AddNeighbour(map[i+1][j+1]);
        }
    }

    //borders:
    for(int i=0;i<w;++i)
    {
        map[0][i]->setLand(LandType::obstacle);
        map[h-1][i]->setLand(LandType::obstacle);
    }
    for(int i=0;i<h;++i)
    {
        map[i][0]->setLand(LandType::obstacle);
        map[i][w-1]->setLand(LandType::obstacle);
    }

    //set unit:
    //map[23][21]->setLand(LandType::unit);

    //set other obstacles:
    map[18][22]->setLand(LandType::obstacle);
    map[19][22]->setLand(LandType::obstacle);
    map[20][22]->setLand(LandType::obstacle);
    map[21][22]->setLand(LandType::obstacle);
    map[22][22]->setLand(LandType::obstacle);
    map[22][21]->setLand(LandType::obstacle);
    map[22][20]->setLand(LandType::obstacle);

    map[23][18]->setLand(LandType::obstacle);
    map[24][18]->setLand(LandType::obstacle);
    map[25][18]->setLand(LandType::obstacle);
    map[26][18]->setLand(LandType::obstacle);
    map[27][18]->setLand(LandType::obstacle);
    map[28][18]->setLand(LandType::obstacle);

    map[28][20]->setLand(LandType::obstacle);
    map[28][21]->setLand(LandType::obstacle);
    map[28][22]->setLand(LandType::obstacle);

    map[26][22]->setLand(LandType::obstacle);
    map[26][23]->setLand(LandType::obstacle);
    map[26][24]->setLand(LandType::obstacle);
    map[26][25]->setLand(LandType::obstacle);
    map[27][25]->setLand(LandType::obstacle);
    map[28][25]->setLand(LandType::obstacle);


    

}
void Map::ShowMap()
{
    for(auto y:map)
    {
        for(auto x:y)
        {
            if(x->getLand()==LandType::none)
                std::cout<<"-";
            else if(x->getLand()==LandType::obstacle)
                std::cout<<"X";
            else if(x->getLand()==LandType::unit)
                std::cout<<"0";
            else if(x->getLand()==LandType::building)
                std::cout<<"B";
            else if(x->getLand()==LandType::mine)
                std::cout<<"M";
            else
                std::cout<<"-";

        }
        std::cout<<"\n";
    }
}
void Map::SetElem(int x, int y, LandType newLandFillment)
{
    map[y][x]->setLand(newLandFillment);
}
int Map::getWidth(){return w;}
int Map::getHeight(){return h;}
std::vector<Cell*> Map::FindPath(int Xfrom, int Yfrom, int Xto, int Yto)
{
    //A* algorithm of pathfinding
    //if there is no access to the cell at all - put the limit on "wandering around"
    int limit = 5000;//

    //reset navigational things of the map
    for(auto a:map)
        for(auto c:a)
            c->resetNavigationalThings();
    
    Cell* current = map[Yfrom][Xfrom];
    current->localDist=0.0f;
    current->globalDist=dist(current->getX(), current->getY(), Xto, Yto);
    std::list<Cell*> toVisit;
    toVisit.push_back(map[Yfrom][Xfrom]);

    while(!toVisit.empty() && limit>=0 && current!=map[Yto][Xto])
    {
        //sorting unvisited cells from best(nearest) to worst (furthest)
        toVisit.sort([](const Cell* a, const Cell* b){return a->globalDist<b->globalDist;});
        //doing clear things with our list
        while(!toVisit.empty() && toVisit.front()->checked)
            toVisit.pop_front();

        //in case there are no cells left
        if(toVisit.empty())
        {
            limit=-1;
            break;
        }

        //say hello to the best of the cells:
        current = toVisit.front();
        current->checked=true;

        //adding neighbours of the new cell:
        for(auto c:current->neighbours())
        {
            if(!c->checked&&c->getLand()==LandType::none)
            {
                toVisit.push_back(c);
            }
            float tmp_local_dist = current->localDist + dist(current->getX(), current->getY(), c->getX(), c->getY());

            if(tmp_local_dist < c->localDist)
            {
                c->from=current;
                c->localDist = tmp_local_dist;
                c->globalDist = c->localDist + dist(c->getX(),c->getY(), Xto, Yto);
            }
        }

        limit--;
    }

    if (limit<0)
    {
        return std::vector<Cell*>();
    }

    std::vector<Cell*> answer;
    while (current!=nullptr)
    {
        answer.push_back(current);
        current = current->from;
    }
    std::reverse(answer.begin(),answer.end());
    return answer;
}

void Unit::findPath()
{
    if(targetX==x&&targetY==y)
    {
        path.clear();
        return;
    }
    path = theMap->FindPath(x,y,targetX, targetY);
}
Unit::Unit()
{
    targetX=targetY=x=y=0;
}
Unit::Unit(int X, int Y, Map *map)
{
    x=targetX=X;
    y=targetY=Y;
    theMap=map;
    theMap->SetElem(x, y, LandType::unit);
}
int Unit::X(){return x;}
int Unit::Y(){return y;}

void Unit::goTo(int newTargetX, int newTargetY)
{
    targetX=newTargetX;
    targetY=newTargetY;
    findPath();
}
//temporary for console
void Unit::ShowPath()
{
    for(int i=0;i<path.size()-1;++i)
        std::cout<<"("<<path[i]->getX()<<", "<<path[i]->getY()<<")=>";
    std::cout<<"("<<path[path.size()-1]->getX()<<", "<<path[path.size()-1]->getY()<<")\n";

}
void Unit::SetLocation(int newX, int newY)
{
    //removing us from map on the one place
    theMap->SetElem(x,y,LandType::none);
    //...and pasting us on other place
    x=newX;
    y=newY;
    theMap->SetElem(x,y,LandType::unit);
}
//----------------------- Task management (new) classes-----------------------
enum TaskType
{
    idle,
    to_go,
    to_fight,
    to_build,
    to_mine
};

class Building
{
    int x,y;
    int width, height;
public:
    Building()
    {
        x=y=0;
        width=height=1;
    }
    Building(int X, int Y, int size, Map* map)
    {
        x = X;
        y = Y;
        width = height = size;
        for(int j=y;j<y+height;++j)
            for(int i=x;i<x+width;++i)
                map->SetElem(i,j,LandType::building);
    }

    int getX(){return x;}
    int getY(){return y;}



};

class Mine:public Building
{
public:
    Mine():Building(){}
    Mine(int X, int Y, int size, Map* map):Building{X,Y,size, map}
    {
        for(int j=Y;j<Y+size;++j)
            for(int i=X;i<X+size;++i)
                map->SetElem(i,j,LandType::mine);
    }
};

class Task
{
	Unit* myUnit;
	std::stack<TaskType> curTask;
    //temporary variables
    int x, y;
    Building* b;
    Unit* otherUnit;
	
	void go()
    {
        if(myUnit->path.size()!=0)
        {
            myUnit->SetLocation(myUnit->path[0]->getX(), myUnit->path[0]->getY());
            delete myUnit->path[0];
            //myUnit->path[0]->~Cell();
            myUnit->path.erase(myUnit->path.begin());//remove first elem
        }
        else
        {
            if(!curTask.empty())
                curTask.pop();
        }
    }
	void mine()
    {

    }
	void fight()
    {

    }
	void build()
    {

    }
public:
    Task(){}
    ~Task(){}
    void setNewTask(Unit *u, TaskType newTask, int X, int Y)
    {
        //for tasks like "go"
        myUnit = u;
        //empty stack
        while(curTask.size()!=0)
            curTask.pop();
        curTask.push(newTask);
        x=X;
        y=Y;
        myUnit->goTo(x,y);
    }
    void doTask()
    {
        if(curTask.empty()==true)
		{
		    this->~Task();
			return;
		}
		else if(curTask.top()==TaskType::idle)
		{
			curTask.pop();
		}
		else if(curTask.top()==TaskType::to_go)
		{
			go();
		}
		else if(curTask.top()==TaskType::to_mine)
		{
			mine();
		}
		else if(curTask.top()==TaskType::to_fight)
		{
			fight();
		}
		else if(curTask.top()==TaskType::to_build)
		{
			build();
		}
    }
    bool isIdle()
    {
        return curTask.empty();
    }
};

int main()
{
    //init of map and units
    Map map = Map();
    Unit* u1=new Unit(1,1,&map);
    Unit* u2=new Unit(21,23,&map);

    Task* t1 = new Task();
    t1->setNewTask(u1,TaskType::to_go, 5, 5);
    
    Task* t2 = new Task();
    t2->setNewTask(u2,TaskType::to_go, 22, 8);
    
    //set other things on map
    Building*b1 = new Building(7,7,2,&map);
    Mine*m1 = new Mine(2,23,3, &map);
    //set task for unit
    
    //main loop emulator
    int iter=0;
    while(!t1->isIdle() || !t2->isIdle())
    {
        if(!t1->isIdle())
            t1->doTask();
        if(!t2->isIdle())
            t2->doTask();
        std::cout<<(iter++)<<"\n";
        map.ShowMap();
    }
    system("pause");
}


/*
"0" is for units
"-" is for free space
"X" is for obstacle
"A" is for target
"!" is for path
map:
                       1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
00 X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X
01 X 0 - - - - - - - - - - - - - - - - - - - - - - - - - - - X
02 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
03 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
04 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
05 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
06 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
07 X - - - - - - B B - - - - - - - - - - - - - - - - - - - - X
08 X - - - - - - B B - - - - - - - - - - - - - - - - - - - - X
09 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
10 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
11 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
12 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
13 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
14 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
15 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
16 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
17 X - - - - - - - - - - - - - - - - - - - - - - - - - - - - X
18 X - - - - - - - - - - - - - - - - - - - - - X - - - - - - X
19 X - - - - - - - - - - - - - - - - - - - - - X - - - - - - X
20 X - - - - - - - - - - - - - - - - - - - - - X - - - - - - X
21 X - - - - - - - - - - - - - - - - - - - - - X - - - - - - X
22 X - - - - - - - - - - - - - - - - - - - X X X - - - - - - X
23 X - M M M - - - - - - - - - - - - - X - - 0 - - - - - - - X
24 X - M M M - - - - - - - - - - - - - X - - - - - - - - - - X
25 X - M M M - - - - - - - - - - - - - X - - - - - - - - - - X
26 X - - - - - - - - - - - - - - - - - X - - - X X X X - - - X
27 X - - - - - - - - - - - - - - - - - X - - - - - - X - - - X
28 X - - - - - - - - - - - - - - - - - X - X X X - - X - - - X
29 X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X

*/