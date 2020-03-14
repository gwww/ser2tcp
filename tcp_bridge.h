#include "config.h"

extern int tcp_bridge_init(struct config *);
extern void tcp_bridge_cleanup();
extern void tcp_write(char *buffer, int length);
