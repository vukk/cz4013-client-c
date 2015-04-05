#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <assert.h>

#include <poll.h>

#include "message.h"
#include "command.h"
#include "marshall.h"
#include "current_utc_time.h"

#define RECV_BUFFER_SIZE 1000
#define RATE_OF_CHAOS 0.15
#define MAX_ERROR 20
#define MAX_RETRY 5

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
void send_message(int *socket_fd, unsigned char *packet, int packet_length, struct sockaddr_in *addr_remote, socklen_t *addr_len, char *server, int port);
bool receive_message(int timeout, bool *again, struct pollfd *poll_fd, int *socket_fd, char *recvbuffer, struct sockaddr_in *addr_remote, socklen_t *addr_len);
int makeargs(char *args, int *argc, char ***aa);


static Message *request; // contains the message to send


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

	struct pollfd poll_fd;
	int poll_res;

	int timeout = 100; // milliseconds

	// seed random
	srand(time(NULL));

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

	// setup poll
	poll_fd.fd = socket_fd;
	poll_fd.events = POLLIN;

	// empty udp packet queue just in case
	timeout = 50;
	while(true) {
		poll_res = poll(&poll_fd, 1, timeout);
		if(poll_res == -1) {
			perror("ERROR: problem while polling socket.");
		}
		else if(poll_res == 0) {
			// timeout
			break;
		}
		else {
			if(recvfrom(socket_fd, recvbuffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&addr_remote, &addr_len) <= 0)
				break;
		}
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

		send_message(&socket_fd, packet, packet_length, &addr_remote, &addr_len, server, port);

		// set timeout appropriately
		if(request->service == 4) // monitor
			timeout = 50;
		else
			timeout = 5000;

		struct timespec now;
		int errcount = 0;
		int retrycount = 0;
		bool again = false;
		// receive
		while(true) {
			if(errcount > MAX_ERROR) {
				fprintf(stderr, "ERROR: receive error count exceeded maximum (20).\n");
				break;
			}

			if(retrycount > MAX_RETRY) {
				printf("Failed to receive a reply.\n");
				break;
			}

			if(!receive_message(timeout, &again, &poll_fd, &socket_fd, recvbuffer, &addr_remote, &addr_len)) {
				errcount++;
				continue;
			}

			// TODO: monitoring has to have different handling, what if the monitoring message is dropped?

			// if not monitoring, and socket read times out (again == true)
			if(request->service != 4 && again) {
				retrycount++;
				printf("Retrying...\n"); // TODO think about whether send_message should on retry set start time again?
				send_message(&socket_fd, packet, packet_length, &addr_remote, &addr_len, server, port);
				continue;
			}

			current_utc_time(&now);
			// if monitoring period exceeded, stop monitoring
			if (now.tv_sec - (request->start_ts).tv_sec >= request->monitor_period) {
				if(request->service == 4) // monitor
					printf("Timed out.\n");
				break;
			}
		}

		// free used memory
		packet_destroy(packet);
		message_destroy(request);
	}

	return 0;
}

bool absolute_chaos() {
    if ((double)rand() / (double)RAND_MAX < RATE_OF_CHAOS)
		return true;
	return false;
}

void send_message(int *socket_fd, unsigned char *packet, int packet_length, struct sockaddr_in *addr_remote, socklen_t *addr_len, char *server, int port) {
	// set starting time
	current_utc_time(&(request->start_ts));
	// send
	printf("Sending request id: %d addr: %s port: %d\n", request->id, server, port);
	if (sendto(*socket_fd, packet, packet_length, 0, (struct sockaddr *)addr_remote, *addr_len) == -1) {
		perror("ERROR: could not send packet via sendto");
		exit(7);
	}
}

bool receive_message(int timeout, bool *again, struct pollfd *poll_fd, int *socket_fd, char *recvbuffer, struct sockaddr_in *addr_remote, socklen_t *addr_len) {
	// receive
	if(!*again)
		printf("Waiting for reply...\n");
	*again = false;

	int poll_res;

	poll_res = poll(poll_fd, 1, timeout);
	if(poll_res == -1) {
		perror("ERROR: problem while polling socket.");
		return false;
	}
	else if(poll_res == 0) {
		// timeout
		*again = true;
		return true;
	}

	// else

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

	// implement packet dropping
	if (absolute_chaos()) {
		if (request->service == 4)
			*again = true;
		return false;
	}

	// Figure out response ID and type
	int32_t id;
	unsigned int type;
	char* ptr = recvbuffer;

	id = unpack_int32(&ptr);
	type = *ptr;
	ptr++;

	if (id != request->id || type != request->service) {
		printf("Got reply that didn't match our request id req: %d repl: %d service req: %d repl: %d. Ignoring it.\n", request->id, id, request->service, type);
		return false;
	}

	// Service 1 ::
	// find <source> <destination>
	if (type == 1) {

		int32_t amount = unpack_int32(&ptr);
		int32_t flights[amount];

		if 	    (amount ==  0) printf("No Route found flying from source to destination.\n");
		else if (amount == -1) printf("Source airport not recognised.\n");
		else if (amount == -2) printf("Destination airport not recognised.\n");
		else if (amount == -3) printf("Both source and destination airports not recognised.\n");
		else if (amount <  -3) printf("Impossible error occurred.");
		else {
			printf("Flights : %d\n", amount);
			for (int i = 0; i < amount; i++ ) {
				flights[i] = unpack_int32(&ptr);
				printf(" - ID: %d\n", flights[i]);
			}
			printf("\n");
		}
	}

	// Service  2 ::
	// show <id>
	if (type == 2) {

		int32_t seats;
		seats = unpack_int32(&ptr);

		if (seats < 0)
			printf("Requested flight not found.\n");
		else {
			char year[5];
			char month[3];
			char day[3];
			char hour[3];
			char minute[3];

			unpack_str(&ptr, year, 4);
			unpack_str(&ptr, month, 2);
			unpack_str(&ptr, day, 2);
			unpack_str(&ptr, hour, 2);
			unpack_str(&ptr, minute, 2);

			float fare = unpack_float(&ptr);
			printf("Message ID: %d \nType: %d \nSeats: %d \nFlight fare: %.2f \nDate: %s.%s.%s\nTime: %s:%s", id, type, seats, fare, day, month, year, hour, minute);
			printf("\n\n");
		}
	}

	// Service  3 ::
	// reserve <id> <tickets>
	if (type == 3) {

		int32_t reserved  = unpack_int32(&ptr);

		if (reserved > 0)
			printf("Congratulations! You reserved %d tickets for the flight.\n", reserved);
		else if (reserved == 0)
			printf("Sorry! This flight is sold out.\n");
		else
			printf("Requested flight not found.\n");
	}

	// Service 4 ::
	// monitor <flight id> <time>
	if (type == 4) {

		int32_t availability = unpack_int32(&ptr);

		if (availability >= 0)
			printf("Update: There are %d seats available on the flight.\n", availability);
		else
			printf("Requested flight not found.\n");
	}

	// Service 5 ::
	// cancel <flight id> <seats>
	if (type == 5) {

		int32_t cancelled = unpack_int32(&ptr);
		if (cancelled > 0)
			printf("You cancelled %d tickets on this flight.\n", cancelled);
		else if (cancelled == 0)
			printf("Insufficient number of tickets booked.\n");
		else
			printf("Requested flight not found.\n");
	}

	// Service 6 ::
	// destinations <source>
	if (type == 6) {

		int32_t amount = unpack_int32(&ptr);
		if (amount == 0)
			printf("No flights departing from this airport.\n");
		else if (amount < 0)
			printf("Source airport not recognized.\n");
		else {
			printf("There are flights departing to following destinations:\n");

			for (int i = 0; i < amount; i++ ) {
				int tmp_length = unpack_int32(&ptr);
				char tmp_buffer[tmp_length+1];
				unpack_str(&ptr, tmp_buffer, tmp_length);
				printf(" - %s\n", tmp_buffer);
			}

			printf("\n");
		}
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
