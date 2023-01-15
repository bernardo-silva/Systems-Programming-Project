CC := gcc
CCFLAGS := -Wall -pedantic -g
INCLUDE := src -lncursesw -pthread
SOURCES := ./obj/chase-game.o ./obj/chase-board.o ./obj/chase-sockets.o ./obj/chase-threads.o
SERVER_FUNCS := ./obj/chase-server.o
#####################################################
all: server human

server: chase-server
human: chase-human-client

chase-server: $(SOURCES) $(SERVER_FUNCS) ./obj/chase-server-main.o
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)

chase-human-client: ./obj/chase-human-client.o $(SOURCES)
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)

./obj/%.o: ./src/%.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
./obj/%.o: ./server/%.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
./obj/%.o: ./human-client/%.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
clean:
	rm ./obj/*.o ./chase-server ./chase-human-client ./obj -r
