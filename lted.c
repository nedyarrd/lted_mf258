//General includes:
#include "lted.h"
#include "myudp.h"
#include "at-commands.h"

//using namespace std;
char *host_ip;
uci_variables uci_config;
uci_variables *uci_get_variables() 
	{ return &uci_config; }


void print_hex_array (char *buf)
{
  for (int i = 0; i < strlen (buf); i++)
    printf ("0x%02x ", buf[i]);
}

void print_and_free (at_return * at_msg)
{
  printf ("%s", at_msg->message);
  FREE_AT_RETURN(at_msg);
}

void get_uci_config()
{
		uci_config.pin = uci_get_string("lte.config.pin");
		uci_config.sms = uci_get_int("lte.config.sms");
		uci_config.lte_network_interface = uci_get_string("lte.config.lte_network_interface");
		uci_config.allow_roaming = uci_get_int("lte.config.allow_roaming");
		uci_config.change_dns = uci_get_int("lte.config.change_dns");
		uci_config.change_dns1 = uci_get_int("lte.config.change_dns1");
		uci_config.change_dns2 = uci_get_int("lte.config.change_dns2");
}

int main (int argc, char *argv[])
{
  at_return *at_msg;
  bool result;
  char *imei;
  char *cgcontrdp_msg;
  bool enabled_apn[5] = { false };	// 0 - not interesting

  if (argc == 1)
    host_ip = "169.254.0.1";
  else
    host_ip = argv[1];
  get_uci_config();
  make_udp_threads(host_ip);
  
  while (1)
    {
/*      printf ("LTED: Waiting for antenna to respond by UDP\n");
      while (is_antena_started() != true) { sleep(1); }
      while (make_connection (host_ip) == -1)// if we don't have tcp connection wait 5 seconds and retry
	    {
	    sleep(5);
	    }

      // antena started so we start AT configuration
      printf ("Start sending AT Commands\n");
      if (send_at_command ("AT")->result != true) {
		perror ("LTED: thats not AT daemon\n");
		exit (-1);
		}
      else
	  printf ("LTED: Connected to AT Daemon\n");
      at_msg = send_at_command ("AT+CGSN");	// show my imei
      print_and_free (at_msg);
      at_msg = send_at_command ("AT\%SWV1");	// show modem hardware revision?*/
//      print_and_free (at_msg);
	  
	  while (1) 
		{
		// assume if we return from PIN stuff that everyting is OK
		if (!is_atdaemon_connection())  			// there is glith in firmware of antenna. Sometimes doesnt restart at all
									// so You need to restart udp threads with new sockets
									// and restart POE Interface... mostly is achieved by down and up interface
									// that can only hapen when antena is soft restarted from web ui
		    {
		    renew_udp_threads();
	            while (is_antena_started() != true) 
			{ 
			sleep(1);
			char *poe = uci_get_string("lte.config.poe_restart");
			printf(poe);
			popen(poe,"r");
			 }
		    while (make_connection (host_ip) == -1)// if we don't have tcp connection wait 5 seconds and retry
	    		{
			sleep(5);
	    		}
		    }
		at_check_pin();
		if (!at_cereg()) break;
		at_cgcontrdp();
		at_activatepdp4();
		sleep(AT_TIME_BETWEEN_CHECKS);
		}
    }
}
