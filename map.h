#ifndef MAP_H
#define MAP_H

#include <string>
#include <vector>

/*a minimal map element*/
class node
{
	bool is_barier = false;
	int thing_that_is_here = 0;
	std::vector<node*> neighbours;
	
};

/*the whoke map operations*/
class map
{
	int width, height;//map size
	std::vector<std::vector<node>> map_itself;
	
public:
	
	/*size of current map*/
	static int getW();
	static int getH();
	
	/*set and get things that are on the map*/
	static int whatIsThere(int _x, int _y);
	static void setToLocation(int _x, int _y, int _whatToSet);
	
	/*read map from saved file*/
	static void loadMap(std::string path);
	
	/*for saving in file*/
	static std::string Serialize();
	static std::string Deserialize();
	
	/*save map to the file*/
	static void openMap(std::string path);	

	//generate random map
	static void generate(int w, int h);
	
	static std::vector<std::tuple<int, int>> findPath(int x_0, int y_0, int x_1, int y_1);
};

#endif