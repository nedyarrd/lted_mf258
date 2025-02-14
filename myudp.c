#include "myudp.h"

char *ip;
bool antena_started;
bool renew_threads = false;
pthread_t t1;


struct sockaddr_in alive_servaddr, alive_cliaddr;
int alive_sockfd;

struct sockaddr_in speed_servaddr, speed_cliaddr;
int speed_sockfd;

bool is_antena_started()
{
	sleep(1);
	return antena_started;
}

void make_alive_socket()
{
  alive_sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  alive_servaddr.sin_family = AF_INET;
  alive_servaddr.sin_addr.s_addr = inet_addr (ip);
  alive_servaddr.sin_port = htons (PORT_UDP_ALIVE);	//destination port for incoming packets
  alive_cliaddr.sin_family = AF_INET;
  alive_cliaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  alive_cliaddr.sin_port = htons (PORT_UDP_ALIVE);	//source port for outgoing packets
  bind (alive_sockfd, (struct sockaddr *) &alive_cliaddr, sizeof (alive_cliaddr));

}

void *udp_alive_thread ()
{
  const char *hello = "IDU ALIVE";
  const char *buffer[100] = { 0 };
  int read_len, len;
  int no_repeats;
  make_alive_socket();

  while (1)
    {
      if (renew_threads)
	{
	close(alive_sockfd);
	make_alive_socket();
	}
      sendto (alive_sockfd, (const char *) hello, strlen (hello), 0,
	      (struct sockaddr *) &alive_servaddr, sizeof (alive_servaddr));
      sleep (ALIVE_DELAY);
      read_len =
	recvfrom (alive_sockfd, &buffer, sizeof (buffer), 0,
		  (struct sockaddr *) &alive_cliaddr, (socklen_t *) & len);
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


void make_speed_socket()
{
  speed_sockfd = socket (AF_INET, SOCK_DGRAM, 0);

  speed_servaddr.sin_family = AF_INET;
  speed_servaddr.sin_addr.s_addr = inet_addr (ip);
  speed_servaddr.sin_port = htons (PORT_UDP_SPEED);	//destination port for incoming packets

  speed_cliaddr.sin_family = AF_INET;
  speed_cliaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  speed_cliaddr.sin_port = htons (PORT_UDP_SPEED);	//source port for outgoing packets
  bind (speed_sockfd, (struct sockaddr *) &speed_cliaddr, sizeof (speed_cliaddr));
}

void *udp_getspeed_thread ()
{
  const char *hello = "getspeed";
  const char *buffer[100] = { 0 };
  int read_len, len;
  
  make_speed_socket();
  while (1)
    if (antena_started)
      {
      if (renew_threads)
	{
	close(speed_sockfd);
	make_speed_socket();
	}

	sendto (speed_sockfd, (const char *) hello, strlen (hello), 0,
		(struct sockaddr *) &speed_servaddr, sizeof (speed_servaddr));
	sleep (GETSPEED_DELAY);
	read_len =
	  recvfrom (speed_sockfd, buffer, sizeof (buffer), 0,
		    (struct sockaddr *) &speed_cliaddr, (socklen_t *) & len);
	buffer[read_len] = 0;
      };

}


void make_udp_threads(char *new_ip) {
  ip = new_ip;
  pthread_create (&t1, NULL, udp_alive_thread, NULL);
  pthread_detach (t1);

  pthread_t t2;
  pthread_create(&t2, NULL, udp_getspeed_thread, NULL);
  pthread_detach(t2); 
  sleep(ALIVE_DELAY); // sleep till we know that ALIVE was sent
    
}

bool first_run = true;

void renew_udp_threads() {
    renew_threads = true; // set flag so threads will detach
    if (!first_run)
	(ALIVE_DELAY > GETSPEED_DELAY) ? sleep(ALIVE_DELAY+1) : sleep(GETSPEED_DELAY+1); // sleep till we sure that threads are killed 
	else first_run = false;
    renew_threads = false;
}
