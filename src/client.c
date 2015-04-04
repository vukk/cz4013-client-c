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
#include "marshall.h"
#include "current_utc_time.h"

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

// declare internal functions
bool receive_message(int *socket_fd, char *recvbuffer, struct sockaddr_in *addr_remote, socklen_t *addr_len);
int makeargs(char *args, int *argc, char ***aa);


static Message *request; 		// contains the message to send


int main(int argc, char **argv) {
	// addresses
	struct sockaddr_in addr_self, addr_remote;
	socklen_t addr_len = sizeof(addr_remote);
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
		fprintf(stderr, "ERROR: Server port has to be an integer.\n");
		return 3;
	};

	int ownport = 2222;
	if (argc == 4) {
		if (!sscanf(argv[3], "%i", &ownport)) {
			fprintf(stderr, "ERROR: Own port has to be an integer.\n");
			return 8;
		}
	}

	printf("Welcome. Using server: %s port: %s\nConnecting...\n", argv[1], argv[2]);
	// create socket
	if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR: can't create UDP socket");
		return 4;
	}

	// bind to every local ip address, pick any port
	addr_self.sin_family = AF_INET;
	addr_self.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_self.sin_port = htons(ownport);

	if (bind(socket_fd, (struct sockaddr *)&addr_self, sizeof(addr_self)) < 0) {
		perror("ERROR: binding local socket failed");
		return 5;
	}

	// fill remote address struct
	addr_remote.sin_family = AF_INET;
	addr_remote.sin_port = htons(port);
	if (inet_aton(server, &addr_remote.sin_addr)==0) {
		perror("ERROR: converting server IP to binary address failed, please input proper IP");
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
			default: fprintf(stderr, "ERROR: internal control logic command not recognized, this should never happen.\n"); exit(7);
		}


		// construct packet from request message
		int packet_length;
		unsigned char *packet = message_to_packet(request, &packet_length);

		// debug: print out contents of packet being sent.
		packet_print(packet, packet_length);

		// Networking

		// set starting time
		current_utc_time(&(request->start_ts));
		// send
		printf("Sending packet... to %u port %hu\n", addr_remote.sin_addr.s_addr, addr_remote.sin_port);
		if (sendto(socket_fd, packet, packet_length, 0, (struct sockaddr *)&addr_remote, addr_len) == -1) {
			perror("ERROR: could not send packet via sendto");
			exit(7);
		}

		struct timespec now;
		int errcount = 0;
		// receive
		while(true) {
			if(errcount > 20) {
				fprintf(stderr, "ERROR: receive error count exceeded maximum (20).\n");
				break;
			}
			if(!receive_message(&socket_fd, recvbuffer, &addr_remote, &addr_len)) {
				errcount++;
				continue;
			}
			current_utc_time(&now);
			// if monitoring period exceeded, stop monitoring
			if (now.tv_sec - (request->start_ts).tv_sec >= request->monitor_period)
				break;
		}

		// TODO: add failures and tolerance, retries and timeouts

		// free used memory
		packet_destroy(packet);
		message_destroy(request);
	}

	return 0;
}

bool receive_message(int *socket_fd, char *recvbuffer, struct sockaddr_in *addr_remote, socklen_t *addr_len) {
	// receive
	printf("Waiting for reply...\n");
	int num_recv_bytes = recvfrom(*socket_fd, recvbuffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)addr_remote, addr_len);
	if(num_recv_bytes <= -1) {
		perror("ERROR: could not receive via recvfrom");
		return false;
	}
	else if (num_recv_bytes == 0) {
		printf("Received empty response.\n");
		return false;
	}

	// else: we received a packet fine

	printf("Received a packet from server, probably as a response to something. TODO unmarshall and output reply.\n");
	// TODO: unmarshall from recvbuffer, output replies

	printf("\n");

	// Figure out response ID and type
	int32_t id;
	unsigned int type;

	for ( int i = 0; i < 5; i++ ) {
		if (i < 4) {
			id = id << 8;
			id = id | recvbuffer[i];
		}
		else if (i == 4) {
			type = recvbuffer[i];
		}
	}

	if (id != request->id || type != request->service) {
		printf("Got reply that didn't match our request. Ignoring it.\n");
		return false;
	}

	if (type == 1) {
		printf("Got reply for service: %d - not yet implemented.\n", type);
	}

	// Service type : 2
	if (type == 2) {

		int32_t seats;
		char year[5];
		char month[3];
		char day[3];
		char hour[3];
		char minute[3];

		char* ptr = &(recvbuffer[5]);

		memcpy(&seats, ptr, sizeof(int32_t));
		ptr += sizeof(int32_t);
		seats = ntohl(seats);

		unpack_str(&ptr, year, 4);
		unpack_str(&ptr, month, 2);
		unpack_str(&ptr, day, 2);
		unpack_str(&ptr, hour, 2);
		unpack_str(&ptr, minute, 2);

		float fare = unpack_float(ptr);

		printf("Message ID: %d \nType: %d \nSeats: %d \nFlight fare: %.2f \nDate: %s.%s.%s\nTime: %s:%s", id, type, seats, fare, day, month, year, hour, minute);
		printf("\n\n");

	}


	if (type == 3) {
		printf("Got reply for service: %d - not yet implemented.\n", type);
	}
	if (type == 4) {
		printf("Got reply for service: %d - not yet implemented.\n", type);
	}
	if (type == 5) {
		printf("Got reply for service: %d - not yet implemented.\n", type);
	}
	if (type == 6) {
		printf("Got reply for service: %d - not yet implemented.\n", type);
	}

	return true;
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
