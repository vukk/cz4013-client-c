#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdint.h>
#include <assert.h>

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

typedef enum {
	DESTINATIONS, FIND, SHOW, RESERVE, CANCEL, MONITOR, EXIT, HELP
} command_t;


static uint32_t nextmsgnum = 1;	// next message number

typedef struct Message
{
   uint32_t id;
   unsigned char service;
   unsigned char *data;
   int length_data;
} Message;

Message * message_new(unsigned char service) {
	assert(service < 8);
	Message *msg;
	msg = malloc(sizeof *msg);
	if(!msg) {
		perror("ERROR: Allocating a message struct failed\n");
		exit(1);
	}
	msg->id = htonl(nextmsgnum++);
	// service is always less than 8, so we don't need to care about byte order
	msg->service = service;
	return msg;
}

void message_destroy(Message * msg) {
	free(msg->data);
}


// ugly
command_t pick_cmd (char *cmdstr) {
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


unsigned char * pack_uint32(unsigned char *to, uint32_t intval)
{
  	/* assumes 8-bit char of course */
  	to[0] = intval >> 24;
  	to[1] = intval >> 16;
  	to[2] = intval >> 8;
  	to[3] = intval;
  	return to + 4; // return pointer to where packing ended
}

// will pack the first n chars, then end with a \0
unsigned char * pack_str(unsigned char *to, char *str, int n)
{
	// could memcpy...
  	/* assumes 8-bit char of course */
	for (int i = 0; i < n; i++) {
		to[i] = str[i];
	}
	to[n] = '\0';
  	return to + n + 1; // return pointer to where packing ended
}

unsigned char * packet_new(Message *msg, int *length)
{
	unsigned char *buffer;
	*length = sizeof(uint32_t) + sizeof(unsigned char) + msg->length_data;
	buffer = malloc(*length);
	unsigned char *ptr = buffer;

	//memcpy_s(ptr, sizeof(uint32_t), msg->id, sizeof(uint32_t));
	memcpy(ptr, &(msg->id), sizeof(uint32_t));
	ptr = ptr + sizeof(uint32_t);
	//memcpy_s(ptr, sizeof(unsigned char), msg->id, sizeof(unsigned char));
	memcpy(ptr, &(msg->service), sizeof(unsigned char));
	ptr = ptr + sizeof(unsigned char);
	//memcpy_s(ptr, msg->length_data, msg->data, msg->length_data);
	memcpy(ptr, msg->data, msg->length_data);

	return buffer;
}

void packet_destroy(unsigned char * buffer) {
	free(buffer);
}

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


static Message *msgbuffer; 		// contains the message to send


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
			case EXIT: return 0;
			default:
				puts(msg_help);
				puts(msg_input);
				continue;
		}

		// Networking

		// construct packet
		int packet_length;
		unsigned char *packet = packet_new(msgbuffer, &packet_length);

		// Print out contents of packet being sent.
		printf("Packet size : %d  \n", packet_length);
		printf("byte n : contents \n");
		for(int i = 0; i < packet_length; i++){
			printf("   %d   :    %d  \n", i, packet[i]);
		}

		// send
		printf("Sending packet...\n");
		if (sendto(socket_fd, packet, packet_length, 0, (struct sockaddr *)&addr_remote, addr_len) == -1) {
			perror("ERROR: could not send packet.\n");
			exit(7);
		}

		// receive
		printf("Waiting for reply...\n");
		int num_recv_bytes = recvfrom(socket_fd, recvbuffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&addr_remote, &addr_len);
		if(num_recv_bytes <= -1) {
			perror("ERROR: could not receive.\n");
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
		message_destroy(msgbuffer);

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


// TODO Message format is [int id, byte service, <data>]

// service 6
void cmd_destinations(char *from) {
	Message *msg = message_new(6);
	int len_from = strlen(from);
	unsigned char *data;
	// +1 to account for null character in the string
	msg->length_data = sizeof(uint32_t) + len_from + 1;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_uint32(ptr, htonl(len_from));
	ptr = pack_str(ptr, from, len_from);

	msg->data = data;
	msgbuffer = msg;

	//printf("service: \n");
	//dumpchar(msg->service);
	//printf("\n");
}

// service 1
void cmd_find(char *from, char *to) {
	Message *msg = message_new(1);
	uint32_t len_from	= strlen(from);
	uint32_t len_to 	= strlen(to);
	unsigned char *data;
	// +2 to account for null characters on the two strings
	msg->length_data = sizeof(uint32_t)*2 + len_from + len_to + 2;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_uint32(ptr, htonl(len_from));
	ptr = pack_str(ptr, from, len_from);
	ptr = pack_uint32(ptr, htonl(len_to));
	ptr = pack_str(ptr, to, len_to);

	msg->data = data;
	msgbuffer = msg;
}

// service 2
void cmd_show(char *id) {
	uint32_t flightid;
	if (!sscanf(id, "%i", &flightid)) {
		perror("ERROR: Flight ID has to be an integer.\n");
		return;
	};
	Message *msg = message_new(2);
	unsigned char *data;
	msg->length_data = sizeof(uint32_t);
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_uint32(ptr, htonl(flightid));

	msg->data = data;
	msgbuffer = msg;
}

// service 3
void cmd_reserve(char *id, char *num) {
	uint32_t flightid;
	uint32_t seats;
	if (!sscanf(id, "%i", &flightid)) {
		perror("ERROR: Flight ID has to be an integer.\n");
		return;
	};
	if (!sscanf(num, "%i", &seats)) {
		perror("ERROR: Number of seats has to be an integer.\n");
		return;
	};
	Message *msg = message_new(3);
	unsigned char *data;
	msg->length_data = sizeof(uint32_t)*2;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_uint32(ptr, htonl(flightid));
	ptr = pack_uint32(ptr, htonl(seats));

	msg->data = data;
	msgbuffer = msg;
}

// service 5
void cmd_cancel(char *id, char *num) {
	uint32_t flightid;
	uint32_t seats;
	if (!sscanf(id, "%i", &flightid)) {
		perror("ERROR: Flight ID has to be an integer.\n");
		return;
	};
	if (!sscanf(num, "%i", &seats)) {
		perror("ERROR: Number of seats has to be an integer.\n");
		return;
	};
	Message *msg = message_new(5);
	unsigned char *data;
	msg->length_data = sizeof(uint32_t)*2;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_uint32(ptr, htonl(flightid));
	ptr = pack_uint32(ptr, htonl(seats));

	msg->data = data;
	msgbuffer = msg;
}

// service 4
void cmd_monitor(char *id, char *time) {
	uint32_t flightid;
	uint32_t seconds;
	if (!sscanf(id, "%i", &flightid)) {
		perror("ERROR: Flight ID has to be an integer.\n");
		return;
	};
	if (!sscanf(time, "%i", &seconds)) {
		perror("ERROR: Monitoring time has to be an integer (seconds).\n");
		return;
	};
	Message *msg = message_new(4);
	unsigned char *data;
	msg->length_data = sizeof(uint32_t)*2;
	data = malloc(msg->length_data);
	unsigned char *ptr = data;

	// pack
	ptr = pack_uint32(ptr, htonl(flightid));
	ptr = pack_uint32(ptr, htonl(seconds));

	msg->data = data;
	msgbuffer = msg;
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
