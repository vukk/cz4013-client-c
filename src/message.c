#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <string.h>
#include <netinet/in.h>

#include <errno.h>
#include <assert.h>

#include "message.h"
#include <time.h>

Message * message_new() {
	Message *msg;
	msg = malloc(sizeof *msg);
	if(!msg) {
		perror("ERROR: Allocating a message struct failed\n");
		exit(1);
	}
	msg->id = (int) time(NULL);
	return msg;
}

void message_destroy(Message * msg) {
	free(msg->data);
}

unsigned char * message_to_packet(Message *msg, int *length)
{
	unsigned char *buffer;
	*length = sizeof(int32_t) + sizeof(unsigned char) + msg->length_data;
	buffer = malloc(*length);
	unsigned char *ptr = buffer;

    int32_t flightid = htonl(msg->id);

	memcpy(ptr, &flightid, sizeof(int32_t));
	ptr = ptr + sizeof(int32_t);
	memcpy(ptr, &(msg->service), sizeof(unsigned char));
	ptr = ptr + sizeof(unsigned char);
	memcpy(ptr, msg->data, msg->length_data);

	return buffer;
}

void packet_destroy(unsigned char *packet) {
	free(packet);
}

void packet_print(unsigned char *packet, int length) {
    printf("Packet size : %d\n", length);
    printf("byte n : contents\n");
    for(int i = 0; i < length; i++){
        printf("\t%d\t:\t%d\n", i, packet[i]);
    }
}

// EOF
