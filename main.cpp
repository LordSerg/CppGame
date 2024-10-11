#include <iostream>

int main()
{
	game::LoadGame("path_to_game.mygame");
	game::getPlayer().selectUnit(5, 5);
	game::getPlayer().go(10, 10);
	while(!0)
	{
		game::Step();
	}
	
	game::SaveGame("path_to_game.mygame");
	/*
	game *g = new game("path_to_game.mygame");
	bool is_stop=false;
	g->addUnit(new peasant(5,5)); //create unit
	g->selectUnit(5,5); //choose unit
	g->build(new garden(8,8)); //make selected unit build a garden
	while(!is_stop)//main game loop
	{
		g->nextStep();
		g->show();
	}
	*/
}