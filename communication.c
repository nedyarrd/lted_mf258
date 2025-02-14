#include "lted.h"
#include "myll.h"

int sd = 0;
bool conn_status;

char *uci_command(char *command,char *path,char *value)
{
	FILE *fp;
	char *buf;
	char *buf2;
	char *value2 = NULL;
	char *path2 = "";
	buf = (char *)malloc(UCI_PATH_LEN);
	buf2 = (char *)malloc(UCI_PATH_LEN);
	if (path != NULL)
	    path2 = path;
	if (value != NULL)
	    {
	    value2 = (char *)malloc(strlen(value)+3);
	    bzero(value2,strlen(value)+3);
	    sprintf(value2,"='%s'",value);
	    }
	    else
	    value2 = "";
	    
	bzero(buf,UCI_PATH_LEN);
	bzero(buf2,UCI_PATH_LEN);
	
	sprintf(buf,"%s %s %s%s",UCI,command,path2,value2);
	syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_INFO),"%s\n",buf);
	fp = popen(buf, "r");
	if (fp == NULL) {
		sprintf(buf,"Cant run: %s",buf);
		perror(buf);
		exit(-1);
		}
	while(fgets(buf,sizeof(buf),fp) != NULL)
		strcat(buf2,buf);
	// uci left some \n remove it
	pclose(fp);
	buf2[strlen(buf2)-1] = 0;
	free(buf);
	if (value != NULL) free(value2);
	return buf2;

}



char *uci_get_string(char *path)
{
	return uci_command("get",path,NULL);
  }

int uci_get_int(char *path)
{
	return atoi(uci_get_string(path));
}

void uci_add_list(char *path, char *value)
{
	uci_command("add_list",path,value);
}



void uci_set_int(char *path, int value)
{
	char tmp[1024];
	sprintf(tmp,"%d",value);
	uci_command("set",path,tmp);
}

void uci_set_string(char *path, char *value)
{
	uci_command("set",path,value);
}

void uci_del(char *path)
{
	uci_command("del",path,NULL);
}


void uci_commit()
{
	uci_command("commit",NULL,NULL);
}




void network_reload()
{
	system("/etc/init.d/network reload");
	return;

}



bool is_atdaemon_connection()
{
    if ((sd == 0) || (sd == -1)) return false; else return true;
}


int make_connection (char *ip)
{
  int ret;
  struct sockaddr_in server;
  struct in_addr ipv4addr;
  struct hostent *hp;

  if (sd != 0) close(sd);

  sd = socket (AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  inet_pton (AF_INET, ip, &server.sin_addr);
  server.sin_port = htons (PORT);
  if (connect (sd, (struct sockaddr *) &server, sizeof (server)) != 0)
    {
      syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_ERR),"No connection to ODU\n");
      conn_status = false;
      return -1;
    }
  conn_status = true;
  return sd;
}


int send_message (char *message)
{
    if (sd != -1) return send (sd, (char *) message, strlen ((char *) message), 0);
	else return -1;
}


void msg_split_lines (at_return * msg)
{
  int i = 0;
  int j = 0;
  int line_number = 0;

  int len = strlen (msg->message);
  char *buf;
  char *buf2;
  msg->head = NULL;
  buf = (char *) malloc (len);
  bzero (buf, len);
  // search for first non \r\n

  while (i < len)
    {
    // ommit \r\n
     while ((i < len) && ((msg->message[i] == '\r') || (msg->message[i] == '\n')))
        i++;

     while ((i < len) && ((msg->message[i] != '\r') && (msg->message[i] != '\n')))
        buf[j++] = msg->message[i++];

     if (strlen (buf) > 0)
            {
              buf2 = (char *) malloc (strlen (buf) + 1);
              strcpy (buf2, buf);
              msg->head = my_ll_append(msg->head,buf2);
              j = 0;
              bzero (buf, len);
            }
    }
  msg->no_lines = line_number;
  free (buf);
}

struct linenode *line_split_char(char *line,char splitter)
{
	struct linenode *tmp = NULL;
	char *buf;
	char *buf2;
	buf = (char *)malloc(strlen(line));
	
	int i = 0;
	int j = 0;
	int len = strlen(line);
	while(i < len)
		{
		while((i < len) && (line[i] == splitter))
			i++;
		while((i < len) && (line[i] != splitter))
			buf[j++] = line[i++];
		if (strlen(buf) >0)
			{
			buf2 = (char *) malloc(strlen(buf)+1);
			strcpy(buf2,buf);
			tmp = my_ll_append(tmp,buf2);
			j = 0;
			bzero(buf,strlen(line));
			}
		}
	free(buf);
	return tmp;
			
}

at_return *send_at_command (char *message)
{
  at_return *at_msg;
  at_msg = (void *) malloc (sizeof (at_return));

  at_msg->conn_status = conn_status = (send_message (message) == -1) ? false : true; 
  if (!conn_status) 
    {
    sd = -1;
    return at_msg;
    }
  char tmp_buff[BUFFER_SIZE] = { 0 };
  char tmp_buff2[BUFFER_SIZE] = { 0 };
// read until end of line is OK or ERROR

  while (1)
    {
      int num_bytes = read (sd, tmp_buff, BUFFER_SIZE - 1);
      at_msg->conn_status = conn_status = ((num_bytes == -1) || (num_bytes == 0)) ? false : true; // if -1 error or 0 - no reading - so socket is ?!
      if (!at_msg->conn_status) 
	{
	sd = -1;
	return at_msg; // dont' do anything, because we don't have connection
	}
    
      tmp_buff[num_bytes] = 0;
      strncat (tmp_buff2, tmp_buff, BUFFER_SIZE - strlen (tmp_buff2));
      char eom[10] = { 0 };
      strncpy (eom, tmp_buff + strlen (tmp_buff) - strlen ("OK\r\n"), strlen ("OK\r\n"));	// does not matter what buffer we check
      if (strcmp (eom, "OK\r\n") == 0)
	{
	  tmp_buff2[strlen (tmp_buff2) - sizeof ("OK\r\n") + 1] = 0;	// trim output buffer before OK
	  at_msg->message = (char *) malloc (strlen (tmp_buff2) + 1);
	  strncpy (at_msg->message, tmp_buff2, strlen (tmp_buff2));
	  msg_split_lines (at_msg);
	  at_msg->result = true;
	  return at_msg;
	}
      if (strcmp (eom, "OR\r\n") == 0)
	{
	  strncpy (eom, tmp_buff + strlen (tmp_buff) - strlen ("ERROR\r\n"), strlen ("ERROR\r\n"));	// recheck if it is OR or ERROR
	  if (strcmp (eom, "ERROR\r\n") == 0)
	    {
	      tmp_buff2[strlen (tmp_buff2) - sizeof ("ERROR\r\n") + 1] = 0;	// trim output buffer before OK
	      at_msg->message = (char *) malloc (strlen (tmp_buff2) + 1);
	      strncpy (at_msg->message, tmp_buff2, strlen (tmp_buff2));
	      msg_split_lines (at_msg);
	      at_msg->result = false;
	      return at_msg;
	    }
	}
    }
}

at_return * send_at_command_delay (char *message, int seconds_delay_after_message)
{
  at_return *tmp = send_at_command (message);
  sleep (seconds_delay_after_message);
  return tmp;
}