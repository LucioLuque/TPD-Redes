/* "Conexion" UDP, usamos getaddrinfo para configurar el socket*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> /* necesario par close() */
#include <string.h> /* necesario para memset() */
#include <errno.h>  /* necesario para codigos de errores */
#include <netinet/in.h>
#include <netdb.h>  /* necesario para getaddrinfo() */
int main(int argc, char *argv[])
{
  int s; /*file descriptor*/

  struct addrinfo hints;
  struct addrinfo *servinfo; /* es donde van los resultados */
  int status;

  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_DGRAM;     /* Socket Type */

  if (argc < 2) {
    printf("Uso: %s <IP>\n", argv[0]);
    return 1;
  }

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
  close(s);
  return 0;
}
