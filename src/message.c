#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <netinet/in.h>
#include <assert.h>

#include "message.h"

static uint32_t nextmsgnum = 1;	// next message number

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

unsigned char * message_to_packet(Message *msg, int *length)
{
	unsigned char *buffer;
	*length = sizeof(uint32_t) + sizeof(unsigned char) + msg->length_data;
	buffer = malloc(*length);
	unsigned char *ptr = buffer;

	memcpy(ptr, &(msg->id), sizeof(uint32_t));
	ptr = ptr + sizeof(uint32_t);
	memcpy(ptr, &(msg->service), sizeof(unsigned char));
	ptr = ptr + sizeof(unsigned char);
	memcpy(ptr, msg->data, msg->length_data);

	return buffer;
}

void packet_destroy(unsigned char *packet) {
	free(packet);
}

// EOF
