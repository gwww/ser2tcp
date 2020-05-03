#include "config.h"

extern int tcp_bridge_init(struct config *);
extern void tcp_bridge_cleanup();
extern void tcp_write(char *buffer, size_t length);
extern void tcp_send_control_command(char* command, char *data, uv_tcp_t *client);

#define TCP_CONTROL_COMMAND_PAUSE "PAUSE"
#define TCP_CONTROL_COMMAND_RESUME "RESUME"
#define TCP_CONTROL_COMMAND_HEARTBEAT "HEARTBEAT"
#define TCP_CONTROL_COMMAND_SERIAL_DISCONNECTED "SERIAL_DISCONNECTED"
#define TCP_CONTROL_COMMAND_SERIAL_CONNECTED "SERIAL_CONNECTED"
#define TCP_CONTROL_COMMAND_ANOTHER_TCP_CLIENT_CONNECTED "ANOTHER_TCP_CLIENT_IS_CONNECTED"