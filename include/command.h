#ifndef COMMAND_H
#define COMMAND_H

#include "message.h"

typedef enum {
	CMD_DESTINATIONS, CMD_FIND, CMD_SHOW, CMD_RESERVE, CMD_CANCEL, CMD_MONITOR,
    CMD_EXIT, CMD_HELP
} command_t;

typedef enum {
    MSG_CREATED, HELP, EXIT, ERROR
} cmd_result_t;

cmd_result_t create_cmd_msg(command_t cmd, int myargc, char **myargv, Message **req);
command_t pick_cmd(char *cmdstr);

#endif

// EOF
