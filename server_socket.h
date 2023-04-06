//
// Created by root on 11/13/22.
//

#ifndef SF_GRA_V2_SERVER_SOCKET_H
#define SF_GRA_V2_SERVER_SOCKET_H

#define PORT 21370

struct send {
    pid_t server_PID;
    time_t ticks;
    int player_number;
    int player_x;
    int player_y;
    int human;
    int deaths;
    int coin_found;
    int coin_brought;
    int game_end;
    int camp_x;
    int camp_y;
    char map[5][5];
};
struct receive {
    pid_t player_PID;
    char move_input;
};

struct player_data {
    int player_id;
    int socket_id;
};

void *player_await(void *);

void *player_control_thread(void *);

int player_create(pid_t, int);

void *player_send(void *);

void *player_receive(void *);

void map_send_prepare(int, char map[][5]);

void player_move(int, char);

int validate_movement(int, int, int);

void take_dropped_treasure(int);

int add_dropped_treasure(int, int, int);

#endif //SF_GRA_V2_SERVER_SOCKET_H
