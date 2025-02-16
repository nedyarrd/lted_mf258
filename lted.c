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
  syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_INFO),"%s", at_msg->message);
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


static void signal_handler(int sig, siginfo_t *si, void *unused)
{

    // remove PID file when we have SIGxxx
    struct stat sts;
    if (stat("/var/run/lted.pid",&sts) != -1)
	remove("/var/run/lted.pid");
    exit(EXIT_FAILURE);
}

void setup()
{
    struct sigaction sa_seg;
    struct sigaction sa_kill;
    FILE *pidFile;

    openlog ("LTED", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    sa_seg.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_seg.sa_mask);
    sa_seg.sa_sigaction = signal_handler;
    if (sigaction(SIGSEGV, &sa_seg, NULL) == -1)
	{
        syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_ERR),"Cant handle Signals");
	exit(EXIT_FAILURE);
	}

    sa_kill.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_kill.sa_mask);
    sa_kill.sa_sigaction = signal_handler;
    if (sigaction(SIGSEGV, &sa_kill, NULL) == -1)
	{
        syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_ERR),"Cant handle Signals");
	exit(EXIT_FAILURE);
	}


    pidFile = fopen("/var/run/lted.pid", "r" );
    if (pidFile)
	{
	// read it and check that pid exists
	char pid_c[64];
	fgets(pid_c,63,pidFile);
	int pid = atoi(pid_c);
	if (pid != 0)
	    {
	    struct stat sts;
	    char *proc;
	    asprintf(&proc,"/proc/%d",pid);
	    if (stat(proc, &sts) != -1) {
    	      // process exists, so don't make another
		exit(0);
		}
	    }
    fclose(pidFile);
    // we got to this point, so process doesnt exist or PID file doesnt exist
        }
    pidFile = fopen("/var/run/lted.pid", "w" );
    if (!pidFile)
        syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_ERR),"Can't make PID file");
	else
	{
	fprintf(pidFile,"%d",getpid());
	fclose(pidFile);
	}

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
  setup();
  get_uci_config();
  make_udp_threads(host_ip);
  
  while (1)
    {
	  while (1) 
		{
		// assume if we return from PIN stuff that everyting is OK
		if (!is_atdaemon_connection())  			// there is glith in firmware of antenna. Sometimes doesnt restart at all
									// so You need to restart udp threads with new sockets
									// and restart POE Interface... mostly is achieved by down and up interface
									// that can only hapen when antena is soft restarted from web ui
		    {
		    renew_udp_threads();
		    if (!is_antena_started())
			{
		    	char *poe = uci_get_string("lte.config.poe_restart");		// if antena is not started that means no IP connection
			popen(poe,"r");
			sleep(10);
			}
	            while (is_antena_started() != true) 
			sleep(1);

		    while (make_connection (host_ip) == -1)// if we don't have tcp connection wait 5 seconds and retry
			sleep(5);
		    }
		at_check_pin();
		if (!at_cereg()) break;
		at_cgcontrdp();
		//at_activatepdp4(); - not needed now
		// now get to SMS
		sleep(AT_TIME_BETWEEN_CHECKS);
		}
    }
}
