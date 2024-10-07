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
	std::vector<std::tuple<int, int>> findPath(map m, int x_0, int y_0, int x_1, int y_1);
};

#endif