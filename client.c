#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <locale.h>
#include "client.h"

pid_t server_PID;
time_t ticks;
time_t server_ticks;
int sock;
int end;
int rows;
int cols;
struct receive received;

pthread_mutex_t input_mtx;
pthread_mutex_t receiving_mtx;
pthread_mutex_t map_show_mtx;
pthread_mutex_t data_access_mtx;

int main() {

    end = 0;
    rows = 0;
    cols = 0;

    pthread_mutex_init(&input_mtx, NULL);
    pthread_mutex_init(&receiving_mtx, NULL);
    pthread_mutex_init(&map_show_mtx, NULL);
    pthread_mutex_init(&data_access_mtx, NULL);

    pthread_t ui_show_t;
    pthread_t server_receive_t;

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, true);
    timeout(5000);
    getmaxyx(stdscr, rows, cols);
    start_color();
    use_default_colors();

    sock = 0;
    int client_fd;
    char buffer;
    struct sockaddr_in serv_addr;
    struct send package;
    package.player_PID = getpid();
    package.move_input = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if ((client_fd = connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        endwin();
        printf("Connection Failed");
        getchar();
        pthread_mutex_destroy(&input_mtx);
        pthread_mutex_destroy(&receiving_mtx);
        pthread_mutex_destroy(&map_show_mtx);

        return -1;
    }

    send(sock, &package, sizeof(package), 0);

    pthread_create(&ui_show_t, NULL, print_ui_thread, NULL);
    pthread_create(&server_receive_t, NULL, server_receiving_thread, NULL);

    while (end == 0) {

        buffer = 0;
        buffer = getch();
        if(end==1)break;
        package.move_input = buffer;
        send_to_server(&package);
        if (buffer == 'q') {
            end = 1;
        }

    }

    endwin();

    close(client_fd);

    pthread_mutex_destroy(&input_mtx);
    pthread_mutex_destroy(&receiving_mtx);
    pthread_mutex_destroy(&map_show_mtx);
    pthread_mutex_destroy(&data_access_mtx);

    printf("\nDISCONNECTED\n");

    getchar();

    return 0;
}