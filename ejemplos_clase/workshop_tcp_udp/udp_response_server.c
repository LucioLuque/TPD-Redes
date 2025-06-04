/* Programa para demostrar en base a qué información ocurre el demultiplexado de informacion en UDP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* necesario para memset() */
#include <errno.h>  /* necesario para codigos de errores */
#include <netinet/in.h>
#include <netdb.h>  /* necesario para getaddrinfo() */
#include <unistd.h>
#include <arpa/inet.h> /* necesario para inet_ntop */


#define SERVER_PORT "1280" 
#define INET4_ADDRSTRLEN 16

int main(int argc, char *argv[])
{

  int s; /*file descriptor*/
  struct addrinfo hints;
  struct addrinfo *servinfo; /* es donde van los resultados */
  int status;
  int inbytes=-1;
  char buffer[3000];

  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_DGRAM;     /* Socket Type */
  hints.ai_flags = AI_PASSIVE;        /* Llena la IP por mi por favor */

  if ( (status = getaddrinfo(argv[1], SERVER_PORT, &hints, &servinfo))!=0)
  {
    fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(status));
    return 1;
  }
  s = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  /* Creacion del socket */
  s = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  /* Binding del socket al puerto e IP configurados por getaddrinfo */
  if ( (status = bind(s, servinfo->ai_addr, servinfo->ai_addrlen)) )
  {
    fprintf(stderr, "Error en bind (%d): %s\n",status, gai_strerror(status));
    return 1;
  }

  /* Variables para almacnar la dirección IP que se envia el datagram */
  struct sockaddr_in their_addr;
  socklen_t addrsize;
  char addrstr[INET4_ADDRSTRLEN];


  /* loop de lectura */
  while (1)
  {
    if (inbytes==0) /* cuando read devuelve 0 implica EOF */
    {
      close(s);
      return 1;
    }
    memset(buffer,0,sizeof(buffer));
    memset(&their_addr, 0, sizeof(their_addr));
    // inbytes = recv(s,buffer,sizeof(buffer),0);
    inbytes = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *) &their_addr, &addrsize);
    // send a response as fast as posible
    // sleep(1);
    sendto(s,"A",1, 0 , (struct sockaddr *)&their_addr, addrsize); 
  
    inet_ntop(AF_INET, &(their_addr.sin_addr),  addrstr, sizeof addrstr);
    fprintf(stderr, "Received datagram FROM %s port %d size=%d socketfd=%d\n", addrstr, ntohs(their_addr.sin_port), inbytes, s);
    printf("[%s]\n",buffer);
  }
  close(s);

  return 0; // main
}
