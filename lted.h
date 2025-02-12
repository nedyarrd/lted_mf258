#ifndef __LTED_H
#define __LTED_H

#include "myll.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>


#include <stdbool.h>

//Network related includes:
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define HOST "169.254.0.1"
#define UCI "uci"
#define UCI_PATH_LEN 2048


#define AT_TIME_BETWEEN_CHECKS 10
//Target host details:
#define PORT 7788


#define BUFFER_SIZE 1024*256 // 1024 SMS or other * 256 chars

#define FREE_AT_RETURN(msg_struct) free(msg_struct->message);  my_ll_free(msg_struct->head); free(msg_struct); 

typedef struct
{
	char 	*pin;
	int 	sms;
	char 	*lte_network_interface; // network of 
	int 	allow_roaming;
	int		change_dns;
	int		change_dns1;
	int		change_dns2;
} uci_variables;

typedef struct
{
	char *apn_name;
	char *client_ip;
	char *netmask;
	char *gateway;
	char *dns1;
	char *dns2;
	int mtu;
} connection_data;


typedef struct
{
  char *message;
  bool result;
  int no_lines;
  struct linenode *head;
  bool conn_status;
} at_return;

#endif


int make_connection (char *ip);
void msg_split_lines (at_return * msg);
struct linenode *line_split_char(char *line,char splitter);
int send_message (char *message);
at_return *send_at_command(char *message);
at_return *send_at_command_delay(char *message, int seconds_delay_after_message);
int uci_get_int(char *path);
char *uci_get_string(char *path);
uci_variables *uci_get_variables();
void get_uci_config();
void uci_set_int(char *path, int value);
void uci_set_string(char *path, char *value);
void uci_commit();
void uci_add_list(char *path, char *value);
void uci_del(char *path);
void network_reload();
bool is_atdaemon_connection();