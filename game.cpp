#include "game.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

static void game::SaveGame(std::string path)
{
	std::string mp = map::Serialize();
	std::string pl = p.Serialize();
	std::vector<std::string> ais(computers.size())
	for(int i=0;i<computers.size();++i)
		ais[i] = computers[i].Serialize();
	std::ofstream gameFile (path);
	if (gameFile.is_open())
	{
		gameFile<<mp<<","<<pl<<",";
		for(auto a : ais)
			gameFile<<a<<",";
		myfile.close();
	}
}

static void game::LoadGame(std::string path)
{
	
}
static void game::NewGame(std::string path_to_map)
{
	
}
static void game::Step(){}
static player game::getPlayer(){}
