#include "Map.h"
#include "Unit.h"

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

    map[19][35]->setLand(LandType::obstacle);
    map[19][36]->setLand(LandType::obstacle);
    map[19][37]->setLand(LandType::obstacle);
    map[19][38]->setLand(LandType::obstacle);

    map[33][36]->setLand(LandType::obstacle);
    map[34][36]->setLand(LandType::obstacle);
    map[35][36]->setLand(LandType::obstacle);
    map[36][36]->setLand(LandType::obstacle);

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