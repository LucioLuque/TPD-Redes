/* Demostraci�n de arquitectura de un server con multiprocess a.k.a. fork() */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* necesario para memset() */
#include <errno.h>  /* necesario para codigos de errores */
#include <netinet/in.h>
#include <netdb.h>  /* necesario para getaddrinfo() */
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h> /* necesario para wait() */
#include <signal.h>  /* necesario para sigaction */

#define INET4_ADDRSTRLEN 16

/* Signal handler that will reap zombie processes */
static void sigchld_handler(int s)
{
  int pid;
  while( (pid = waitpid(-1,NULL,WNOHANG)) > 0) {
    /* printf("Reaping zombie PID %d\n", pid); */
  }
}

int main(int argc, char *argv[])
{
  int s; /*file descriptor*/
  struct addrinfo hints;
  struct addrinfo *servinfo; /* es donde van los resultados */
  int status;

  /* set up signal handler that will reap zombie child process*/
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  memset(&hints,0,sizeof(hints));     /* es necesario inicializar con ceros */
  hints.ai_family = AF_INET;          /* Address Family */
  hints.ai_socktype = SOCK_STREAM;    /* Socket Type */
  hints.ai_flags = AI_PASSIVE;        /* Llena la IP por mi por favor */
  if ( (status = getaddrinfo(NULL,"1280", &hints, &servinfo))!=0)
  {
    fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(status));
    return 1;
  }
  /* Creaci�n del socket */
  s = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  /* set socket option in order to lose the pesky "Address already in use" error message */
  int yes=1;
  //char yes='1'; // Solaris people use this
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof yes) == -1) {
    perror("setsockopt");
    return 1;
  } 

  /* Binding del socket al puerto e IP configurados por getaddrinfo */
  if ( (status = bind(s, servinfo->ai_addr, servinfo->ai_addrlen)) )
  {
    fprintf(stderr, "Error en bind (%d): %s\n",status, gai_strerror(status));
    return 1;
  }
  if ( (status = listen(s, 10)) ) /* Supports up to 10 connection request on a backlog queue */
  {
    fprintf(stderr, "Error en listen (%d) : %s\n",status, gai_strerror(status));
    return 1;
  }
  int new_s;
  struct sockaddr_in their_addr;
  socklen_t addrsize;
  char addrstr[INET4_ADDRSTRLEN];
  
  addrsize = sizeof their_addr;
  while(1) 
  { /* main accept() loop */
    addrsize = sizeof their_addr;
    new_s = accept(s, (struct sockaddr *)&their_addr, &addrsize);
    if (new_s == -1) 
    {
      fprintf(stderr, "Error en accept: %s\n",gai_strerror(status));
      continue;
    }
    inet_ntop(AF_INET, &(their_addr.sin_addr),  addrstr, sizeof addrstr);
    printf("server: got connection from %s\n", addrstr);
    if (!fork()) 
    { /* this is the child process */
      close(s); /* child doesn't need the listener */
      if (send(new_s, "Hello, world!", 13, 0) == -1)
        perror("send");
      sleep(10);
      close(new_s);
      printf("server: exiting after fullfiling request\n");
      return 0;
    }
    close(new_s); /* parent doesn't need this */
  }
  close(s);
  return 0;
}
