//
// Created by root on 11/13/22.
//

#include <locale.h>
#include <stdio.h>
#include "server.h"
#include "server_socket.h"
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

time_t ticks;

struct server_game session;
//mutex for control server resource
pthread_mutex_t data_access_mtx;

//mutex for controlling that every thread execute only once per tick
pthread_mutex_t control_sync_mtx;
pthread_mutex_t ui_sync_mtx;
pthread_mutex_t player_send_sync_mtx[4];
pthread_mutex_t player_receive_sync_mtx[4];

int main() {

    ticks = 0;
    session.player_count = 0;
    session.beast_count = 0;
    session.end = 0;
    session.dropped_treasure_count = 0;


    session.PID = getpid();
    for (int i = 0; i < 4; i++) {
        session.players[i].connected = 0;
    }

    int rows = 0;
    int cols = 0;

    pthread_t input_t;
    pthread_t server_socket_connection_t;
    pthread_t tick_counter_t;

    pthread_mutex_init(&data_access_mtx, NULL);
    pthread_mutex_init(&control_sync_mtx, NULL);
    pthread_mutex_init(&ui_sync_mtx, NULL);

    for (int i = 0; i < 4; i++) {
        pthread_mutex_init(&player_receive_sync_mtx[i], NULL);
        pthread_mutex_init(&player_send_sync_mtx[i], NULL);
        pthread_mutex_init(&session.players[i].player_sync_mtx, NULL);
    }
    for (int i = 0; i < MAX_BEAST; i++) {
        pthread_mutex_init(&session.beasts[i].beast_sync_mtx, NULL);
    }


    srand(time(NULL));

    map_load_from_file(session.map, "map_1", max_row, max_col);

    int camp_found = 0;

    //find camp
    for (int i = 0; i < max_row; i++) {
        for (int y = 0; y < max_col; y++) {
            if (session.map[i][y] == 'A') {
                camp_found = 1;
                session.camp_fire_x = i;
                session.camp_fire_y = y;
                break;
            }
        }
        if (camp_found == 1) {
            break;
        }
    }

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, true);

    pthread_create(&tick_counter_t, NULL, game_tick_counter, NULL);
    pthread_create(&input_t, NULL, controls, NULL);
    pthread_create(&server_socket_connection_t, NULL, player_await, NULL);

    getmaxyx(stdscr, rows, cols);
    start_color();
    use_default_colors();

    do {
        pthread_mutex_lock(&ui_sync_mtx);
        pthread_mutex_lock(&data_access_mtx);
        print_ui(cols, rows);
        pthread_mutex_unlock(&data_access_mtx);
    } while (session.end == 0);

    endwin();

    pthread_mutex_destroy(&data_access_mtx);
    pthread_mutex_destroy(&control_sync_mtx);
    pthread_mutex_destroy(&ui_sync_mtx);

    for (int i = 0; i < 4; i++) {
        pthread_mutex_destroy(&player_receive_sync_mtx[i]);
        pthread_mutex_destroy(&player_send_sync_mtx[i]);
        pthread_mutex_destroy(&session.players[i].player_sync_mtx);
    }
    for (int i = 0; i < MAX_BEAST; i++) {
        pthread_mutex_destroy(&session.beasts[i].beast_sync_mtx);
    }

    return 0;
}