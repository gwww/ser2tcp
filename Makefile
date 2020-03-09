all: ser2tcp

ser2tcp:  main.o tcp_bridge.o serial_bridge.o
	$(CC) $(CFLAGS) -luv -o ser2tcp main.o tcp_bridge.o serial_bridge.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

tcp_bridge.o: tcp_bridge.c
	$(CC) $(CFLAGS) -c tcp_bridge.c
#
serial_bridge.o: serial_bridge.c
	$(CC) $(CFLAGS) -c serial_bridge.c

clean: 
	$(RM) ser2tcp *.o *~
