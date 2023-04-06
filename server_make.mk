server: server.o server_functions.o server_socket.o
	gcc -g $^ -o $@ -lncursesw -pthread
server.o: server.c server.h server_functions.c
	gcc -g -c $< -o $@ -lncursesw -pthread
server_socket.o: server_socket.c server_socket.h server.h
	gcc -g -c $< -o $@ -pthread
clean:
	rm *.o server