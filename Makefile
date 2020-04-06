TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

ringmaster: ringmaster.cpp potato.h server.h player.h
	g++ -g -o $@ $<

player: player.cpp potato.h server.h player.h
	g++ -g -o $@ $<