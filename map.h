#ifndef MAP_H
#define MAP_H
#include <string>
#include <vector>

class map
{
	int width, height;//map size
	std::vector<std::vector<int>> map_itself;
	public:
	map();
	int getW();
	int getH();
	int whatIsThere(int _x, int _y);
	void setToLocation(int _x, int _y, int _whatToSet);
	//map(std::string path_to_map)() //read map
	void generate(int w, int h);//generate random map
};

#endif