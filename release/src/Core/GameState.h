#ifndef GAMESTATE_H
#define GAMESTATE_H

enum class GameState {
    MAIN_MENU,
    NEW_GAME_MENU,
    LOAD_GAME_MENU,
    PLAYING,
    PAUSED,
    WIN_SCREEN,
    LOSE_SCREEN,
    EXIT
};

struct CurrentGameState
{
    static GameState CGS;
};

#endif // GAMESTATE_H