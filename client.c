#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

static const char* msg_help = "Usage:\n"
"  destinations <from>\n"
"  find <from> <to>\n"
"  show <flightid>\n"
"  reserve <flightid> <numberofseats>\n"
"  cancel <flightid> <numberofseats>\n"
"  monitor <flightid> <time>\n"
"  help\n"
"  exit\n";

static const char* msg_input = "Please input next command:";

typedef enum {
	DESTINATIONS, FIND, SHOW, RESERVE, CANCEL, MONITOR, EXIT, HELP
} command_t;

// ugly
command_t pick_cmd (char* cmdstr) {
	if (strcmp(cmdstr, "destinations") 	== 0) return DESTINATIONS;
	if (strcmp(cmdstr, "find") 			== 0) return FIND;
	if (strcmp(cmdstr, "show") 			== 0) return SHOW;
	if (strcmp(cmdstr, "reserve") 		== 0) return RESERVE;
	if (strcmp(cmdstr, "cancel") 		== 0) return CANCEL;
	if (strcmp(cmdstr, "monitor") 		== 0) return MONITOR;
	if (strcmp(cmdstr, "exit") 			== 0) return EXIT;
	return HELP;
}

// declare functions
int makeargs(char *args, int *argc, char ***aa);
bool check_numargs(int should, int is);
void cmd_destinations(char *from);
void cmd_find(char *from, char *to);
void cmd_show(char *id);
void cmd_reserve(char *id, char *num);
void cmd_cancel(char *id, char *num);
void cmd_monitor(char *id, char *time);


int main(int argc, char **argv) {
	// addresses
	struct sockaddr_in addr_self, addr_remote;
	memset((char *)&addr_self, 0, sizeof(addr_self));
	memset((char *)&addr_remote, 0, sizeof(addr_remote));

	int socket_fd; // socket file descriptor
	char inputbuffer[1000]; // buffer for reading user input
	char **myargv; // the command inputted by user as argument vector
	int myargc;    // argument count for the previous vector

	if (argc < 3) {
		printf("Usage: client <server ip> <server port>\n");
		return 1;
	}

	char* server = argv[1]; // server ip
	int port;				// server port
	if (!sscanf(argv[2], "%i", &port)) {
		perror("ERROR: Server port has to be an integer.\n");
		return 2;
	};

	printf("Welcome. Using server: %s port: %s\nConnecting...\n", argv[1], argv[2]);
	// create socket
	if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("ERROR: can't create UDP socket.\n");
		return 3;
	}

	// bind to every local ip address, pick any port
	addr_self.sin_family = AF_INET;
	addr_self.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_self.sin_port = htons(0);

	if (bind(socket_fd, (struct sockaddr *)&addr_self, sizeof(addr_self)) < 0) {
		perror("ERROR: binding local socket failed.\n");
		return 4;
	}

	// fill remote address struct
	addr_remote.sin_family = AF_INET;
	addr_remote.sin_port = htons(port);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		perror("ERROR: converting server IP to binary address failed, please input proper IP.\n");
		return 5;
	}

	// TODO: construct messages to send, send/receive messages
	// TODO: add failures and tolerance

	//// Command loop

	puts(msg_input);

	// exit with ^D, ^C, or typing "exit"
	while(fgets(inputbuffer, sizeof(inputbuffer), stdin)) {
		// remove newline
		inputbuffer[strcspn(inputbuffer, "\n")] = 0;
		// parse arguments
		makeargs(inputbuffer, &myargc, &myargv);
		command_t cmd = pick_cmd(myargv[0]);

		//printf("myargc : %d input : '%s' enum : %d\n", myargc, myargv[0], cmd);

		switch (cmd) {
			case DESTINATIONS:
				if (!check_numargs(myargc, 2)) break;
				cmd_destinations(myargv[1]);
				break;
			case FIND:
				if (!check_numargs(myargc, 3)) break;
				cmd_find(myargv[1], myargv[2]);
				break;
			case SHOW:
				if (!check_numargs(myargc, 2)) break;
				cmd_show(myargv[1]);
				break;
			case RESERVE:
				if (!check_numargs(myargc, 3)) break;
				cmd_reserve(myargv[1], myargv[2]);
				break;
			case CANCEL:
				if (!check_numargs(myargc, 3)) break;
				cmd_cancel(myargv[1], myargv[2]);
				break;
			case MONITOR:
				if (!check_numargs(myargc, 3)) break;
				cmd_monitor(myargv[1], myargv[2]);
				break;
			case EXIT: break;
			default: puts(msg_help);
		}

		if (cmd == EXIT) break;

		puts(msg_input);
	}

	return 0;
}

bool check_numargs(int should, int is) {
	if (should != is) {
		puts(msg_help);
		return false;
	}
	return true;
}

void cmd_destinations(char *from) {

}

void cmd_find(char *from, char *to) {

}

void cmd_show(char *id) {

}

void cmd_reserve(char *id, char *num) {
	int seats;
	if (!sscanf(num, "%i", &seats)) {
		perror("ERROR: Number of seats has to be an integer.\n");
		return;
	};
}

void cmd_cancel(char *id, char *num) {
	int seats;
	if (!sscanf(num, "%i", &seats)) {
		perror("ERROR: Number of seats has to be an integer.\n");
		return;
	};
}

void cmd_monitor(char *id, char *time) {
	int seconds;
	if (!sscanf(time, "%i", &seconds)) {
		perror("ERROR: Monitoring time has to be an integer (seconds).\n");
		return;
	};
}


// the following utility function is from
// http://stackoverflow.com/questions/1706551/parse-string-into-argv-argc
int makeargs(char *args, int *argc, char ***aa) {
    char *buf = strdup(args);
    int c = 1;
    char *delim;
    char **argv = calloc(c, sizeof (char *));

    argv[0] = buf;

    while ((delim = strchr(argv[c - 1], ' '))) {
        argv = realloc(argv, (c + 1) * sizeof (char *));
        argv[c] = delim + 1;
        *delim = 0x00;
        c++;
    }

    *argc = c;
    *aa = argv;

    return c;
}

// EOF
