#include "map.h"

map::map()
{
	std::cout<<"Make new map\n";
	generate(100,100);
}
map::void generate(int w, int h)
{
	std::cout<<"map "<<w<<" x "<<h<<" is generated\n";
	width=w;
	height=h;
	for(int i=0;i<width;++i)
	{
		for(int j=0;j<height;++j)
		{
			map_itself[i][j]=0;
		}
	}
}
map::int whatIsThere(int _x, int _y)
{
	return map_itself[_x][_y];
}

map::void setToLocation(int _x, int _y, int _whatToSet)
{
	map_itself[_x][_y] = _whatToSet;
}

map::int getW()
{
	return width;
}
map::int getH()
{
	return height;
}