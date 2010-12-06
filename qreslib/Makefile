WARM	:= -Wall
INCLUDE := -isystem /lib/modules/`uname -r`/build/include
EXTRA_CFLAGS  := -O2 -g ${WARN} ${INCLUDE} -DQOS_DEBUG_LEVEL=8

all:
	${CC} ${EXTRA_CFLAGS} -c qos_debug.c
	${CC} ${EXTRA_CFLAGS} -c qres_lib.c
	${CC} ${EXTRA_CFLAGS} -c util_periodic.c
	${CC} ${EXTRA_CFLAGS} qos_debug.o qres_lib.o test-qres-beginend.c -o test-qres-beginend
	${CC} ${EXTRA_CFLAGS} qos_debug.o qres_lib.o test-qres-loop.c -o test-qres-loop
	${CC} ${EXTRA_CFLAGS} qos_debug.o qres_lib.o test-qres-app.c -o test-qres-app
	${CC} ${EXTRA_CFLAGS} qos_debug.o qres_lib.o util_periodic.o test-get-budget.c -o test-get-budget
clean:
	rm -rf *.o
