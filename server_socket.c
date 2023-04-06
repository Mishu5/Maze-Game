
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server_socket.h"
#include "server.h"

extern time_t ticks;

extern struct server_game session;
extern pthread_mutex_t data_access_mtx;
extern pthread_mutex_t control_sync_mtx;
extern pthread_mutex_t ui_sync_mtx;

extern pthread_mutex_t player_send_sync_mtx[4];
extern pthread_mutex_t player_receive_sync_mtx[4];

void *player_await(void *val) {

    int server_fd;
    int player_socket;
    int val_read;
    struct sockaddr_in address;
    int opt = 1;
    int addr_len = sizeof(address);
    struct receive received_package;
    int position;

    pthread_t player_t[4];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *) &address, sizeof(address));

    listen(server_fd, 4);
    while (session.end != 1) {

        player_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addr_len);

        val_read = read(player_socket, &received_package, sizeof(struct receive));

        if (session.player_count >= 4) {
            close(player_socket);
            continue;
        }
        pthread_mutex_lock(&data_access_mtx);
        position = player_create(received_package.player_PID, player_socket);

        pthread_create(&player_t[position], NULL, player_control_thread, (void *) &position);
        pthread_mutex_unlock(&data_access_mtx);

    }
    shutdown(server_fd, SHUT_RDWR);
    return NULL;
}

void *player_control_thread(void *val) {

    int player_id = *(int *) val;
    struct player *current_player = &session.players[player_id];
    struct player_data p_data = {player_id, current_player->socket_id};

    pthread_t send_t, receive_t;

    pthread_create(&send_t, NULL, player_send, (void *) &p_data);
    pthread_create(&receive_t, NULL, player_receive, (void *) &p_data);

    while (session.end == 0 && session.players[player_id].connected == 1) {
        pthread_mutex_lock(&session.players[player_id].player_sync_mtx);

    }

    pthread_mutex_lock(&data_access_mtx);
    session.map[session.players[player_id].cord_y][session.players[player_id].cord_x] = session.players[player_id].standing;
    session.player_count--;
    session.players[player_id].connected = 0;
    pthread_mutex_unlock(&data_access_mtx);

    close(current_player->socket_id);

    return NULL;
}

int player_create(pid_t PID, int socket_id) {

    int position;

    for (position = 0; position < 4; position++) {
        if (session.players[position].connected == 0) {
            session.players[position].connected = 1;
            break;
        }
    }

    int place_flag = 0;
    int buffer_x;
    int buffer_y;

    while (place_flag == 0) {

        buffer_x = rand() % max_col;
        buffer_y = rand() % max_row;

        if (session.map[buffer_y][buffer_x] != 'w' && session.map[buffer_y][buffer_x] != '*' &&
            session.map[buffer_y][buffer_x] != 'c' && session.map[buffer_y][buffer_x] != 't' &&
            session.map[buffer_y][buffer_x] != 'T' && session.map[buffer_y][buffer_x] != 'D' &&
            session.map[buffer_y][buffer_x] != '#' && session.map[buffer_y][buffer_x] != '1' &&
            session.map[buffer_y][buffer_x] != '2' && session.map[buffer_y][buffer_x] != '3' &&
            session.map[buffer_y][buffer_x] != '4' && session.map[buffer_y][buffer_x] != 'A') {
            place_flag = 1;
        } else {
            continue;
        }

    }

    session.players[position].standing = session.map[buffer_y][buffer_x];
    session.players[position].cord_x = buffer_x;
    session.players[position].cord_y = buffer_y;
    session.players[position].spawn_x = buffer_x;
    session.players[position].spawn_y = buffer_y;
    session.players[position].human = 1;
    session.players[position].stunned = 0;
    session.players[position].deaths = 0;
    session.players[position].coin_carried = 0;
    session.players[position].coin_brought = 0;
    session.players[position].PID = PID;
    session.players[position].socket_id = socket_id;
    session.map[buffer_y][buffer_x] = position + '0' + 1;

    return position;
}

void *player_send(void *val) {

    struct player_data p_data = *(struct player_data *) val;
    struct send sending_package;

    while (session.end == 0) {

        pthread_mutex_lock(&player_send_sync_mtx[p_data.player_id]);
        pthread_mutex_lock(&data_access_mtx);

        map_send_prepare(p_data.player_id, sending_package.map);
        sending_package.server_PID = session.PID;
        sending_package.ticks = ticks;
        sending_package.player_number = p_data.player_id + 1;
        sending_package.player_x = session.players[p_data.player_id].cord_x;
        sending_package.player_y = session.players[p_data.player_id].cord_y;
        sending_package.human = session.players[p_data.player_id].human;
        sending_package.deaths = session.players[p_data.player_id].deaths;
        sending_package.coin_found = session.players[p_data.player_id].coin_carried;
        sending_package.coin_brought = session.players[p_data.player_id].coin_brought;

        pthread_mutex_unlock(&data_access_mtx);

        if (session.players[p_data.player_id].connected == 1) {
            send(session.players[p_data.player_id].socket_id, &sending_package, sizeof(struct send), 0);
        } else {
            break;
        }

    }

    return NULL;
}

void *player_receive(void *val) {

    struct player_data p_data = *(struct player_data *) val;
    struct receive received_package;
    int val_read;

    while (session.end == 0) {

        pthread_mutex_lock(&player_receive_sync_mtx[p_data.player_id]);
        val_read = read(p_data.socket_id, &received_package, sizeof(struct receive));

        if (val_read <= 0) {
            session.players[p_data.player_id].connected = 0;
            break;
        }
        pthread_mutex_lock(&data_access_mtx);
        switch (received_package.move_input) {

            case 'W':
            case 'w':
            case 'A':
            case 'a':
            case 'S':
            case 's':
            case 'D':
            case 'd': {
                player_move(p_data.player_id, received_package.move_input);
            }
                break;
            case 'Q':
            case 'q': {

                session.map[session.players[p_data.player_id].cord_y][session.players[p_data.player_id].cord_x] = session.players[p_data.player_id].standing;
                session.player_count--;
                session.players[p_data.player_id].connected = 0;
            }
                break;
            default: {

            }
                break;

        }
        pthread_mutex_unlock(&data_access_mtx);

    }

    return NULL;
}

void map_send_prepare(int position, char map[][5]) {

    if (session.players[position].connected == 0) {
        return;
    }

    int player_x;
    int player_y;

    player_x = session.players[position].cord_x;
    player_y = session.players[position].cord_y;

    int from_x, from_y;
    int to_x, to_y;

    from_x = player_x - 2;
    from_y = player_y - 2;

    if (from_x < 0) from_x = 0;
    if (from_y < 0) from_y = 0;

    for (int y = 0; y < 5 && from_y < max_row; y++) {
        from_x = player_x - 2;
        if (from_x < 0) from_x = 0;
        for (int x = 0; x < 5 && from_x < max_col; x++) {
            map[y][x] = session.map[from_y][from_x];
            from_x++;
        }
        from_y++;
    }

    return;
}

void player_move(int player_id, char direction) {

    int validation;

    struct player *current_player = &session.players[player_id];

    switch (direction) {
        case 'W':
        case 'w': {

            validation = validate_movement(0, -1, player_id);
            if (validation) {
                session.map[current_player->cord_y][current_player->cord_x] = current_player->standing;
                current_player->cord_y--;
                current_player->standing = session.map[current_player->cord_y][current_player->cord_x];
                session.map[current_player->cord_y][current_player->cord_x] = player_id + '0' + 1;

                switch (current_player->standing) {
                    case 'c': {
                        current_player->coin_carried += 1;
                        current_player->standing = '.';
                    }
                        break;
                    case 't': {
                        current_player->coin_carried += 10;
                        current_player->standing = '.';
                    }
                        break;
                    case 'T' : {
                        current_player->coin_carried += 50;
                        current_player->standing = '.';
                    }
                    case 'A': {
                        current_player->coin_brought += current_player->coin_carried;
                        current_player->standing = 'A';
                    }
                    case 'D': {
                        take_dropped_treasure(player_id);
                        current_player->standing = '.';
                    }
                    default: {

                    }
                        break;
                }

            }

        }
            break;
        case 'S':
        case 's': {
            validation = validate_movement(0, 1, player_id);
            if (validation) {
                session.map[current_player->cord_y][current_player->cord_x] = current_player->standing;
                current_player->cord_y++;
                current_player->standing = session.map[current_player->cord_y][current_player->cord_x];
                session.map[current_player->cord_y][current_player->cord_x] = player_id + '0' + 1;

                switch (current_player->standing) {
                    case 'c': {
                        current_player->coin_carried += 1;
                        current_player->standing = '.';
                    }
                        break;
                    case 't': {
                        current_player->coin_carried += 10;
                        current_player->standing = '.';
                    }
                        break;
                    case 'T' : {
                        current_player->coin_carried += 50;
                        current_player->standing = '.';
                    }
                    case 'A': {
                        current_player->coin_brought += current_player->coin_carried;
                    }
                    case 'D': {
                        take_dropped_treasure(player_id);
                        current_player->standing = '.';
                    }
                    default: {

                    }
                        break;
                }

            }
        }
            break;
        case 'A':
        case 'a': {
            validation = validate_movement(-1, 0, player_id);
            if (validation) {
                session.map[current_player->cord_y][current_player->cord_x] = current_player->standing;
                current_player->cord_x--;
                current_player->standing = session.map[current_player->cord_y][current_player->cord_x];
                session.map[current_player->cord_y][current_player->cord_x] = player_id + '0' + 1;

                switch (current_player->standing) {
                    case 'c': {
                        current_player->coin_carried += 1;
                        current_player->standing = '.';
                    }
                        break;
                    case 't': {
                        current_player->coin_carried += 10;
                        current_player->standing = '.';
                    }
                        break;
                    case 'T' : {
                        current_player->coin_carried += 50;
                        current_player->standing = '.';
                    }
                    case 'A': {
                        current_player->coin_brought += current_player->coin_carried;
                    }
                    case 'D': {
                        take_dropped_treasure(player_id);
                        current_player->standing = '.';
                    }
                    default: {

                    }
                        break;
                }

            }
        }
            break;
        case 'D':
        case 'd': {
            validation = validate_movement(1, 0, player_id);
            if (validation) {
                session.map[current_player->cord_y][current_player->cord_x] = current_player->standing;
                current_player->cord_x++;
                current_player->standing = session.map[current_player->cord_y][current_player->cord_x];
                session.map[current_player->cord_y][current_player->cord_x] = player_id + '0' + 1;

                switch (current_player->standing) {
                    case 'c': {
                        current_player->coin_carried += 1;
                        current_player->standing = '.';
                    }
                        break;
                    case 't': {
                        current_player->coin_carried += 10;
                        current_player->standing = '.';
                    }
                        break;
                    case 'T' : {
                        current_player->coin_carried += 50;
                        current_player->standing = '.';
                    }
                    case 'A': {
                        current_player->coin_brought += current_player->coin_carried;
                    }
                    case 'D': {
                        take_dropped_treasure(player_id);
                        current_player->standing = '.';
                    }
                    default: {

                    }
                        break;
                }

            }
        }
            break;

        default: {

        }
            break;
    }

}

int validate_movement(int offset_x, int offset_y, int player_id) {

    if (session.players[player_id].stunned == 1) {
        session.players[player_id].stunned = 0;
        return 0;
    }

    int cur_x = session.players[player_id].cord_x;
    int cur_y = session.players[player_id].cord_y;

    cur_x += offset_x;
    cur_y += offset_y;

    if (session.map[cur_y][cur_x] != '1' && session.map[cur_y][cur_x] != '2' && session.map[cur_y][cur_x] != '3' &&
        session.map[cur_y][cur_x] != '4' && session.map[cur_y][cur_x] != 'w' && session.map[cur_y][cur_x] != '*') {
        if (session.map[cur_y][cur_x] == '#') {
            session.players[player_id].stunned = 1;
        }
        return 1;
    }

    return 0;
}

void take_dropped_treasure(int player_id) {

    int player_x = session.players[player_id].cord_x;
    int player_y = session.players[player_id].cord_y;

    for (int i = 0; i < session.dropped_treasure_count; i++) {
        if (player_x == session.dropped_treasures[i].cord_x && player_y == session.dropped_treasures[i].cord_y &&
            session.dropped_treasures[i].taken == 0) {
            session.players[player_id].coin_carried += session.dropped_treasures[i].value;
            session.dropped_treasures[i].taken = 1;
            break;
        }
    }

}

int add_dropped_treasure(int cur_x, int cur_y, int player_id) {

    struct dropped_treasure *temp = realloc(session.dropped_treasures,
                                            sizeof(struct dropped_treasure) * (session.dropped_treasure_count + 1));

    if (temp == NULL) {
        return 1;
    }

    session.dropped_treasures = temp;
    session.dropped_treasures[session.dropped_treasure_count].cord_x = cur_x;
    session.dropped_treasures[session.dropped_treasure_count].cord_x = cur_y;
    session.dropped_treasures[session.dropped_treasure_count].value = session.players[player_id].coin_carried;
    session.dropped_treasures[session.dropped_treasure_count].taken = 0;
    session.dropped_treasure_count++;

    return 0;
}