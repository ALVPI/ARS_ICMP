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

//CTEs that we are going to need
#define PING_COUNT 5  //number of the msg type
#define ICMP_ECHO_TYPE 8  //EchoRequest type
#define ICMP_ECHO_REPLY 0  //EchoReply type
#define SIZE 1500   //buffer size for the msgs
#define OPT_PING "Echo to check Availability"
#define PING_LEN (strlen(OPTIONAL_DATA)+1)
#define TIMEOUT_SEC 5 //Time wait a host answer
#define SLEEP_TIME 5 //Time btw sendings

int main (int argc, char*argv[])
{
  //Check if the number of parameters are the correct ones.
  if(argc!=2)
  {
      fprintf("Example of normal use: ./a.out IPDirection\n");
      exit(1);
  }

  //Variables for stats
  int npSend = 0; //number of packages sended
  int npRcv = 0; //number of packages recieved
  int double total_rtt = 0.0; // round trip time total
  int min_rtt = 999999.0; // minimum round trip time
  int max_rtt = 0.0; //maximum round trip time
  char snd_buffer[SIZE];  //send buffer
  char rcv_buffer[SIZE];  //recv buffer
  struct sockaddr_in host;//al que vamos a hacer el ping
  struct sockfd; //socket
  memeset(&host, 0, sizeof(host));

  //we store the ip address
  if(inet_aton(argv[1], &host.sin_addr)==0)
  {
    fprintf("An error has ocurred during the ip translation\n")
    exit(1);
  }
  host.sin_family = AF_INET; //IPv4
  host.sin_port = 0; //the ping doesnt have a WKP is the OSI MODEL (3rd layer)
  pritnf("PING (%s): %d bytes of data\n", argv[1], PING_LEN);

  //Create the socket RAW
  //AF_INET IPv4
  //SOCK_RAW allow us to create a socket with no header
  //IPROTO_ICMP we are going to use the ICMP
  sockfd = socket(AF_INET, SOCK_RAW,IPROTO_ICMP); 
  if (sockfd <0 )
  {
    perror("An error has ocurred during the RAW SOCKET creation\n");
    exit(1);
  }

  //Now we are going to bind the local interface
  if (bind (sockfd, (struct sockaddr *)&host, sizeof(host)))
  {
    perror("An error has ocurred during the bindeing process\n");
    exit(1);
  }

  pid_t pid = getpid(); //We get the PID of the process 

  //Here is the main for loop for sending the petitions
  for (int i=0; i<PING_COUNT;i++)
  {
    struct icmphdr *icmpPing= (struct icmphdr*) snd_buffer;
    memset(snd_buffer, 0, PING_LEN);
    icmpPing->checksum =0; //we initialice the checksum to 0

    //We create the header
    icmpPing->type = ICMP_ECHO_TYPE;
    icmpPing->code = ICMP_CODE;
    icmpPing-> un.echo.id = htons(pid);
    icmpPing-> un.echo.sequence = htons(seq);

    //Add optional data
    memcpy(snd_buffer + sizeof(struct icmphdr), OPTIONAL_DATA, PING_LEN);

    icmp_packet->checksum = checksumCalculation((unsigned double *)icmp_packet, PING_LEN);
  }

  return 0;
}
