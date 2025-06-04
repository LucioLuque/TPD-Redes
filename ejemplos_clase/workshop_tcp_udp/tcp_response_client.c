/* Envio de datos sobre TCP y medicion del tiempo de respuesta */
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

  /* Variables para el timestamp */
  struct timeval tv;
  double ts1, ts2;

  /* Buffer */
  int inbytes;
  char buffer[1000];
  memset(buffer,0,sizeof(buffer));


  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_STREAM;    /* Socket Type */
  if ( (status = getaddrinfo(argv[1], SERVER_PORT, &hints, &servinfo))!=0) {
    fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(status));
    return 1;
  }
  s = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  memset(buffer,0,sizeof(buffer));

  gettimeofday(&tv,NULL); /* Timestamping */
  ts1 = tv.tv_sec + tv.tv_usec*1e-6; 
  if ( (status = connect(s, servinfo->ai_addr, servinfo->ai_addrlen))!=0) {
    fprintf(stderr, "Error en connect: %s\n",gai_strerror(status));
    return 1;
  }

  /* Send */
  sendto(s,"1",1, 0 , servinfo->ai_addr, servinfo->ai_addrlen);

  /* loop de lectura */
  while (1)
  {
    if (inbytes==0) /* cuando read devuelve 0 implica EOF */
    {
      close(s);
      return 1;
    }
    inbytes = recv(s,buffer,sizeof(buffer),0);
    gettimeofday(&tv,NULL); /* Timestamping */
    ts2 = tv.tv_sec + tv.tv_usec*1e-6;
    if (inbytes > 0) {
      fprintf(stderr, "Received response segment FROM %s port %s size=%d in %.6f seconds\n", argv[1], SERVER_PORT, inbytes, ts2-ts1);
      close(s);
      return 0;
    }
  }

  return 0;
}
