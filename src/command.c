#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <string.h>
#include <netinet/in.h>

#include "command.h"
#include "marshall.h"


bool cmd_destinations(Message *msg, char *from);
bool cmd_find(Message *msg, char *from, char *to);
bool cmd_show(Message *msg, char *id);
bool cmd_reserve(Message *msg, char *id, char *num);
bool cmd_cancel(Message *msg, char *id, char *num);
bool cmd_monitor(Message *msg, char *id, char *time);


cmd_result_t create_cmd_msg(command_t cmd, int myargc, char **myargv, Message **req) {
	bool constructed = false;
	Message *msg = message_new();

	switch (cmd) {
		case CMD_DESTINATIONS:
			if (myargc != 2) return HELP;
			msg->service = 6;
			constructed = cmd_destinations(msg, myargv[1]);
			break;
		case CMD_FIND:
			if (myargc != 3) return HELP;
			msg->service = 1;
			constructed = cmd_find(msg, myargv[1], myargv[2]);
			break;
		case CMD_SHOW:
			if (myargc != 2) return HELP;
			msg->service = 2;
			constructed = cmd_show(msg, myargv[1]);
			break;
		case CMD_RESERVE:
			if (myargc != 3) return HELP;
			msg->service = 3;
			constructed = cmd_reserve(msg, myargv[1], myargv[2]);
			break;
		case CMD_CANCEL:
			if (myargc != 3) return HELP;
			msg->service = 5;
			constructed = cmd_cancel(msg, myargv[1], myargv[2]);
			break;
		case CMD_MONITOR:
			if (myargc != 3) return HELP;
			msg->service = 4;
			constructed = cmd_monitor(msg, myargv[1], myargv[2]);
			break;
		case CMD_EXIT: return EXIT;
		default: return HELP;
	}

	if (constructed) {
		*req = msg;
		return MSG_CREATED;
	}
	else {
		message_destroy(msg);
		return ERROR;
	}
}

command_t pick_cmd(char *cmdstr) {
	if (strcmp(cmdstr, "destinations") 	== 0) return CMD_DESTINATIONS;
	if (strcmp(cmdstr, "find") 			== 0) return CMD_FIND;
	if (strcmp(cmdstr, "show") 			== 0) return CMD_SHOW;
	if (strcmp(cmdstr, "reserve") 		== 0) return CMD_RESERVE;
	if (strcmp(cmdstr, "cancel") 		== 0) return CMD_CANCEL;
	if (strcmp(cmdstr, "monitor") 		== 0) return CMD_MONITOR;
	if (strcmp(cmdstr, "exit") 			== 0) return CMD_EXIT;
	return CMD_HELP;
}

// service 6
bool cmd_destinations(Message *msg, char *from) {
	int len_from = strlen(from);
	unsigned char *data;
	msg->length_data = sizeof(int32_t) + len_from;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_int32(ptr, len_from);
	ptr = pack_str(ptr, from, len_from);

	msg->data = data;
	return true;
}

// service 1
bool cmd_find(Message *msg, char *from, char *to) {
	int32_t len_from	= strlen(from);
	int32_t len_to 	= strlen(to);
	unsigned char *data;
	msg->length_data = sizeof(int32_t)*2 + len_from + len_to;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_int32(ptr, len_from);
	ptr = pack_str(ptr, from, len_from);
	ptr = pack_int32(ptr, len_to);
	ptr = pack_str(ptr, to, len_to);

	msg->data = data;
	return true;
}

// service 2
bool cmd_show(Message *msg, char *id) {
	int32_t flightid;
	if (!sscanf(id, "%i", &flightid)) {
		fprintf(stderr, "ERROR: Flight ID has to be an integer.\n");
		return false;
	};
	unsigned char *data;
	msg->length_data = sizeof(int32_t);
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_int32(ptr, flightid);

	msg->data = data;
	return true;
}

// service 3
bool cmd_reserve(Message *msg, char *id, char *num) {
	int32_t flightid;
	int32_t seats;
	if (!sscanf(id, "%i", &flightid)) {
		fprintf(stderr, "ERROR: Flight ID has to be an integer.\n");
		return false;
	};
	if (!sscanf(num, "%i", &seats)) {
		fprintf(stderr, "ERROR: Number of seats has to be an integer.\n");
		return false;
	};
	unsigned char *data;
	msg->length_data = sizeof(int32_t)*2;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_int32(ptr, flightid);
	ptr = pack_int32(ptr, seats);

	msg->data = data;
	return true;
}

// service 5
bool cmd_cancel(Message *msg, char *id, char *num) {
	int32_t flightid;
	int32_t seats;
	if (!sscanf(id, "%i", &flightid)) {
		fprintf(stderr, "ERROR: Flight ID has to be an integer.\n");
		return false;
	};
	if (!sscanf(num, "%i", &seats)) {
		fprintf(stderr, "ERROR: Number of seats has to be an integer.\n");
		return false;
	};
	unsigned char *data;
	msg->length_data = sizeof(int32_t)*2;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_int32(ptr, flightid);
	ptr = pack_int32(ptr, seats);

	msg->data = data;
	return true;
}

// service 4
bool cmd_monitor(Message *msg, char *id, char *time) {
	int32_t flightid;
	int32_t seconds;
	if (!sscanf(id, "%i", &flightid)) {
		fprintf(stderr, "ERROR: Flight ID has to be an integer.\n");
		return false;
	};
	if (!sscanf(time, "%i", &seconds)) {
		fprintf(stderr, "ERROR: Monitoring time has to be an integer (seconds).\n");
		return false;
	};
	unsigned char *data;
	msg->length_data = sizeof(int32_t)*2;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_int32(ptr, flightid);
	ptr = pack_int32(ptr, seconds);

	msg->data = data;
	return true;
}

// EOF
