/* Creamos y cerramos un socket */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
  int s; /*file descriptor*/
  s = socket( PF_INET, SOCK_STREAM,0);
  printf("El file descriptor vale %d\n",s);
  sleep(20);
  close(s);
  printf("Se cierra el socket con fd=%d\n",s);
  sleep(50);
  return 0;
}
