#include "myudp.h"

char *ip;
bool antena_started;

bool is_antena_started()
{
	sleep(1);
	return antena_started;
}


void *udp_alive_thread ()
{
  const char *hello = "IDU ALIVE";
  const char *buffer[100] = { 0 };
  int sockfd, read_len, len;
  struct sockaddr_in servaddr, cliaddr;
  int no_repeats;


  sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr (ip);
  servaddr.sin_port = htons (PORT_UDP_ALIVE);	//destination port for incoming packets

//    struct timeval read_timeout;
//    read_timeout.tv_sec = 0;
//    read_timeout.tv_usec = 10;
//    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  cliaddr.sin_family = AF_INET;
  cliaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  cliaddr.sin_port = htons (PORT_UDP_ALIVE);	//source port for outgoing packets
  bind (sockfd, (struct sockaddr *) &cliaddr, sizeof (cliaddr));

  while (1)
    {
      sendto (sockfd, (const char *) hello, strlen (hello), 0,
	      (struct sockaddr *) &servaddr, sizeof (servaddr));
      sleep (ALIVE_DELAY);
      read_len =
	recvfrom (sockfd, &buffer, sizeof (buffer), 0,
		  (struct sockaddr *) &cliaddr, (socklen_t *) & len);
      buffer[read_len] = 0;
      if (strcmp ((char *) buffer, "ODU ALIVE") == 0)
	{
	  antena_started = true;
	  no_repeats = ODU_REPEATS;
	}
      else
	{
	  if (no_repeats == 0)
	    antena_started = false;
	  else
	    no_repeats--;
	}
    }
}



void *udp_getspeed_thread ()
{
  const char *hello = "getspeed";
  const char *buffer[100] = { 0 };
  int sockfd, read_len, len;
  struct sockaddr_in servaddr, cliaddr;
  sockfd = socket (AF_INET, SOCK_DGRAM, 0);

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr (ip);
  servaddr.sin_port = htons (PORT_UDP_SPEED);	//destination port for incoming packets

  cliaddr.sin_family = AF_INET;
  cliaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  cliaddr.sin_port = htons (PORT_UDP_SPEED);	//source port for outgoing packets
  bind (sockfd, (struct sockaddr *) &cliaddr, sizeof (cliaddr));

  while (1)
    if (antena_started)
      {
	sendto (sockfd, (const char *) hello, strlen (hello), 0,
		(struct sockaddr *) &servaddr, sizeof (servaddr));
	sleep (GETSPEED_DELAY);
	read_len =
	  recvfrom (sockfd, buffer, sizeof (buffer), 0,
		    (struct sockaddr *) &cliaddr, (socklen_t *) & len);
	buffer[read_len] = 0;
      };

}


void make_udp_threads(char *new_ip) {
  ip = new_ip;
  pthread_t t1;
  pthread_create (&t1, NULL, udp_alive_thread, NULL);
  pthread_detach (t1);

/*    pthread_t t2;
    pthread_create(&t2, NULL, udp_getspeed_thread, NULL);
    pthread_detach(t2); */
}