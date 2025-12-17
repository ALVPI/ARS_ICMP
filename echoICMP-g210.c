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
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>   //for using ips 
#include <netinet/ip_icmp.h> // icmp  
#include <sys/time.h>      // for the  rtt of the client
#include <errno.h>

//CTEs that we are going to need
#define PING_COUNT 5  //number of the msg type
#define ICMP_ECHO_TYPE 8  //EchoRequest type
#define ICMP_ECHO_CODE 0 //EchoRequest
#define ICMP_ECHO_REPLY 0  //EchoReply type
#define ICMP_DATA_SIZE 56
#define SIZE 1500   //buffer size for the msgs
#define OPTIONAL_DATA "Echo to check Availability"
#define PING_LEN (strlen(OPTIONAL_DATA)+1)
#define TIMEOUT_SEC 5 //Time wait a host answer
#define SLEEP_TIME 5 //Time btw sendings


uint16_t checksumCalculation(void *data, int len) {
    uint16_t *buf = data;
    uint32_t sum = 0;


    for(int i=0; i<len/2; i++){
        sum += *buf;
        buf++;
    }

    if (len == 1)
        sum += *(uint8_t *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16_t)(~sum);
}

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
  struct sockaddr_in host, local_addr; //al que vamos a hacer el ping
  struct sockfd; //socket
  struct timeval t_start, t_end;
  
  fd_set readfds;
  struct timeval timeout;


  Echo echo;
  EchoReply *echoRep;


  memset(&host, 0, sizeof(host));

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

  memset(&local_addr,0,sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;

  //Now we are going to bind the local interface
  if (bind (sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)))
  {
    perror("An error has ocurred during the bindeing process\n");
    exit(1);
  }

   

  //Here is the main for loop for sending the petitions
  for (int i=0; i<PING_COUNT;i++)
  {
    //We get the PID of the process 
    pid_t pid = getpid();
    
    memset(&echo, 0, sizeof(Echo));
    echo.icmpHdr.type = ICMP_ECHO_TYPE;
    echo.icmpHdr.code = ICMP_ECHO_CODE;
    echo.id = pid & 0xFFFF;
    echo.sequence = i;
    
    memset(echo.payload, 0, LEN);
    strncpy(echo.payload, OPTIONAL_DATA, LEN - 1);

    echo.icmpHdr.checksum = 0;
    echo.icmpHdr.checksum = checksumCalculation(&echo, sizeof(Echo));


    /*We create the header

    icmpPing->checksum =0; //we initialice the checksum to 0
    icmpPing->type = ICMP_ECHO_TYPE;
    icmpPing->code = ICMP_CODE;
    icmpPing-> un.echo.id = htons(pid);
    icmpPing-> un.echo.sequence = htons(seq);

    //Add optional data
    memcpy(snd_buffer + sizeof(struct icmphdr), OPTIONAL_DATA, PING_LEN);

    icmp_packet->checksum = checksumCalculation((unsigned double *)icmp_packet, PING_LEN);
    */
    gettimeofday(&t_start, NULL);

    if(sendto(sockfd, echo, sizeof(Echo), 0,
        (struct sockaddr *)&host, sizeof(host))<0){

          perror("Error al enviar el echo");
          close(sockfd);
          exit(EXIT_FAILURE);
        }

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

    if (ready == 0) {
      printf("Echo %d: Timeout\n\n", i);
      sleep(1);
      continue;
    }
    else if (ready < 0) {
      perror("Timeout expired...");
      break;
    }


    socklen_t addr_len;
    addr_len = sizeof(host);
    int bytes = recvfrom(sockfd, rcv_buffer, SIZE, 0,
        (struct sockaddr *)&host, &addr_len);
  
    if (bytes < 0) {
      perror("recvfrom");
      close(sockfd);
      exit(EXIT_FAILURE);
    }

    gettimeofday(&t_end, NULL);
  }

  
  
  // FALTA CALCULAR EL RTT, CREAR EL ECHO REPLY Y COMPROBAR LA RESPUESTA

  return 0;
