GCCARGS = -Wall -pedantic -std=c11 -g
CC = clang $(GCCARGS)
MAIN_LIBS =

run: test.o vm
	-./vm $< ||:

test.o: test.asm asm ins.h
	./asm $< $@

bench: bench.o vm
	-./vm $< ||:

bench.o: bench.asm asm ins.h
	-./asm $< $@

asm: asm.o
	$(CC) $(MAIN_LIBS) -o $@ $^

asm.o: asm.c
	$(CC) -c -o $@ $<

vm: vm.o hashmap.o
	$(CC) $(MAIN_LIBS) -o $@ $^

vm.o: vm.c
	$(CC) -c -o $@ $<

hashmap.o: hashmap.c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.a asm vm
