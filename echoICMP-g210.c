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
    uint32_t sum = 0; //Acumulator

    //We go 2 by 2 adding the 2 bytes into the sum 
    for(int i=0; i<len/2; i++){
        sum += *buf;
        buf++;
    }
    //if we have a non par length  we take that last byte and add it to sum with the cast 
    if (len == 1)
        sum += *(uint8_t *)buf;

    //We add the carry 
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16_t)(~sum);
}

double diff_time (const struct timeval end_time, const struct timeval initial_time)
{
    return ((end_time-> tv_sec - initial_time->tv_sec)*1000.0 )+ ((end_time->tv_usec- initial_time->tv_usec)/1000.0);
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
  double rtt = 0.0; //round trip time of the packet 
  int recv_count=0; //number of recieved packets
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
    rtt = diff_time(&t_end, &t_start);
    total_rtt +=rtt;
    if(rtt<min_rtt) min_rtt = rtt;
    if(max_rtt<rtt) max_rtt = rtt;
    recv_count ++;
    printf("Echo Reply from %s: seq=%d TTL=%d RTT%.3f ms\n", inet_ntoa(host.sin_addr), seq, ip->ip_ttl, rtt);
  }

  //We print the final stats
  printf("--- %s estadisticas ---\n", argv[1]);
  printf("%d packets transmitted %d recieved %d packet loss\n", 
         PING_COUNT, recv_count , (PING_COUNT-recv_count*100)/100);

  //we only print if we recieved something
  if (recv_count!=0)
  {
    printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n",min_rtt, total_rtt/recv_count, max_rtt);
  }
  return 0;
