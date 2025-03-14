

all:  bin/AntiVirus



bin/AntiVirus: obj/AntiVirus.o
	mkdir -p bin
	gcc -m32 -g -o bin/AntiVirus obj/AntiVirus.o



obj/AntiVirus.o: AntiVirus.c
	mkdir -p obj
	gcc -m32 -g -c -o obj/AntiVirus.o AntiVirus.c

clean:
	rm -rf obj bin

.PHONY: all clean
