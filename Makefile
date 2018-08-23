libwhere_create.so: where_create.c stacktrace.c
	clang -O2 -Wall -fPIC -g -o $@ -shared $^ -ldl -lunwind -lpthread

.PHONY: clean
clean:
	-rm ./libwhere_create.so
