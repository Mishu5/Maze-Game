//
// Created by root on 10/22/22.
//
#include <locale.h>
#include "server.h"
#include "server_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

extern time_t ticks;

extern struct server_game session;

extern pthread_mutex_t data_access_mtx;

extern pthread_mutex_t control_sync_mtx;
extern pthread_mutex_t ui_sync_mtx;
extern pthread_mutex_t player_send_sync_mtx[4];
extern pthread_mutex_t player_receive_sync_mtx[4];

void *game_tick_counter(void *val) {

    while (session.end == 0) {
        //locking every thread
        pthread_mutex_lock(&data_access_mtx);

        pthread_mutex_unlock(&control_sync_mtx);

        for (int i = 0; i < session.beast_count; i++) {
            pthread_mutex_unlock(&session.beasts[i].beast_sync_mtx);
        }

        for (int i = 0; i < 4; i++) {
            pthread_mutex_unlock(&session.players[i].player_sync_mtx);
            pthread_mutex_unlock(&player_send_sync_mtx[i]);
            pthread_mutex_unlock(&player_receive_sync_mtx[i]);
        }
        //giving every thread access to move in tick
        pthread_mutex_unlock(&data_access_mtx);
        //unlocking every thread at once to prevent giving beast or player advantage
        usleep(1000000);
        //unlocking thread for printing current game tick status and data
        pthread_mutex_unlock(&ui_sync_mtx);
        ++ticks;
    }

    pthread_mutex_unlock(&control_sync_mtx);

    for (int i = 0; i < session.beast_count; i++) {
        pthread_mutex_unlock(&session.beasts[i].beast_sync_mtx);
    }

    for (int i = 0; i < 4; i++) {
        pthread_mutex_unlock(&session.players[i].player_sync_mtx);
        pthread_mutex_unlock(&player_send_sync_mtx[i]);
        pthread_mutex_unlock(&player_receive_sync_mtx[i]);
    }
    pthread_mutex_unlock(&ui_sync_mtx);
    pthread_mutex_unlock(&data_access_mtx);

    pthread_exit(NULL);
}

int map_load_from_file(char map[][max_col], const char *file, int max_x, int max_y) {
    if (map == NULL || file == NULL || max_x <= 0 || max_y <= 0) {
        return 1;
    }
    char *buffer = calloc(max_y + 1, sizeof(char));
    if (buffer == NULL) {
        return 2;
    }

    FILE *f = fopen(file, "rt");

    if (f == NULL) {
        free(buffer);
        return 3;
    }

    for (int i = 0; i < max_x; i++) {
        fscanf(f, "%s\n", buffer);
        strcpy(map[i], buffer);
        //printf("%s\n",map[i]);
    }
    free(buffer);
    fclose(f);


    return 0;
}

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
    printw("Server's PID: %lu\n", session.PID);
    move(start_y + 1, start_x);
    printw("Campsite X/Y: %d/%d\n", session.camp_fire_x, session.camp_fire_y);
    move(start_y + 2, start_x);
    printw("Round number: %lld\n", ticks);
    move(start_y + 4, start_x);
    printw("\n");
    printw("Parameters:\tPlayer1\tPlayer2\tPlayer3\tPlayer4\n");
    move(start_y + 5, start_x);
    printw("PID\t\t");
    for (int i = 0; i < 4; i++) {
        if (session.players[i].connected == 1) {
            printw("%lu\t", session.players[i].PID);
        } else {
            printw("-\t");
        }
    }
    move(start_y + 6, start_x);
    printw("\n");
    printw("Type\t\t");
    for (int i = 0; i < 4; i++) {
        if (session.players[i].connected == 1) {
            if (session.players[i].human == 1)
                printw("HUMAN\t");
            else
                printw("CPU\t");
        } else {
            printw("-\t");
        }
    }
    move(start_y + 7, start_x);
    printw("\n");
    printw("Curr X/Y\t");
    for (int i = 0; i < 4; i++) {
        if (session.players[i].connected == 1) {
            printw("%d/%d\t", session.players[i].cord_x, session.players[i].cord_y);
        } else {
            printw("--/--\t");
        }
    }
    move(start_y + 8, start_x);
    printw("\n");
    printw("Deaths\t\t");
    for (int i = 0; i < 4; i++) {
        if (session.players[i].connected == 1) {
            printw("%d\t", session.players[i].deaths);
        } else {
            printw("-\t");
        }
    }
    move(start_y + 9, start_x);
    printw("\n\n");
    printw("Coins\n");
    move(start_y + 10, start_x);
    printw("carried\t\t");
    for (int i = 0; i < 4; i++) {
        if (session.players[i].connected == 1) {
            printw("%d\t", session.players[i].coin_carried);
        } else {
            printw("-\t");
        }
    }
    printw("\n");
    move(start_y + 11, start_x);
    printw("brought\t\t");
    for (int i = 0; i < 4; i++) {
        if (session.players[i].connected == 1) {
            printw("%d\t", session.players[i].coin_brought);
        } else {
            printw("-\t");
        }
    }
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

void print_ui(int cols, int rows) {

    //clear();
    refresh();

    map_show(session.map, max_col, max_row, cols / 20, rows / 50);
    printw("\n");
    info_show(max_col, max_row, cols / 30 + 5, rows / 50 + 51);

    refresh();

    return;
}

void *controls(void *val) {

    pthread_t beasts_t[16];

    char choice;
    int beast_counter = 0;

    do {

        choice = getch();
        pthread_mutex_lock(&control_sync_mtx);

        pthread_mutex_lock(&data_access_mtx);

        switch (choice) {
            case 'Q':
            case 'q': {
                session.end = 1;
            }
                break;
            case 'B':
            case 'b': {
                if (!beast_add()) {
                    pthread_create(&beasts_t[beast_counter], NULL, beast_control_thread, NULL);
                    ++beast_counter;
                }
            }
                break;
            case 'c': {
                money_map_add(session.map, COIN_E);
            }
                break;
            case 't': {
                money_map_add(session.map, TREASURE_E);
            }
                break;
            case 'T': {
                money_map_add(session.map, LARGE_TREASURE_E);
            }
                break;
            default: {

            }
                break;
        }

        pthread_mutex_unlock(&data_access_mtx);

    } while (choice != 'q');

    for (int i = 0; i < beast_counter; i++) {
        pthread_join(beasts_t[i], NULL);
    }

    pthread_exit(NULL);
}

int beast_add() {
    if (session.beast_count >= 16) {
        return 1;
    }

    int place_flag = 0;
    int buffer_x;
    int buffer_y;

    while (place_flag == 0) {

        buffer_x = rand() % max_col;
        buffer_y = rand() % max_row;

        if (session.map[buffer_y][buffer_x] != 'w' && session.map[buffer_y][buffer_x] != '*') {
            place_flag = 1;
        } else {
            continue;
        }
        session.beasts[session.beast_count].standing = session.map[buffer_y][buffer_x];

    }

    session.beasts[session.beast_count].cord_x = buffer_x;
    session.beasts[session.beast_count].cord_y = buffer_y;
    session.beasts[session.beast_count].stunned = 0;
    session.map[buffer_y][buffer_x] = '*';
    session.beast_count++;

    return 0;
}

void *beast_control_thread(void *val) {

    struct beast *current_beast = &session.beasts[session.beast_count - 1];
    int current_best_id = session.beast_count - 1;
    int movement = 0;

    do {

        pthread_mutex_lock(&session.beasts[current_best_id].beast_sync_mtx);
        pthread_mutex_lock(&data_access_mtx);

        movement = look_for_players(current_best_id);

        beast_move(session.map, current_beast, movement);

        if (current_beast->standing >= '1' && current_beast->standing <= '4') {
            int error = player_kill(current_beast->standing - '0' - 1);
            if (error == 1) {
                pthread_mutex_unlock(&data_access_mtx);
                session.end = 1;
            }
            session.map[current_beast->cord_y][current_beast->cord_x] = '*';
            current_beast->standing = 'D';
        }

        pthread_mutex_unlock(&data_access_mtx);

    } while (session.end == 0);

    pthread_exit(NULL);
}

int beast_move(char map[][max_col], struct beast *entity, enum direction_e direction) {

    int beast_x = entity->cord_x;
    int beast_y = entity->cord_y;

    if (entity->stunned == 1) {
        entity->stunned = 0;
        return 1;
    }

    switch (direction) {

        case UP: {

            if (beast_y == 0) {

                return 1;
            }
            if (map[beast_y - 1][beast_x] == 'w' || map[beast_y - 1][beast_x] == '*') {

                return 1;
            }

            map[beast_y][beast_x] = entity->standing;
            entity->standing = map[beast_y - 1][beast_x];
            --entity->cord_y;
            map[beast_y - 1][beast_x] = '*';

        }
            break;

        case DOWN: {

            if (beast_y == max_row) {

                return 1;
            }
            if (map[beast_y + 1][beast_x] == 'w' || map[beast_y + 1][beast_x] == '*') {

                return 1;
            }

            map[beast_y][beast_x] = entity->standing;
            entity->standing = map[beast_y + 1][beast_x];
            ++entity->cord_y;
            map[beast_y + 1][beast_x] = '*';

        }
            break;

        case LEFT: {

            if (beast_x == 0) {

                return 1;
            }
            if (map[beast_y][beast_x - 1] == 'w' || map[beast_y][beast_x - 1] == '*') {

                return 1;
            }

            map[beast_y][beast_x] = entity->standing;
            entity->standing = map[beast_y][beast_x - 1];
            --entity->cord_x;
            map[beast_y][beast_x - 1] = '*';

        }
            break;

        case RIGHT: {

            if (beast_x == max_col) {

                return 1;
            }
            if (map[beast_y][beast_x + 1] == 'w' || map[beast_y][beast_x + 1] == '*') {

                return 1;
            }

            map[beast_y][beast_x] = entity->standing;
            entity->standing = map[beast_y][beast_x + 1];
            ++entity->cord_x;
            map[beast_y][beast_x + 1] = '*';

        }
            break;

        default: {

        }
            break;
    }
    if (entity->standing == '#') {
        entity->stunned = 1;
    }
    return 0;
}

int money_map_add(char map[][max_col], enum value_e val) {
    if (map == NULL) {
        return 1;
    }

    int buffer_x;
    int buffer_y;
    int place_flag = 0;

    while (place_flag == 0) {
        buffer_x = rand() % max_col;
        buffer_y = rand() % max_row;
        if (map[buffer_y][buffer_x] == '.') {
            place_flag = 1;
        }
    }

    switch (val) {
        case COIN_E: {
            map[buffer_y][buffer_x] = 'c';
        }
            break;
        case TREASURE_E: {
            map[buffer_y][buffer_x] = 't';
        }
            break;
        case LARGE_TREASURE_E: {
            map[buffer_y][buffer_x] = 'T';
        }
            break;
        default: {

        }
            break;
    }

    return 0;
}

int player_kill(int player_id) {

    int error = add_dropped_treasure(session.players[player_id].cord_x, session.players[player_id].cord_y, player_id);
    if (error == 1) {
        return 1;
    }
    session.players[player_id].cord_x = session.players[player_id].spawn_x;
    session.players[player_id].cord_y = session.players[player_id].spawn_y;
    session.players[player_id].coin_carried = 0;
    session.players[player_id].deaths += 1;
    session.players[player_id].standing = '.';
    session.map[session.players[player_id].cord_y][session.players[player_id].cord_x] = player_id + '0' + 1;

    return 0;
}

int look_for_players(int beast_id) {

    int beast_x = session.beasts[beast_id].cord_x;
    int beast_y = session.beasts[beast_id].cord_y;

    //search_up
    for (int y = 0; y < 2 && beast_y - y >= 0; y++) {
        if (session.map[beast_y - y][beast_x] == 'w') {
            break;
        }
        if (session.map[beast_y - y][beast_x] >= '1' && session.map[beast_y - y][beast_x] <= '4') {
            return UP;
        }
    }
    //search_down
    for (int y = 0; y < 2 && beast_y + y < max_row; y++) {
        if (session.map[beast_y + y][beast_x] == 'w') {
            break;
        }
        if (session.map[beast_y + y][beast_x] >= '1' && session.map[beast_y + y][beast_x] <= '4') {
            return DOWN;
        }
    }
    //search_left
    for (int x = 0; x < 2 && beast_x - x >= 0; x++) {
        if (session.map[beast_y][beast_x - x] == 'w') {
            break;
        }
        if (session.map[beast_y][beast_x - x] >= '1' && session.map[beast_y][beast_x - x] <= '4') {
            return LEFT;
        }
    }
    //search_right
    for (int x = 0; x < 2 && beast_x + x < max_col; x++) {
        if (session.map[beast_y][beast_x + x] == 'w') {
            break;
        }
        if (session.map[beast_y][beast_x + x] >= '1' && session.map[beast_y][beast_x + x] <= '4') {
            return RIGHT;
        }
    }
    //search_up_left
    for (int i = 0; i < 2; i++) {
        if (beast_y - i >= 0) {
            if (session.map[beast_y - i][beast_x] == 'w') {
                break;
            }
            if (session.map[beast_y - i][beast_x] >= '1' && session.map[beast_y - i][beast_x] <= '4') {
                return UP;
            }
            if (beast_x - i >= 0) {
                if (session.map[beast_y - i][beast_x - i] == 'w') {
                    break;
                }
                if (session.map[beast_y - i][beast_x - i] >= '1' && session.map[beast_y - i][beast_x - i] <= '4') {
                    return UP;
                }
            }
        }
    }
    //search_up_right
    for (int i = 0; i < 2; i++) {
        if (beast_y - i >= 0) {
            if (session.map[beast_y - i][beast_x] == 'w') {
                break;
            }
            if (session.map[beast_y - i][beast_x] >= '1' && session.map[beast_y - i][beast_x] <= '4') {
                return DOWN;
            }
            if (beast_x + i < max_col) {
                if (session.map[beast_y - i][beast_x + i] == 'w') {
                    break;
                }
                if (session.map[beast_y - i][beast_x + i] >= '1' && session.map[beast_y - i][beast_x + i] <= '4') {
                    return DOWN;
                }
            }
        }
    }
    //search_down_left
    for (int i = 0; i < 2; i++) {
        if (beast_y + i < max_row) {
            if (session.map[beast_y + i][beast_x] == 'w') {
                break;
            }
            if (session.map[beast_y + i][beast_x] >= '1' && session.map[beast_y + i][beast_x] <= '4') {
                return DOWN;
            }
            if (beast_x - i >= 0) {
                if (session.map[beast_y + i][beast_x - i] == 'w') {
                    break;
                }
                if (session.map[beast_y + i][beast_x - i] >= '1' && session.map[beast_y + i][beast_x - i] <= '4') {
                    return DOWN;
                }
            }
        }
    }
    //search_down_right
    for (int i = 0; i < 2; i++) {
        if (beast_y + i < max_row) {
            if (session.map[beast_y + i][beast_x] == 'w') {
                break;
            }
            if (session.map[beast_y + i][beast_x] >= '1' && session.map[beast_y + i][beast_x] <= '4') {
                return DOWN;
            }
            if (beast_x - i < max_col) {
                if (session.map[beast_y + i][beast_x + i] == 'w') {
                    break;
                }
                if (session.map[beast_y + i][beast_x + i] >= '1' && session.map[beast_y + i][beast_x + i] <= '4') {
                    return DOWN;
                }
            }
        }
    }
    //special_cases
    //case 1
    if (beast_y - 2 >= 0 && beast_x - 1 >= 0) {
        if ((session.map[beast_y - 2][beast_x - 1] >= '1' && session.map[beast_y - 2][beast_x - 1] <= '4') &&
            session.map[beast_y - 1][beast_x - 1] != 'w') {
            return UP;
        }
    }
    //case 2
    if (beast_y - 1 >= 0 && beast_x - 2 >= 0) {
        if ((session.map[beast_y - 1][beast_x - 2] >= '1' && session.map[beast_y - 1][beast_x - 2] <= '4') &&
            session.map[beast_y - 1][beast_x - 1] != 'w') {
            return LEFT;
        }
    }
    //case 3
    if (beast_y + 1 < max_row && beast_x - 2 >= 0) {
        if ((session.map[beast_y + 1][beast_x - 2] >= '1' && session.map[beast_y + 1][beast_x - 2] <= '4') &&
            session.map[beast_y + 1][beast_x - 1] != 'w') {
            return LEFT;
        }
    }
    //case 4
    if (beast_y + 2 < max_row && beast_x - 1 >= 0) {
        if ((session.map[beast_y + 2][beast_x - 1] >= '1' && session.map[beast_y + 2][beast_x - 1] <= '4') &&
            session.map[beast_y + 1][beast_x + 1] != 'w') {
            return DOWN;
        }
    }
    //case 5
    if (beast_y + 1 < max_row && beast_x + 2 < max_col) {
        if ((session.map[beast_y + 1][beast_x + 2] >= '1' && session.map[beast_y + 1][beast_x + 2] <= '4') &&
            session.map[beast_y + 1][beast_x + 1] != 'w') {
            return DOWN;
        }
    }
    //case 6
    if (beast_y + 2 < max_row && beast_x + 1 < max_col) {
        if ((session.map[beast_y + 2][beast_x + 1] >= '1' && session.map[beast_y + 2][beast_x + 1] <= '4') &&
            session.map[beast_y + 1][beast_x + 1] != 'w') {
            return RIGHT;
        }
    }
    //case 7
    if (beast_y - 1 >= 0 && beast_x + 2 < max_col) {
        if ((session.map[beast_y - 1][beast_x + 2] >= '1' && session.map[beast_y - 1][beast_x + 2] <= '4') &&
            session.map[beast_y - 1][beast_x + 1] != 'w') {
            return RIGHT;
        }
    }
    //case 8
    if (beast_y - 2 >= 0 && beast_x + 1 < max_col) {
        if ((session.map[beast_y - 2][beast_x + 1] >= '1' && session.map[beast_y - 2][beast_x + 1] <= '4') &&
            session.map[beast_y - 1][beast_x + 1] != 'w') {
            return UP;
        }
    }
    return (int) rand() % 4;
}
