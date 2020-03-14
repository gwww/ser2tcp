src = $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS = -g -O0 -Wall -Werror
LDFLAGS = -luv
myprog = ser2tcp

$(myprog): $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) -f $(obj) $(myprog)
