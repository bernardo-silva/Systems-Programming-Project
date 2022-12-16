CC := gcc
CCFLAGS := -Wall -pedantic 
INCLUDE := src -lncurses 
SOURCES := ./obj/chase-game.o ./obj/chase-board.o ./obj/chase-sockets.o
#####################################################
all: server human bot

server: chase-server
human: chase-human-client
bot: chase-bot-client

chase-server: $(SOURCES) ./obj/chase-server.o
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)

chase-human-client: ./obj/chase-human-client.o $(SOURCES)
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)

chase-bot-client: ./obj/chase-bot-client.o $(SOURCES)
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)
#--- Now every .c file can easily be found from VPATH !
./obj/%.o: ./src/%.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
./obj/%.o: ./server/%.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
./obj/%.o: ./human-client/%.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
./obj/%.o: ./bot-client/%.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
clean:
	rm ./obj/*.o ./chase-server ./chase-human-client ./chase-bot-client
