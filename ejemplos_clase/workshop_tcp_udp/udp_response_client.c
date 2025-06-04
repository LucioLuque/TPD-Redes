/* Envio de datos sobre UDP y medicion del tiempo de respuesta */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* necesario para memset() */
#include <errno.h>  /* necesario para codigos de errores */
#include <netinet/in.h>
#include <netdb.h>  /* necesario para getaddrinfo() */
#include <unistd.h>
#include <sys/time.h> /* necesario para gettimeofday */
#include <arpa/inet.h> /* necesario para inet_ntop */


#define SERVER_PORT "1280" 
#define INET4_ADDRSTRLEN 16


int main(int argc, char *argv[])
{
  /* argc da la cantidad de argumentos pasados */
  if (argc < 2 )  /* Si no se pasan dos argumentos sale */
  {
    fprintf(stderr,"Uso: %s host\n",argv[0]);
    fflush(stderr);
    return 1;
  }

  int s; /*file descriptor*/
  struct addrinfo hints;
  struct addrinfo *servinfo; /* es donde van los resultados */
  int status;

  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_DGRAM;     /* Socket Type */
  if ( (status = getaddrinfo(argv[1], SERVER_PORT, &hints, &servinfo))!=0)
  {
    fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(status));
    return 1;
  }
  s = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  /* Variables para almacnar la dirección IP que se envia el datagram */
  struct sockaddr_in their_addr;
  socklen_t addrsize;
  char addrstr[INET4_ADDRSTRLEN];
  addrsize = sizeof(their_addr); /* very important:  Before the call, it should be initialized to the size of the buffer associated with src_addr. */

  /* Variables para el timestamp */
  struct timeval tv;
  double ts1, ts2;

  /* Buffer */
  int inbytes;
  char buffer[1000];
  memset(buffer,0,sizeof(buffer));

  gettimeofday(&tv,NULL); /* Timestamping */
  ts1 = tv.tv_sec + tv.tv_usec*1e-6; 
  /* Send */
  sendto(s,"1",1, 0 , servinfo->ai_addr, servinfo->ai_addrlen);
  /* Receive response in blocking mode*/
  inbytes = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *) &their_addr, &addrsize); // blocking
  gettimeofday(&tv,NULL); /* Timestamping */
  ts2 = tv.tv_sec + tv.tv_usec*1e-6;
  if (inbytes == -1) {
    fprintf(stderr, "Error in recvfrom (Error %d: %s)\n", errno, gai_strerror(errno));
  }
  else {
    memset(addrstr,0, sizeof addrstr);
    inet_ntop(AF_INET, &(their_addr.sin_addr),  addrstr, sizeof addrstr);
    fprintf(stderr, "Received response datagram FROM %s port %d size=%d in %.6f seconds\n", addrstr, ntohs(their_addr.sin_port), inbytes, ts2-ts1);
  }
  close(s);
}
