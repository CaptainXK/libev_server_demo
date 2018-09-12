.PONY:clean

LIB_FLAG := -levent

test.app:main.o
	gcc $< -o $@ $(LIB_FLAG)

main.o:server.c
	gcc -c $< -o $@

test:test.app
	$(EXEC) ./test.app

clean:
	rm *.o *.app
