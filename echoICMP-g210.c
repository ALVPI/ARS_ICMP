/*
* Tema8: ICMP-ECHO
*
* Llorente del Olmo, Sergio
* Lopez Primo, Alvaro
*
* */
#include<ip-icmp.h> //Camara's header file, we must import it, and checked that we dont import the libraries twice.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>   //for using ips 
#include <netinet/ip_icmp.h> // icmp  
#include <sys/time.h>      // for the  rtt of the client
#include <errno.h>


int main (int argc, char*argv[])
{


}
