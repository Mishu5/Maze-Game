//
// Created by root on 11/27/22.
//
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <locale.h>
#include "client.h"

extern pid_t server_PID;
extern time_t ticks;
extern time_t server_ticks;
extern int sock;
extern int end;
extern int rows;
extern int cols;
extern struct receive received;

extern pthread_mutex_t receiving_mtx;
extern pthread_mutex_t map_show_mtx;
extern pthread_mutex_t data_access_mtx;

void map_show(const char map[][max_col], int max_x, int max_y, int start_x, int start_y) {
    if (map == NULL || max_x <= 0 || max_y <= 0) {
        return;
    }

    init_pair(1, COLOR_BLACK, -1);
    init_pair(2, COLOR_BLACK, COLOR_YELLOW);
    init_pair(3, COLOR_RED, -1);
    init_pair(4, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(5, COLOR_WHITE, COLOR_GREEN);

    for (int x = 0; x < max_x; x++) {
        for (int y = 0; y < max_y; y++) {

            move(start_y + y, start_x + x);
            switch (map[y][x]) {

                case 'w': {
                    attron(A_REVERSE | COLOR_PAIR(1));
                    printw("%c", WALL);
                    attroff(A_REVERSE | COLOR_PAIR(1));
                }
                    break;
                case 'c': {
                    attron(COLOR_PAIR(2));
                    printw("%c", COIN);
                    attroff(COLOR_PAIR(2));
                }
                    break;
                case 't': {
                    attron(COLOR_PAIR(2));
                    printw("%c", TREASURE);
                    attroff(COLOR_PAIR(2));
                }
                    break;
                case 'T': {
                    attron(COLOR_PAIR(2));
                    printw("%c", BIG_TREASURE);
                    attroff(COLOR_PAIR(2));
                }
                    break;
                case '*': {
                    attron(COLOR_PAIR(3));
                    printw("%c", BEAST);
                    attroff(COLOR_PAIR(3));
                }
                    break;
                case '.': {
                    attron(COLOR_PAIR(1));
                    printw("%c", PATH);
                    attroff(COLOR_PAIR(1));
                }
                    break;
                case '#': {
                    attron(COLOR_PAIR(1));
                    printw("%c", BUSH);
                    attroff(COLOR_PAIR(1));
                }
                    break;
                case 'A': {
                    attron(COLOR_PAIR(5));
                    printw("%c", CAMP);
                    attroff(COLOR_PAIR(5));
                }
                    break;
                case '1':
                case '2':
                case '3':
                case '4': {
                    attron(COLOR_PAIR(4));
                    printw("%c", map[y][x]);
                    attroff(COLOR_PAIR(4));
                }
                    break;
                default: {

                }
                    break;
            }

        }
    }
    return;
}

void print_ui() {

    clear();
    refresh();

    map_show(received.map, max_col, max_row, cols / 20, rows / 50);
    printw("\n");
    info_show(max_col, max_row, cols / 30 + 5, rows / 50 + 51);

    refresh();

    return;
}

void info_show(int max_x, int max_y, int start_x, int start_y) {

    if (max_x <= 0 || max_y <= 0) {
        return;
    }

    init_pair(1, COLOR_BLACK, -1);
    init_pair(2, COLOR_BLACK, COLOR_YELLOW);
    init_pair(3, COLOR_RED, -1);
    init_pair(4, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(5, COLOR_WHITE, COLOR_GREEN);

    move(start_y, start_x);
    printw("Server's PID: %d\n", received.server_PID);
    move(start_y + 1, start_x);
    printw("Campsite X/Y:\t");
    if (received.camp_x == -1) {
        printw("unknown\n");
    } else {
        printw("%d/%d\n", received.camp_x, received.camp_y);
    }
    move(start_y + 2, start_x);
    printw("Round number:\t%d\n", received.ticks);
    move(start_y + 3, start_x);
    printw("\n");
    move(start_y + 4, start_x);
    printw("Player\n");
    move(start_y + 5, start_x);
    printw("Number:\t%d\n", received.player_number);
    move(start_y + 6, start_x);
    printw("Type:\t");
    if (received.human == 1) {
        printw("HUMAN\n");
    } else {
        printw("CPU\n");
    }
    move(start_y + 7, start_x);
    printw("Curr X/Y:\t%d/%d\n", received.player_x, received.player_y);
    move(start_y + 8, start_x);
    printw("Deaths:\t%d\n", received.deaths);
    move(start_y + 9, start_x);
    printw("\n");
    move(start_y + 10, start_x);
    printw("Coins found:\t%d\n", received.coin_found);
    move(start_y + 11, start_x);
    printw("Coins brought:\t%d\n", received.coin_brought);
    move(start_y + 12, start_x);

    printw("\n\n");
    printw("Legend: ");
    printw("\n");
    attron(COLOR_PAIR(4));
    printw("1234");
    attroff(COLOR_PAIR(4));
    printw("\t-players\n");
    attron(A_REVERSE | COLOR_PAIR(1));
    printw(" ");
    attroff(A_REVERSE | COLOR_PAIR(1));
    printw("\t-wall\n");
    attron(COLOR_PAIR(1));
    printw("#");
    attroff(COLOR_PAIR(1));
    printw("\t-bushes (slow down)\n");
    attron(COLOR_PAIR(3));
    printw("*");
    attroff(COLOR_PAIR(3));
    printw("\t-wild beast\n");
    attron(COLOR_PAIR(2));
    printw("c");
    attroff(COLOR_PAIR(2));
    printw("\t-one coin\n");
    attron(COLOR_PAIR(2));
    printw("t");
    attroff(COLOR_PAIR(2));
    printw("\t-treasure (10 coins)\n");
    attron(COLOR_PAIR(2));
    printw("T");
    attroff(COLOR_PAIR(2));
    printw("\t-large treasure (50 coins)\n");
    attron(COLOR_PAIR(5));
    printw("A");
    attroff(COLOR_PAIR(5));
    printw("\t-campsite\n");
    attron(COLOR_PAIR(2));
    printw("D");
    attroff(COLOR_PAIR(2));
    printw("\t-dropped treasure");

    return;
}

void *print_ui_thread(void *val) {

    while (end == 0) {
        pthread_mutex_lock(&map_show_mtx);
        pthread_mutex_lock(&data_access_mtx);
        print_ui();
        pthread_mutex_unlock(&data_access_mtx);
    }

    return NULL;
}

void *server_receiving_thread(void *val) {

    struct receive received_buffer;

    int val_read;

    while (end == 0) {

        val_read = read(sock, &received_buffer, sizeof(struct receive));

        if (val_read <= 0) {
            end = 1;
            break;
        }

        pthread_mutex_lock(&data_access_mtx);

        received.server_PID = received_buffer.server_PID;
        received.ticks = received_buffer.ticks;
        received.player_number = received_buffer.player_number;
        received.player_x = received_buffer.player_x;
        received.player_y = received_buffer.player_y;
        received.human = received_buffer.human;
        received.deaths = received_buffer.deaths;
        received.coin_found = received_buffer.coin_found;
        received.coin_brought = received_buffer.coin_brought;
        received.game_end = received_buffer.game_end;
        received.camp_x = received_buffer.camp_x;
        received.camp_y = received_buffer.camp_y;
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < 5; x++) {
                received.map[y][x] = received_buffer.map[y][x];
            }
        }
        if (received.game_end == 1) {
            end = 1;
        }
        pthread_mutex_unlock(&data_access_mtx);
        pthread_mutex_unlock(&map_show_mtx);

    }

    return NULL;
}

void send_to_server(struct send *to_send) {
    if (end == 0) {
        send(sock, to_send, sizeof(struct send), 0);
    }
    return;

}