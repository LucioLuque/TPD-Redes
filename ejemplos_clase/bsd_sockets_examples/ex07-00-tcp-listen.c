/* Listening de un socket TCP y accept() */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* necesario para memset() */
#include <errno.h>  /* necesario para codigos de errores */
#include <netinet/in.h>
#include <netdb.h>  /* necesario para getaddrinfo() */
#include <unistd.h> /* necesario para close() */
int main(int argc, char *argv[])
{

  int s; /*file descriptor*/
  struct addrinfo hints;
  struct addrinfo *servinfo; /* es donde van los resultados */
  int status;

  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_STREAM;    /* Socket Type */
  hints.ai_flags = AI_PASSIVE;        /* Llena la IP por mi por favor */
  if ( (status = getaddrinfo(NULL,"1280", &hints, &servinfo))!=0)
  {
    fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(status));
    return 1;
  }
  /* Creación del listener socket */
  s = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  /* Binding del socket al puerto e IP configurados por getaddrinfo */
  if ( (status = bind(s, servinfo->ai_addr, servinfo->ai_addrlen)) )
  {
    fprintf(stderr, "Error en bind: %s\n",gai_strerror(status));
    return 1;
  }
  if ( (status = listen(s, 1)) ) //Soporta hasta 1 pedidos de conexion en cola
  {
    fprintf(stderr, "Error en listen: %s\n",gai_strerror(status));
    return 1;
  }
  int new_s;
  struct sockaddr their_addr;
  socklen_t addrsize;
  addrsize = sizeof their_addr;
  if ((new_s = accept(s, &their_addr, &addrsize))==-1)
  {
    fprintf(stderr, "Error en accept: %s\n",gai_strerror(status));
    return 1;
  }
  ssize_t result;
  result = send(new_s,"Bienvenido\n",strlen("Bienvenido\n"), MSG_NOSIGNAL);
  if (result==-1)
  {
    fprintf(stderr,"Error %i: %s %d \n",errno,strerror(errno),new_s);
    return 1;
  }

  sleep(20);
  close(s);
  close(new_s);
  sleep(1);
  return 0;
}
