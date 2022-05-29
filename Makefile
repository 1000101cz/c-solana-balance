CFLAGS += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs json-c)
LDLIBS = -lcurl


HW=prgsem
BINARIES=SolBalance


all: ${BINARIES}

OBJS=${patsubst %.c,%.o,${wildcard *.c}}

SolBalance: ${OBJS}
	gcc ${OBJS} ${LDFLAGS} ${LDLIBS} -o $@

${OBJS}: %.o: %.c
	gcc -c ${CFLAGS} $< -o $@

clean:
	rm -f ${BINARIES} ${OBJS}