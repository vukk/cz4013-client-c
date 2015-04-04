#ifndef MESSAGE_H
#define MESSAGE_H

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

typedef struct Message
{
    struct timespec start_ts;
    int monitor_period;
    int32_t id;
    unsigned char service;
    unsigned char *data;
    int length_data;
} Message;

// message constructor
Message * message_new();
// message destructor
void message_destroy(Message * msg);


// packet constructor
unsigned char * message_to_packet(Message *msg, int *length);
// packet destructor
void packet_destroy(unsigned char *packet);
// for debugging, print packet as ints
void packet_print(unsigned char *packet, int length);


#endif

// EOF
