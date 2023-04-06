//
// Created by root on 11/13/22.
//

#ifndef SF_GRA_V2_SERVER_H
#define SF_GRA_V2_SERVER_H

#include <sys/types.h>

#define max_row 25
#define max_col 51
#define MAX_BEAST 16

#define WALL 32
#define COIN 99
#define TREASURE 116
#define BIG_TREASURE 84
#define BEAST 42
#define CAMP 65
#define PATH 32
#define BUSH 35


enum direction_e {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};

enum value_e {
    COIN_E = 0,
    TREASURE_E = 1,
    LARGE_TREASURE_E = 2
};

struct player {
    int cord_x;
    int cord_y;
    int spawn_x;
    int spawn_y;
    int human;
    int deaths;
    int coin_carried;
    int coin_brought;
    char standing;
    int stunned;
    pid_t PID;
    int socket_id;
    int connected;
    pthread_mutex_t player_sync_mtx;
};

struct beast {
    int cord_x;
    int cord_y;
    int stunned;
    char standing;
    pthread_mutex_t beast_sync_mtx;
};

struct dropped_treasure {
    int cord_x;
    int cord_y;
    int value;
    int taken;
};

struct server_game {
    char map[max_row][max_col];
    int player_count;
    struct player players[4];
    struct beast beasts[MAX_BEAST];
    int beast_count;
    int dropped_treasure_count;
    struct dropped_treasure *dropped_treasures;
    int end;
    int camp_fire_x;
    int camp_fire_y;
    pid_t PID;
};

void *game_tick_counter(void *);

int map_load_from_file(char map[][max_col], const char *file, int max_x, int max_y);

void map_show(const char map[][max_col], int max_x, int max_y, int start_x, int start_y);

void info_show(int max_x, int max_y, int start_x, int start_y);

void print_ui(int cols, int rows);

void *controls(void *);

int beast_add();

void *beast_control_thread(void *);

int beast_move(char map[][max_col], struct beast *entity, enum direction_e);

int money_map_add(char map[][max_col], enum value_e);

int player_kill(int);

int look_for_players(int);


#endif //SF_GRA_V2_SERVER_H