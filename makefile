CC = gcc
CFLAGS = -Wall -pthread
DEP = settings.h

all: broker cliente

broker: controlador.c $(DEP)
	$(CC) -o controlador controlador.c $(CFLAGS)

cliente: cliente.c $(DEP)
	$(CC) -o cliente cliente.c $(CFLAGS)

clean:
	rm -f cliente controlador
	rm -f CONTROL_PIPE_*
	rm -f CLIENT_PIPE
