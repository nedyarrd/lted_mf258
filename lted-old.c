//General includes:
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

//Target host details:
#define PORT 7788
#define PORT_UDP_ALIVE 4667
#define PORT_UDP_SPEED 4666
#define HOST "169.254.0.1"
#define BUFFER_SIZE 4096

#define EXCEPTION_NO_OK


#define ALIVE_DELAY 5 // seconds between sending ALIVE requests
#define ODU_REPEATS 6 // how many repeats without response to mark antena as unavailable
#define GETSPEED_DELAY 11 // seconds between sending getspeed requests

#define AT_COMMANDS_DELAY 5 // seconds between sending ALIVE requests

//using namespace std;
int sd;
char *ip;
bool antena_started = false;


typedef struct {
    char *message;
    bool result;
} at_return;



void *udp_alive_thread()
    {
    const char *hello = "IDU ALIVE";
    const char *buffer[100] = {0};
    int sockfd,read_len,len;
    struct sockaddr_in servaddr,cliaddr;
    int no_repeats;


    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(ip);
    servaddr.sin_port=htons(PORT_UDP_ALIVE); //destination port for incoming packets

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
//    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);


    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr= htonl(INADDR_ANY);
    cliaddr.sin_port=htons(PORT_UDP_ALIVE); //source port for outgoing packets
    bind(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
    
    while(1)
        {
	  sendto(sockfd, (const char *)hello, strlen(hello), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
//        std::this_thread::sleep_for(std::chrono::seconds(ALIVE_DELAY));
	  sleep(ALIVE_DELAY);
	  read_len = recvfrom(sockfd, &buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
	  buffer[read_len] = 0;
//	  string tmp_buff = string(buffer);
	  if (strcmp((char *)buffer,"ODU ALIVE") == 0)
	    {
	    antena_started = true;
	    no_repeats = ODU_REPEATS;
	    }
	    else
	    {
	    if (no_repeats == 0)
		antena_started =  false;
		else
    		no_repeats--;

	    }
        }

    }

void *udp_getspeed_thread()
    {
    const char *hello = "getspeed";
    const char *buffer[100] = {0};
    int sockfd,read_len,len;
    struct sockaddr_in servaddr,cliaddr;

//    std::cout << "Thread 2 executing\n";

//    printf("Starting UDP thread");

    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(ip);
    servaddr.sin_port=htons(PORT_UDP_SPEED); //destination port for incoming packets


    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr= htonl(INADDR_ANY);
    cliaddr.sin_port=htons(PORT_UDP_SPEED); //source port for outgoing packets
    bind(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
    
    while(1)
	if (antena_started)
        {
	  sendto(sockfd, (const char *)hello, strlen(hello), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
//          std::this_thread::sleep_for(std::chrono::seconds(GETSPEED_DELAY));
	  sleep(GETSPEED_DELAY);
	  read_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
	  buffer[read_len] = 0;
        };

    }


int make_connection()
    {
    int sd,ret;
    struct sockaddr_in server;
    struct in_addr ipv4addr;
    struct hostent *hp;

    sd = socket(AF_INET,SOCK_STREAM,0);
    server.sin_family = AF_INET;
    inet_pton(AF_INET, HOST, &server.sin_addr);
    server.sin_port = htons(PORT);
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) != 0)
	{
	printf("No connection to ODU\n");
	exit(-1);
	}
    return sd;
    }

void send_message(char *message)
    {
//    printf(message.c_str());
    send(sd,(char *)message,strlen((char *)message),0);
    }

void print_hex_array(char *buf)
    {
    for (int i = 0; i < strlen(buf); i++)
	printf("0x%02x ", buf[i]);
    }


// returns true when last line is OK
// 

void print_at_msg(char *msg)
    {
    printf("%s\n",msg);
//    for (int i = 0; i < sizeof(msg); i++)
//	printf("%s\n",(char *)msg[i]);
    }

at_return *send_at_command(char *message)
    {
    send_message(message);
    at_return *at_msg;
    char tmp_buff[BUFFER_SIZE] = {0};
    int num_bytes = read(sd,tmp_buff,BUFFER_SIZE);

    at_msg = (void *)malloc(sizeof(at_return));
//  at_msg->message = (char *)malloc(num_bytes+1);
    char eom[10] = {0};
    strncpy(eom,tmp_buff+strlen(tmp_buff)-4,4);
    if (strcmp(eom,"OK\r\n") == 0)
	{
	at_msg->message = (char *)malloc(num_bytes+1-4);
	strncpy(at_msg->message,tmp_buff,strlen(tmp_buff)-4);
	at_msg->result = true;
	return at_msg;
	}
    else 
    if (strcmp(eom,"OR\r\n") == 0) // ERROR
	{
	at_msg->message = (char *)malloc(num_bytes+1-7);
	strncpy(at_msg->message,tmp_buff,strlen(tmp_buff)-7);
	at_msg->result = true;
	return at_msg;
	}
    exit(-1);
    }

at_return *send_at_command_delay(char *message,char *at_msg,int seconds_delay_after_message)
    {
    at_return *tmp = send_at_command(message);
    //std::this_thread::sleep_for(std::chrono::seconds(seconds_delay_after_message));
    sleep(seconds_delay_after_message);
    return tmp;
    }
void print_and_free(at_return *at_msg)
    {
    printf("%c",at_msg->message);
    free(at_msg->message);
    free(at_msg);
    }


int main(int argc, char *argv[])
{
    at_return *at_msg;
    bool result;
    char *imei;
    char *cgcontrdp_msg;
    bool enabled_apn[5] = {false}; // 0 - not interesting



    if (argc == 1)
	ip = "169.254.0.1";
	else
	ip = argv[1];
    printf("Testing LTED\n");
//    sd = make_udp_connection();

    pthread_t t1;
    pthread_create(&t1, NULL, udp_alive_thread, NULL);
    pthread_detach(t1);

/*    pthread_t t2;
    pthread_create(&t2, NULL, udp_getspeed_thread, NULL);
    pthread_detach(t2);
*/
//    std::thread t1(udp_alive_thread);
//    std::thread t2(udp_getspeed_thread);
//    t1.detach();
//    t2.detach();
    sd = make_connection();				// check that is AT command
    while(1)
	{
	printf("Start sending AT Commands\n");
	while(!antena_started) {}
	// antena started so we start AT configuration
	 if(send_at_command("AT")->result != true) { 
	    perror("thats not AT daemon");
	    exit(-1);
	    }
	    else
	    {
	    printf("Connected to AT Daemon\n");
	    }
    at_msg = send_at_command("AT\%GDMCNT=1,\"RRC\"");	// unknown vedor command
    print_and_free(at_msg);
    send_at_command("AT+CGSN");			// show my imei
    print_and_free(at_msg);
    send_at_command("AT\%SWV1");			// show modem hardware revision?*/
    print_and_free(at_msg);
    sleep(10);
/*    FILE* fd = fopen("idu-new.txt","r");
    char file_buffer[1024];
    while (fgets(file_buffer,1024,fd) != NULL)	
	{
//        printf(file_buffer);
	send_at_command_delay(file_buffer,at_msg,1);
	}
    fclose(fd);/*

/*        send_at_command("AT\%SUPPBAND",at_msg);	// unknown vedor command
        send_at_command("AT\%GFREQRNG?",at_msg);	// unknown vedor command
        send_at_command("AT\%GDMCNT=1,\"RRC\"",at_msg);	// unknown vedor command
        send_at_command("AT+CGSN",imei);			// show my imei
        send_at_command("AT\%SWV1",at_msg);			// show modem hardware revision?*/

/*        send_at_command("AT\%SYSCMD=\"ucfg get config wan lte default_gateway\"",at_msg);			// get from modem that is default gateway
	if (at_msg[0].compare("%SYSCMD: default_gateway=1") != 0)
	    {
	    perror("Not a default gateway, change it at http://169.0.0.1\n");
	    exit(-1);
	    }
    	send_at_command("AT+CFUN=0",at_msg,1);				// set phone functionality

        send_at_command("AT%GDMCNT=1,\"RRC\"",at_msg);			// set SMS center to ""

        send_at_command("AT+CSCA=\"\"",at_msg);			// set SMS center to ""
        send_at_command("AT+CMGF=0",at_msg);			// set SMS Message format to TEXT
        send_at_command("AT+SMSIMS=0,\"3GPP\",\"OFF\",\"OFF\"",at_msg);	// set receive SMS without IMS ?!
        send_at_command("AT+CPMS=\"ME\",\"ME\",\"ME\"",at_msg);	// set SMS storage area at ME - phone or modem
        send_at_command("AT+CGPIAF=1,1,0,0",at_msg);		// IPV6 (::) colon notation, Subnet notation (/), Leading zeros in address, Compress zeros in adres
	for (int i = 1; i <= 4; i++)
	    {
	    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn"+std::to_string(i)+" ENABLE\"",at_msg);			// get apn1 is enabled or not in config\

	    if (at_msg[0].compare("%SYSCMD: ENABLE=1") == 0) enabled_apn[i] = true;
	    }
//	send_at_command("AT+CGCONTRDP=3",at_msg,2);
    	send_at_command("AT+CFUN=1",at_msg,1);				// set phone functionality
    	send_at_command("AT\%GSWTESTW=3,0",at_msg,1);				// set phone functionality

        send_at_command("AT+CPIN?",at_msg);			// check that pin is enabled for SIM
	if (at_msg[0].compare("+CPIN: READY") != 0 )
	    {
	    if (at_msg[0].compare("+CPIN: SIM PUK") == 0)
		{
		perror("PUK needed");
		exit(-1);
		}
    	    if (at_msg[0].compare("+CPIN: SIM PIN") == 0)
		{
		    send_at_command("AT+CPINR",at_msg);			// get PIN, PUK status - retries max used;
		    string pin_status;
		    int pos;
		    for (int i = 0; i < 4; i++)
			if ((pos = at_msg[i].find("PIN, ")) != std::string::npos)
			    {
			    string pin_tries =  at_msg[i].substr(pos+5);
			    pos = pin_tries.find(",");
			    if (std::stoul(pin_tries.substr(0,pos)) <= 1) 
				{
				perror("Only one PIN try left");
				exit(-1);
				}
			    }
		    send_at_command("AT+CPIN=\"3103\"",at_msg);			// get PIN, PUK status - retries max used;

		}
	    
	    
	    }
	bool cereg = false;
	for (int i = 0; i < 5; i++)
	    if (send_at_command("AT+CEREG?",at_msg,3)) // check that we are registered in network and wait 3 seconds
		{
    		cereg = true;
		break;		// check that we are registered in network and wait 3 seconds
		}
	if (cereg == false) continue; // no cereg - get that all once again 

//        send_at_command("AT+CGDCONT=3,\"IP\",\"internet\"",at_msg,1);		// as in https://m2msupport.net/m2msupport/atcgdcont-define-pdp-context/
	send_at_command("AT+COPS=0",at_msg,1);					// select and register in network 0 - automaticaly; 1 - manually; 2 - deregister; 
        send_at_command("AT+CGDCONT=3,\"IP\",\"internet\"",at_msg);		// as in https://m2msupport.net/m2msupport/atcgdcont-define-pdp-context/
    	send_at_command("AT+CGAUTH=3,0,\"\",\"\"",at_msg);		// set authorization CID (3),(0) - no protocol; (1) PAP; (2) CHAP;, username,password

	send_at_command("AT+CGCONTRDP=3",at_msg,2);
	print_at_msg(at_msg);
        send_at_command("AT+CREG?",at_msg);				// get registration status 0 - unsubscribe error codes, 1 - registered in home network
									// https://onomondo.com/blog/at-command-cereg/#at-cereg-1

        send_at_command("AT+CGATT?",at_msg);			// check status of packet device attach
        send_at_command("AT+CGACT?",at_msg);			// check status of packet device attach
	print_at_msg(at_msg);
    	send_at_command("AT\%DEFBRDP=3",at_msg,1);				// set phone functionality
	print_at_msg(at_msg);
    	send_at_command("AT\%GDMITEM?",at_msg,1);				// set phone functionality
	print_at_msg(at_msg);
    	send_at_command("AT+CTZU=1",at_msg,1);				// set phone functionality
    


/*
// get from active apn
        send_at_command("AT+CGDCONT=3,\"IP\",\"internet\"",at_msg);		// as in https://m2msupport.net/m2msupport/atcgdcont-define-pdp-context/
										// set PDP Context
										// CID = 3
										// PDP Type = IP
										// APN = internet
										// dont set anything else like "0.0.0.0"  PDP adress, Data Compression, Header Compression
    	send_at_command("AT+CGAUTH=3,0,\"\",\"\"",at_msg);		// set authorization CID (3),(0) - no protocol; (1) PAP; (2) CHAP;, username,password
    	send_at_command("AT+CFUN=0",at_msg,1);				// set phone functionality
    	std::this_thread::sleep_for(std::chrono::seconds(1));
    	send_at_command("AT+CFUN=1",at_msg,1);				// set phone functionality
    	send_at_command("AT\%GSWTESTW=3,0",at_msg);				// TODO: check in manual - find it on elektroda.pl
	if(!send_at_command("AT",at_msg)) { 
	    perror("thats not AT daemon - after GSWTESTW");
	    exit(-1);
	    }
	send_at_command("AT\%GSWTESTW=3,1",at_msg);				// 
	send_at_command("AT+COPS=0",at_msg,3);					// select and register in network 0 - automaticaly; 1 - manually; 2 - deregister; 
    	send_at_command("AT\%GSWTESTW=3,0",at_msg);				// TODO: check in manual - find it on elektroda.pl
    
        send_at_command("AT+CGCONTRDP=3",cgcontrdp_msg);		// as in https://m2msupport.net/m2msupport/atcgdcont-define-pdp-context/
								// look at that later - get info for routing, dns etc 
								// +CGCONTRDP: 3,5,"internet.mnc002.mcc260.gprs","10.226.58.207.255.255.255.0","10.226.58.48","213.158.199.1","213.158.199.5","","",0,0,1500

        send_at_command("AT+CGCONTRDP=4",cgcontrdpv6_msg);		// as in https://m2msupport.net/m2msupport/atcgdcont-define-pdp-context/

// added from https://eko.one.pl/forum/viewtopic.php?id=21363&p=2
        send_at_command("AT\%GLOCKCELL=1,0,-1",at_msg,1);
        send_at_command("AT\%GLOCKCELL=2,0,-1",at_msg,1);
        send_at_command("AT\%GLOCKCELL=3,0,-1",at_msg,1);
        send_at_command("AT\%GLOCKCELL=4,0,-1",at_msg,1);
        send_at_command("AT\%GLOCKCELL=5,0,-1",at_msg,1);
        std::this_thread::sleep_for(std::chrono::seconds(AT_COMMANDS_DELAY));
	
//        send_at_command("AT+CIMI",at_msg);			// get IMSI code 
//        send_at_command("AT+CLCK=\"SC\",2",at_msg);				// get information about SIM LOCK at startup; SC - SIM PIN Request; 2 - get status
									// +CLCK: 1 - SIM Locked at startup
									// https://www.emnify.com/developer-blog/at-commands-for-cellular-modules - search here
	*/
	}













/*    if(!send_at_command("AT",at_msg)) { 
	perror("thats not AT daemon");
	exit(-1);
	}
    send_at_command("AT\%GDMCNT=1,\"RRC\"",at_msg);	// unknown vedor command
    send_at_command("AT+CGSN",at_msg);			// show my imei
    printf("My IMEI is: ");
    print_at_msg(at_msg);
    send_at_command("AT+CGMM",at_msg);			// show model version
    send_at_command("AT+ZMCGMM",at_msg);		// show cat version
    send_at_command("AT\%SWV1",at_msg);			// show modem hardware revision?
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte default_gateway\"",at_msg);			// get from modem that is default gateway
    // TODO: should check it
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte autocm attach_type\"",at_msg);			// get from modem attach type
    send_at_command("AT+CSCA=\"\"",at_msg);			// set SMS center to ""
    send_at_command("AT+CMGF=0",at_msg);			// set SMS Message format to TEXT
    send_at_command("AT+SMSIMS=0,\"3GPP\",\"OFF\",\"OFF\"",at_msg);	// set receive SMS without IMS ?!
    send_at_command("AT+CPMS=\"ME\",\"ME\",\"ME\"",at_msg);	// set SMS storage area at ME - phone or modem
    // should return: +CPMS: 1,150,3,150,1,150 which means 1/150 sms in ME(Phone) store, 3 messages in SIM store, 1 message in MT store
    send_at_command("AT+CGPIAF=1,1,0,0",at_msg);		// IPV6 (::) colon notation, Subnet notation (/), Leading zeros in address, Compress zeros in adres
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 ENABLE\"",at_msg);			// get apn1 is enabled or not in config\
    // actualy returns %SYSCMD: ENABLE=0
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 pdn_type\"",at_msg);			// get pdn_type for APN1 - 0 ipv4; 1 - ipv6; 2 - ipv4v6
    send_at_command("AT+CPIN?",at_msg);			// check that pin is enabled for SIM
    // possible returns:
    // READY - sim unlocked
    // SIM PIN - waiting for code
    // SIM PUK - maximum retries of PIN reached - need PUK code
														// %SYSCMD: profile_name=apn2
    send_at_command("AT+CIMI",at_msg);			// get IMSI code 
						        // my is 26002XXXXXXXXXX - that means - 260 - POLAND; 02 - T-Mobile;  XXX... - nr MSIN - nr urządzenia w sieci GSM - nie nr telefonu
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 apn_name\"",at_msg);			// get APN1 name
    // %SYSCMD: apn_name=
    send_at_command("AT+CRSM=176,28486,0,0,17",at_msg);			// get file from SIM card - 176 - read binary, 28486 - fileid, 0 - offset, 0 - offset 2?, 17 - length 
									// +CRSM: 144,0,"02542D4D6F62696C652E706CFFFFFFFFFF" - check SIM SPN - service provider name - 02 - ignore rest to FFFF T-Mobile
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 username\"",at_msg);			// get apn1 username
    send_at_command("AT+CLCK=\"SC\",2",at_msg);				// get information about SIM LOCK at startup; SC - SIM PIN Request; 2 - get status
									// +CLCK: 1 - SIM Locked at startup
									// https://www.emnify.com/developer-blog/at-commands-for-cellular-modules - search here
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 password\"",at_msg);			// get apn1 user password
    send_at_command("AT+CPINR",at_msg);			// get PIN, PUK status - retries max used;
        // +CPINR: SIM PIN, 3, 3
	// +CPINR: SIM PUK, 10, 10
	// +CPINR: SIM PIN2, 3, 3
	// +CPINR: SIM PUK2, 10, 10
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 auth_flag\"",at_msg);			// get auth_flag - ?! 0 - PAP, 1 - CHAP
														// %SYSCMD: auth_flag=0
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 manage_interface\"",at_msg);			// get manage_interface status
    send_at_command("AT+CRSM=176,28589,0,0,0,0",at_msg);			// get ciphering information of SIM
										// https://ccdcoe.org/uploads/2018/10/Art-16-Attacking-the-Baseband-Modem-of-Breach-the-Users-Privacy-and-Network-Security.pdf - paper about cipher security in SIM cards
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn1 band_mac\"",at_msg);			// get APN1 band MAC %SYSCMD: band_mac=FF:FF:FF:FF:FF:FF

    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 ENABLE\"",at_msg);
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 pdn_type\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 profile_name\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 username\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 password\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 auth_flag\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 manage_interface\"",at_msg);		
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn2 band_mac\"",at_msg);			// all responses AS ABOVE



    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 ENABLE\"",at_msg);
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 pdn_type\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 profile_name\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 username\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 password\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 auth_flag\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 manage_interface\"",at_msg);		
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 band_mac\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn3 gen_pco\"",at_msg);			// new response: %SYSCMD: gen_pco=0010010010000100


/*
%SYSCMD: ENABLE=1
%SYSCMD: pdn_type=0
%SYSCMD: profile_name=INTERNET
%SYSCMD: apn_name=internet
%SYSCMD: username=
%SYSCMD: password=
%SYSCMD: auth_flag=0
%SYSCMD: manage_interface=
%SYSCMD: band_mac=FF:FF:FF:FF:FF:FF
%SYSCMD: gen_pco=0010010010000100

*/

/*
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 ENABLE\"",at_msg);
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 pdn_type\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 profile_name\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 username\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 password\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 auth_flag\"",at_msg);			
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 manage_interface\"",at_msg);		
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte apntable apn4 band_mac\"",at_msg);			// all responses AS in APN1 and APN2

//    send_at_command("AT+CDGCONT?",at_msg);		
//    print_at_msg("AT-CDGCONT?",at_msg); // error
// TEST IT? : at+cgdcont?

    send_at_command("AT+CGDCONT=3,\"IP\",\"internet\"",at_msg);		// as in https://m2msupport.net/m2msupport/atcgdcont-define-pdp-context/
									// set PDP Context
									// CID = 3
									// PDP Type = IP
									// APN = internet
									// dont set anything else like "0.0.0.0"  PDP adress, Data Compression, Header Compression


    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte autocm manual\"",at_msg);	// get if something like autocm is manual
											// %SYSCMD: manual=0
    send_at_command("AT\%SYSCMD=\"ucfg get config wan lte plmn_search_param roaming\"",at_msg);		// is that a roaming profile? or in roaming use?
													// %SYSCMD: roaming=
    send_at_command("AT+CFUN=1",at_msg);				// set phone functionality
									// 0 - minimal; 1 - full; 2 - disable TX; 3 - disable RX; 4 - disable RX and TX
    send_at_command("AT\%GSWTESTW=3,0",at_msg);				// TODO: check in manual - find it on elektroda.pl

    send_at_command("AT+CNUM",at_msg);				// TODO: remove - wants to find own subscriber number - not that way
    send_at_command("AT+CSCA?",at_msg);				// get SMS center number
    send_at_command("AT+CESQ",at_msg);				// get signal quality
//    print_at_msg("AT+CESQ ",at_msg);
								//
    send_at_command("AT+CESQ=?",at_msg);				// get signal quality
//    print_at_msg("AT+CESQ=? ",at_msg);

    send_at_command("AT+CREG?",at_msg);				// get registration status 0 - unsubscribe error codes, 1 - registered in home network
								// https://onomondo.com/blog/at-command-cereg/#at-cereg-1

    send_at_command("AT+CGATT?",at_msg);			// check status of packet device attach
    send_at_command("AT+CGAUTH=3,0,\"\",\"\"",at_msg);		// set authorization CID (3),(0) - no protocol; (1) PAP; (2) CHAP;, username,password

*/
    shutdown( sd, SHUT_RDWR );
}