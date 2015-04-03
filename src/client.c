#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <assert.h>

#include "message.h"
#include "command.h"

#define RECV_BUFFER_SIZE 1000

static const char* msg_help = "Usage:\n"
"  destinations <from>\n"
"  find <from> <to>\n"
"  show <flightid>\n"
"  reserve <flightid> <numberofseats>\n"
"  cancel <flightid> <numberofseats>\n"
"  monitor <flightid> <time>\n"
"  help\n"
"  exit\n";

static const char *msg_input = "Please input next command:";

// declare functions
int makeargs(char *args, int *argc, char ***aa);

void dumpint(uint32_t n)
{
    uint32_t i;
    for (i = 1 << 31; i > 0; i = i / 2) {
        (n & i) ? printf("1"): printf("0");
	}
}

void dumpchar(unsigned char n)
{
    uint32_t i;
    for (i = 1 << 7; i > 0; i = i / 2) {
        (n & i) ? printf("1"): printf("0");
	}
}


static Message *request; 		// contains the message to send


int main(int argc, char **argv) {
	// addresses
	struct sockaddr_in addr_self, addr_remote;
	unsigned int addr_len = sizeof(addr_remote);
	memset((char *)&addr_self, 0, sizeof(addr_self));
	memset((char *)&addr_remote, 0, sizeof(addr_remote));

	int socket_fd; // socket file descriptor
	char inputbuffer[1000]; // buffer for reading user input
	char recvbuffer[RECV_BUFFER_SIZE]; // buffer for receiving messages
	char **myargv; // the command inputted by user as argument vector
	int myargc;    // argument count for the previous vector

	if (argc < 3) {
		printf("Usage: client <server ip> <server port>\n");
		return 2;
	}

	char* server = argv[1]; // server ip
	int port;				// server port
	if (!sscanf(argv[2], "%i", &port)) {
		perror("ERROR: Server port has to be an integer.\n");
		return 3;
	};

	printf("Welcome. Using server: %s port: %s\nConnecting...\n", argv[1], argv[2]);
	// create socket
	if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR: can't create UDP socket.\n");
		return 4;
	}

	// bind to every local ip address, pick any port
	addr_self.sin_family = AF_INET;
	addr_self.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_self.sin_port = htons(0);

	if (bind(socket_fd, (struct sockaddr *)&addr_self, sizeof(addr_self)) < 0) {
		perror("ERROR: binding local socket failed.\n");
		return 5;
	}

	// fill remote address struct
	addr_remote.sin_family = AF_INET;
	addr_remote.sin_port = htons(port);
	if (inet_aton(server, &addr_remote.sin_addr)==0) {
		perror("ERROR: converting server IP to binary address failed, please input proper IP.\n");
		return 6;
	}

	//// Command loop

	// exit with ^D, ^C, or typing "exit"
	while(true) {
		// output instruction for user
		puts(msg_input);
		// get the user input
		if(!fgets(inputbuffer, sizeof(inputbuffer), stdin)) {
			break;
		}
		// remove newline
		inputbuffer[strcspn(inputbuffer, "\n")] = 0;

		// parse arguments
		makeargs(inputbuffer, &myargc, &myargv);

		// find out which command
		command_t cmd = pick_cmd(myargv[0]);
		// create request message according to command
		cmd_result_t cmd_result = create_cmd_msg(cmd, myargc, myargv, &request);

		switch (cmd_result) {
			case MSG_CREATED: break;
			case HELP: puts(msg_help); continue;
			case ERROR: continue;
			case EXIT: return 0;
			default: perror("ERROR: internal control logic command not recognized, this should never happen.\n"); exit(7);
		}


		// construct packet from request message
		int packet_length;
		unsigned char *packet = message_to_packet(request, &packet_length);

		// debug: print out contents of packet being sent.
		packet_print(packet, packet_length);

		// Networking

		// send
		printf("Sending packet...\n");
		if (sendto(socket_fd, packet, packet_length, 0, (struct sockaddr *)&addr_remote, addr_len) == -1) {
			perror("ERROR: could not send packet via sendto");
			exit(7);
		}

		// receive
		printf("Waiting for reply...\n");
		int num_recv_bytes = recvfrom(socket_fd, recvbuffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&addr_remote, &addr_len);
		if(num_recv_bytes <= -1) {
			perror("ERROR: could not receive via recvfrom");
		}
    	else if (num_recv_bytes == 0) {
			printf("Received empty response.\n");
        }
		else {
			printf("Received a packet from server, probably as a response to something. TODO unmarshall and output reply.\n");
			// TODO: unmarshall from recvbuffer, output replies
		}

		// TODO: add failures and tolerance, retries and timeouts

		// free used memory
		packet_destroy(packet);
		message_destroy(request);
	}

	return 0;
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
