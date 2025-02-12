#include "lted.h"
#include "at-commands.h"

#define CEREG_WAIT 1
#define CEREG_MAX_RETRIES 10

connection_data data;
connection_data last_data;

void at_check_pin()
{
	at_return *at_msg;
	at_msg = send_at_command ("AT+CPIN?");	// check that pin is enabled for SIM
	if (!at_msg->conn_status) return; 
    if(strcmp(my_ll_get_first(at_msg->head),"+CPIN: READY") !=0)
		{
		if (strcmp(my_ll_get_first(at_msg->head),"+CPIN: SIM PUK") == 0)
			{
			perror ("PUK needed");
			exit (-1);
			}
		}
    if (strcmp(my_ll_get_first(at_msg->head),"+CPIN: SIM PIN") == 0)
	    {
	    at_msg =  send_at_command ("AT+CPINR");	// get PIN, PUK status - retries max used;
	    if (!at_msg->conn_status) return; 
	    struct linenode *tmp;
	    char *cpinr_tmp = "+CPINR: SIM PIN,";
            char *buf_sim = malloc(BUFFER_SIZE);
            my_ll_foreach(tmp,at_msg->head)
                if (strstr(tmp->line,cpinr_tmp) != NULL)
                    {
                    strcpy(buf_sim,tmp->line+strlen(cpinr_tmp));
                    buf_sim[strcspn(buf_sim,",")] =  0;
                    if (atoi(buf_sim) <= 1)
                        {
                        perror("Last PIN try, not continuing");
                        exit(-1);
                        }
					}
			free(buf_sim);
			uci_variables *uci_config;
			uci_config = uci_get_variables();
			int pin_length = strlen(uci_config->pin);
			if ((pin_length < 4) || (pin_length > 8))
			{
				perror("PIN is needed and  PIN provided");
				exit(-1);
			}
			char pinbuf[20] = {0};
			sprintf(pinbuf,"AT+CPIN=\"%s\"",uci_config->pin);
			at_msg = send_at_command(pinbuf);
			if (!at_msg->conn_status) return; 
			if (!at_msg->result)
				{
				perror("PIN incorect, not continuing");
				exit(-1);
				}
        }
	FREE_AT_RETURN(at_msg);
}

bool at_activatepdp4()
{
	printf("Activating PDP Context\n");
	at_return *at_msg;
	at_msg = send_at_command_delay("AT%%GPDNTYPE=3,1",2);
	if (!at_msg->conn_status) return at_msg; 
	at_msg = send_at_command_delay("AT+CGACT=1,3",2);
	if (!at_msg->conn_status) return at_msg; 
}

bool at_cereg()
{
	int i;
	bool tmp = false;
	at_return *at_msg;
	at_msg = send_at_command_delay("AT+CEREG=0",CEREG_WAIT); // make sure we don't wait for some error codes. just check that we are registered in network
	if (!at_msg->conn_status) return at_msg;
	for (i = 0; i < CEREG_MAX_RETRIES; i++) // wait max 
		{
		at_msg = send_at_command_delay("AT+CEREG?",CEREG_WAIT);
		if (!at_msg->conn_status) return at_msg; 
		if (at_msg->result) 
			{
			// 	+CEREG: <n>,<stat>
			if (strcmp(my_ll_get_first(at_msg->head),"+CEREG: 0,1"))
				{
					tmp = true;
					break;
				}
			if (strcmp(my_ll_get_first(at_msg->head),"+CEREG: 0,5"))
				{
					// if not alowed to roaming unregister from network
					uci_variables *uci;
					uci = uci_get_variables();
					if (!uci->allow_roaming)
						{
						send_at_command("AT+COPS=2");
						perror("Roaming not allowed, so deregistering from network and exiting daemon");
						exit(-1);
						}
					tmp = true;
					break;
				}
			if (strcmp(my_ll_get_first(at_msg->head),"+CEREG: 0,3"))
				{
					perror("Registration denied in this network");
					exit(-1);
				}

			if (strcmp(my_ll_get_first(at_msg->head),"+CEREG: 0,90"))
				{
					perror("SIM card failure");
					exit(-1);
				}
			
			//  +CEREG: 0,1 - registered
			//  +CEREG: 0,5 - roaming network
			//	+CEREG: 0,2 - not registered but is searching for operator
			//	+CEREG: 0,3 - registration denied
			//	+CEREG: 0,4 - unknown - maybe out of range
			// 	+CEREG: 0,90 - sim card failure
			
			}
		}
	
	FREE_AT_RETURN(at_msg);
	return tmp;
}


void at_cgcontrdp()
{
	printf("Geting context\n");
		at_return *at_msg;
		at_msg = send_at_command_delay("AT+COPS=0",2); 		// https://onomondo.com/blog/at-command-cops/#at-cops 
		if (!at_msg->conn_status) return; 							// AT+COPS=0 - automatic operator selection
													// AT+COPS=1,<format>,<operator> - manual operator selection
													// AT+COPS=2 - deregister from network
													// AT+COPS=3,<format> - sets that command AT+COPS? return only specified format
													// AT+COPS=4,<format>,<operator> - manual operator selection with automatic fallback
													//
													// AT+COPS? - returns current mode
													// return:
													/// +COPS: [selection mode],[operator format],[operator],[radio access technology]
													//  [selection mode]
													//			0 - unknown
													//			1 - manual
													//			2 - deregistered
													//	[operator format]
													//			0 - long alphanumeric "T-Mobile"
													//			1 - short alphanumeric "TMO"
													//			2 - MMC - 26002 (MCC+MNC)
													// [radio access technology]
													// 			0 = GSM						(2G)
													//			1 = GSM Compact
													//			2 = UTRAN
													//			3 = GSM w/EGPRS
													//			4 = UTRAN w/HSDPA
													//			5 = UTRAN w/HSUPA
													//			6 = UTRAN w/HSDPA and HSUPA
													//			7 = E-UTRAN					(LTE)
													//			8 = EC-GSM-IoT (A/Gb mode)
													//			9 = E-UTRAN (NB-S1 mode)
													
		at_msg = send_at_command("AT+CGCONTRDP=3"); // I think that <cid> = 3 is context id or apn number leaving it hardwired for now
													// CGCONTRDP? returns only one line: AT+CGCONTRDP: (3) - so only one is enabled
		if (!at_msg->conn_status) return; 
		struct linenode *cgcont = line_split_char(my_ll_get_first(at_msg->head),',');
													// typical response in that case is:
													// +CGCONTRDP: 3,5,"internet.mnc002.mcc260.gprs","10.210.86.117.255.255.255.0","10.210.86.138","213.158.199.1","213.158.199.5","","",0,0,1500
													// 0 - +CGCONTRDP: 3					<cid>
													// 1 - 5								<bearer id>
													// 2 - "internet.mnc002.mcc260.gprs"	<apn>
													// 3 - "10.210.86.117.255.255.255.0" 	<local addres. subnet mask>
													// 4 - "10.210.86.138"					<gateway address>
													// 5 - "213.158.199.1"					<dns1>
													// 6 - "213.158.199.5"					<dns2>
													// 7 - ""
													// 8 - ""
													// 9 - 0
													// 10 - 0
													// 11 - 1500 							<interface MTU> - good to change
		struct linenode *tmp;
		int i = 0;
		my_ll_foreach(tmp,cgcont)
			{
				switch(i)
				{
					case 2:
						data.apn_name = tmp->line;
						break;
					case 3:
						// first we need to split for two buffers;
						int j = 0;
						int dot_count = 0;
						while ((j < strlen(tmp->line) && (dot_count < 4)))
							{
							if (tmp->line[j] == '.')
								dot_count++;
							j++;
							}
						// we have 4th dot in string
						char *ip;
						char *mask;
						ip = (char *)malloc(j+1);
						mask = (char *)malloc(strlen(tmp->line)-j+1);
						strncpy(ip,tmp->line+1,j-2); // tmp->line+1 remove "
						strcpy(mask,tmp->line+j);    // tmp->line+j one char after that dot "."
						ip[j-2] = 0;		     // trim to dot position -2 because trailing \" and dot
						mask[strlen(mask)-1]=0;	     // remove trailing \"
						data.client_ip = ip;
						data.netmask = mask;
						break;
					case 4: 
						tmp->line = tmp->line+1;
						tmp->line[strlen(tmp->line)-1] = 0; // remove ""
						data.gateway = tmp->line;
						break;					
					case 5:
						tmp->line = tmp->line+1;
						tmp->line[strlen(tmp->line)-1] = 0; // remove ""
						data.dns1 = tmp->line;
						break;					
					case 6:
						tmp->line = tmp->line+1;
						tmp->line[strlen(tmp->line)-1] = 0; // remove ""
						data.dns2 = tmp->line;
						break;					
					case 11:
						data.mtu = atoi(tmp->line);
						break;					
				}
			i++;	
			}
		my_ll_free(tmp);
		// check if last data changed
		bool data_changed = false;
		if (last_data.client_ip != NULL)
    		    if (strcmp(data.client_ip,last_data.client_ip) != 0) data_changed = true;
		if (last_data.netmask != NULL)
    		    if (strcmp(data.netmask,last_data.netmask) != 0)  data_changed = true;
		if (last_data.gateway != NULL)
		    if (strcmp(data.gateway,last_data.gateway) != 0)  data_changed = true;	
		if (last_data.dns1 != NULL)
		    if (strcmp(data.dns1,last_data.dns1) != 0)  data_changed = true;
		if (last_data.dns2 != NULL)
		    if (strcmp(data.dns2,last_data.dns2) != 0) data_changed = true;
		if (data.mtu != last_data.mtu)  data_changed = true;
		if (data_changed == false) return;
		
		uci_variables *uci;
		uci = uci_get_variables();
		char uci_path[UCI_PATH_LEN];
		bzero(uci_path,UCI_PATH_LEN);
		sprintf(uci_path,"network.%s.ipaddr",uci->lte_network_interface);
		uci_set_string(uci_path,data.client_ip);
		sprintf(uci_path,"network.%s.mask",uci->lte_network_interface);
		uci_set_string(uci_path,data.netmask);
		sprintf(uci_path,"network.%s.gateway",uci->lte_network_interface);
		uci_set_string(uci_path,data.gateway);
		sprintf(uci_path,"network.%s.mtu",uci->lte_network_interface);
		uci_set_int(uci_path,data.mtu);
		if (uci->change_dns != 0)
			{
			sprintf(uci_path,"network.%s.dns",uci->lte_network_interface);
			uci_del(uci_path);
			if (uci->change_dns1 | uci->change_dns2) 
				{
					uci->change_dns1 = 1;
					uci->change_dns2 = 1;
				}
			if (uci->change_dns1 != 0)
				{
				sprintf(uci_path,"network.%s.dns",uci->lte_network_interface);
				uci_add_list(uci_path,data.dns1);
				}
			if (uci->change_dns2 != 0)
				{
				sprintf(uci_path,"network.%s.dns",uci->lte_network_interface);
				uci_add_list(uci_path,data.dns2);
				}
				
			}
		last_data  = data;
		uci_commit();
		network_reload();
}		
