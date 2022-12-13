CC := gcc
CCFLAGS := -Wall -pedantic 
INCLUDE := . -lncurses
SOURCES := ./obj/chase-game.o ./obj/chase-board.o ./obj/chase-sockets.o
#####################################################
all: server human bot

server: chase-server
human: chase-human-client
bot: chase-bot-client

chase-server: ./obj/chase-server.o $(SOURCES)
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)

chase-human-client: ./obj/chase-human-client.o $(SOURCES)
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)

chase-bot-client: ./obj/chase-bot-client.o $(SOURCES)
	$(CC) $(CCFLAGS) -o $@ $^ -I $(INCLUDE)
#--- Now every .c file can easily be found from VPATH !
./obj/%.o: %.c
	mkdir -p obj
	$(CC) $(CCFLAGS) -c $^ -o $@ -I $(INCLUDE)
clean:
	rm ./obj/*.o ./chase-server ./chase-human-client ./chase-bot-client
