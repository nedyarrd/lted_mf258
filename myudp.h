#ifndef __MYUDP_H
#define __MYUDP_H

#include <stdbool.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define PORT_UDP_ALIVE 4667
#define PORT_UDP_SPEED 4666

#define ALIVE_DELAY 5		// seconds between sending ALIVE requests
#define ODU_REPEATS 1		// how many repeats without response to mark antena as unavailable
#define GETSPEED_DELAY 11	// seconds between sending getspeed requests

#define AT_COMMANDS_DELAY 5	// seconds between sending ALIVE requests




bool is_antena_started();
void make_udp_threads(char *ip);
void renew_udp_threads();
#endif