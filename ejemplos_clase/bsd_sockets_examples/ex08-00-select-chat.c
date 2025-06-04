/* Demostracion del uso de select para evitar bloqueo */ 
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* necesario para memset() */
#include <errno.h>  /* necesario para codigos de errores */
#include <netinet/in.h>
#include <netdb.h>  /* necesario para getaddrinfo() */
#include <unistd.h>
#include <arpa/inet.h>

#define INET4_ADDRSTRLEN 16

int main(int argc, char *argv[])
{

  int s_tcp; /*file descriptor*/
  struct addrinfo hints;
  struct addrinfo *servinfo; /* es donde van los resultados */
  int status;
  int inbytes=-1;
  char buffer[3000];

  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_STREAM;    /* Socket Type */
  hints.ai_flags = AI_PASSIVE;        /* Llena la IP por mi por favor */

  if ( (status = getaddrinfo(NULL,"1280", &hints, &servinfo))!=0)
  {
    fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(status));
    return 1;
  }
  /* Creacion del socket */
  s_tcp = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  /* Binding del socket al puerto e IP configurados por getaddrinfo */
  if ( (status = bind(s_tcp, servinfo->ai_addr, servinfo->ai_addrlen) ) )
  {
    fprintf(stderr, "Error en bind: %s\n", gai_strerror(status));
    return 1;
  }

  if ( (status = listen(s_tcp, 10)) ) /* Soporta hasta 10 pedidos de conexion en cola */
  {
    fprintf(stderr, "Error en listen: %s\n", gai_strerror(status));
    return 1;
  }
  int i;
  int s_chat[10]; /* array of sockets for chat clients */
  for ( i = 0; i < 10 ; i++)
    s_chat[i] = -1; /* initialization */  
  int n_client = 0;

  struct sockaddr_in their_addr;
  socklen_t addrsize;
  char addrstr[INET4_ADDRSTRLEN];

  addrsize = sizeof their_addr;


  /*variables del select*/
  struct timeval tv;
  fd_set rfds;
  int retval;
  /* configuro timeout en 500 mili segundos */
  int timeout_sec = 0;
  int timeout_usec = 500000;

  /* infinite loop of accepting clients and delivering messages */
  while (1) 
  {
    /* pre-select: es necesario reconfigurar esto antes de cada llamada a select */
    tv.tv_sec = timeout_sec;
    tv.tv_usec = timeout_usec;
    /* pre-select: borro el SET de file descriptors de lectura */
    FD_ZERO(&rfds);
    /* pre-select: agrego el file descriptor del socket s_tcp */
    FD_SET(s_tcp, &rfds);
    for (i = 0 ; i < 10 ; i++)
    {
      if (s_chat[i]!=-1) /* if it is a connected socket, added it to FDSET */
        FD_SET(s_chat[i], &rfds);
    }

    /* Chequeo si hay algun socket listo para lectura */
    retval = select((13+1), &rfds, NULL, NULL, &tv); /* do not HARDCODE like me ;) */

    // Me fijo si hubo algun socket listo
    if (retval >0)
    {
      // Me fijo si en el listener socket hay una conexion pendiente
      if (FD_ISSET(s_tcp, &rfds))
      {
        s_chat[n_client] = accept(s_tcp,  (struct sockaddr *)&their_addr, &addrsize);
        inet_ntop(AF_INET, &(their_addr.sin_addr),  addrstr, sizeof addrstr);
        printf("Received new connection FROM %s port %d\n", addrstr, their_addr.sin_port);
        printf("Client number %d\n", n_client);
        n_client++;
      }
      /* Itero sobre todos los file descriptors (chat clients) */
      for (i=0 ; i < 10 ; i++ )
      {
        if (FD_ISSET(s_chat[i], &rfds))
        {
          /* Leer el socket y copiarlo al resto de los sockets */
          memset(buffer, 0, sizeof(buffer));
          inbytes = recv(s_chat[i], buffer, sizeof(buffer), 0);
          /*printf("%s\n",buffer); */
          printf("[%d] %d bytes\n",i, inbytes); 
	  if (inbytes == 0) {
            printf("Client number %d closed connection\n", i);
            close(s_chat[i]);
            s_chat[i] = -1;
          }
          
          if (inbytes > 0) { 
            int j;
            for (j=0; j <10 ; j++)
            {
              if (i !=j && s_chat[j]!= -1) {
                char  message[10000];
                sprintf(message, "[%d] %s", i, buffer); 
                retval = write(s_chat[j], message, strlen(message));
                if (retval == -1) {
                  fprintf(stderr, "Error writing to client %d (Error %d: %s)\n", j, errno, gai_strerror(errno));
                  s_chat[j] = -1; 
                }
              }
            }
          } 
        }
      }
    }
    else {
      printf("No file descriptor ready in %.3f seconds\n", timeout_sec+timeout_usec/1e6); 
    }
  }

  return 0;
}
