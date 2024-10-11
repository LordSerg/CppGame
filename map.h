#ifndef MAP_H
#define MAP_H

#include <string>
#include <vector>

class map
{
	int width, height;//map size
	std::vector<std::vector<int>> map_itself;
	public:
	
	/*size of current map*/
	static int getW();
	static int getH();
	
	/*set and get things that are on the map*/
	static int whatIsThere(int _x, int _y);
	static void setToLocation(int _x, int _y, int _whatToSet);
	
	/*read map from saved file*/
	static void loadMap(std::string path);
	
	/*save map to the file*/
	static void openMap(std::string path);	

	//generate random map
	static void generate(int w, int h);
	
	
	static std::vector<std::tuple<int, int>> findPath(int x_0, int y_0, int x_1, int y_1);
};

#endif