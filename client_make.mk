client: client.o client_functions.o
	gcc -g $^ -o $@ -lncursesw -pthread
client.o: client.c client.h client_functions.c
	gcc -g -c $< -o $@ -lncursesw -pthread
clean:
	rm *.o client