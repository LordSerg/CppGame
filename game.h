#ifndef GAMEGAME_H
#define GAMEGAME_H

#include "AI.h"
//#include "player.h" in AI.h is already included


class game
{
	player* p;
	std::vector<*AI> computers;
public:
	/*open/close game*/
	static void SaveGame(std::string path);
	static void LoadGame(std::string path);
	
	/*new game with new map*/
	static void NewGame(std::string path_to_map);
	
	/*main loop, one step for each*/
	static void Step();
	
	/*for operating player's units*/
	static player getPlayer();
}

#endif