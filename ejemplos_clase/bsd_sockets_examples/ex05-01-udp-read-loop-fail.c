/* Envio de datos sobre UDP usando connect y send + loop de lectura (queda trabado)*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* necesario para memset() */
#include <errno.h>  /* necesario para codigos de errores */
#include <netinet/in.h>
#include <netdb.h>  /* necesario para getaddrinfo() */
#include <unistd.h>
int main(int argc, char *argv[])
{
  /* argc da la cantidad de argumentos pasados */
  if (argc < 3 )  /* Si no se pasan dos argumentos sale */
  {
    fprintf(stderr,"Uso: %s host \"string\"\n",argv[0]);
    fflush(stderr);
    return 1;
  }

  int s; /*file descriptor*/
  struct addrinfo hints;
  struct addrinfo *servinfo; /* es donde van los resultados */
  int status;
  int inbytes=-1;
  char buffer[3000];

  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_DGRAM;     /* Socket Type */
  if ( (status = getaddrinfo(argv[1],"80", &hints, &servinfo))!=0)
  {
    fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(status));
    return 1;
  }
  s = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  switch (connect(s, servinfo->ai_addr, servinfo->ai_addrlen))
  {
    case -1: 
      fprintf(stderr,"Error %i: %s \n",errno,strerror(errno));
      return 1;
      break;
    case 0:  
      printf("Esta conectado ( ^c para salir)\n\n");
      break;
  }
  send(s,argv[2],strlen(argv[2]), 0);
  send(s,"\r\n\r\n",4, 0);

  /* loop de lectura */
  while (1)
  {
    fprintf(stderr,"socket descriptor: %d inbytes=%d\n",s,inbytes);
    if (inbytes==0) /* cuando read devuelve 0 implica EOF */
    {
      close(s);
      return 1;
    }
    memset(buffer,0,sizeof(buffer));
    inbytes = recv(s,buffer,sizeof(buffer),0);
    printf("%s\n",buffer);
  }
  close(s);
}
