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

void handle_icmp_error(int type, int code) {
    if (type == 3) {
        printf("Destination Unreachable: ");
        switch(code) {
            case 0: printf("Net Unreachable (Type 3, Code 0)\n"); break;
            case 1: printf("Host Unreachable (Type 3, Code 1)\n"); break; // [cite: 80]
            case 3: printf("Port Unreachable (Type 3, Code 3)\n"); break;
            default: printf("Error code %d\n", code); break;
        }
    } else if (type == 11) {
        printf("Time Exceeded: Time-to-Live Exceeded in Transit (Type 11, Code 0)\n"); // [cite: 85]
    } else {
        printf("ICMP Error: Type %d, Code %d\n", type, code);
    }
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
  if(inet_aton(argv[1], &host.sin_addr)==0)
  {
    perror("Invalid IP\n");
    exit(1);
  }
  local_addr.sin_addr.s_addr = INADDR_ANY;

  //Now we are going to bind the local interface
  if (bind (sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)))
  {
    perror("An error has ocurred during the bindeing process\n");
    exit(1);
  }

  //Initial message
  printf("PING %s: %zu bytes of data\n", argv[1], strlen(OPTIONAL_DATA));
   

  //Here is the main for loop for sending the petitions
for (int seq = 1; seq <= PING_COUNT; ++seq) {
    /* 1. construye datagrama ICMP plano */
    uint8_t icmp_pkt[sizeof(struct icmphdr) + strlen(OPTIONAL_DATA)];
    struct icmphdr *ic = (struct icmphdr *)icmp_pkt;

    ic->type     = ICMP_ECHO_TYPE;                     // 8
    ic->code     = ICMP_ECHO_CODE;                     // 0
    ic->checksum = 0;                                  // se calcula después
    ic->un.echo.id       = htons(getpid() & 0xFFFF);
    ic->un.echo.sequence = htons(seq);
    memcpy(icmp_pkt + sizeof(struct icmphdr), OPTIONAL_DATA, strlen(OPTIONAL_DATA));

    ic->checksum = checksumCalculation(icmp_pkt, sizeof(icmp_pkt)); // bytes que viajan

    /* 2. envío y RTT */
    struct timeval t_start, t_end;
    gettimeofday(&t_start, NULL);
    if (sendto(sockfd, icmp_pkt, sizeof(icmp_pkt), 0,
               (struct sockaddr *)&host, sizeof(host)) < 0) {
        perror("sendto"); continue;
    }
    sent++;

    /* 3. espera respuesta con timeout */
    fd_set rfds; struct timeval tv = { .tv_sec = TIMEOUT_SEC, .tv_usec = 0 };
    FD_ZERO(&rfds); FD_SET(sockfd, &rfds);
    int ready = select(sockfd + 1, &rfds, NULL, NULL, &tv);
    if (ready == 0) { printf("Timeout Expired...\n"); continue; }
    if (ready <  0) { perror("select"); break; }

    /* 4. recibe y extrae cabeceras */
    socklen_t fromlen = sizeof(from_addr);
    int n = recvfrom(sockfd, rcv_buffer, sizeof(rcv_buffer), 0,
                     (struct sockaddr *)&from_addr, &fromlen);
    if (n < 0) { perror("recvfrom"); continue; }
    gettimeofday(&t_end, NULL);

    struct iphdr   *ip     = (struct iphdr *)rcv_buffer;
    int ip_hdr_len         = ip->ihl * 4;
    struct icmphdr *icmp_r = (struct icmphdr *)(rcv_buffer + ip_hdr_len);

    /* 5. procesa respuesta */
    if (icmp_r->type == 0 && icmp_r->code == 0) {               // Echo Reply
        if (icmp_r->un.echo.id   == ic->un.echo.id &&
            icmp_r->un.echo.sequence == ic->un.echo.sequence) {
            double rtt_ms = diff_time(&t_end, &t_start);
            total_rtt += rtt_ms;
            if (rtt_ms < min_rtt) min_rtt = rtt_ms;
            if (rtt_ms > max_rtt) max_rtt = rtt_ms;
            recv_count++;
            printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                   n, inet_ntoa(from_addr.sin_addr), seq, ip->ttl, rtt_ms);
        }
    }
    else if (icmp_r->type == ICMP_ECHO_TYPE) {                  // loopback sin procesar
        printf("ICMP Datagram Not Processed...\n");
    }
    else {
        handle_icmp_error(icmp_r->type, icmp_r->code);          // errores varios
    }

    /* 6. espera 1 s entre envíos (salvo el último) */
    if (seq < PING_COUNT - 1) sleep(SLEEP_TIME);
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
}
