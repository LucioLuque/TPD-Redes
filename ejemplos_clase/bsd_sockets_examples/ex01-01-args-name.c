/* Manejo de argumentos */
#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[])
{
  if(strcmp(argv[0],"./mi_hack")==0){
	printf("Sos un hacker\n");
	return 0;
	}

  /* argc da la cantidad de argumentos pasados */
  if (argc < 2 )  /* Si no se pasan dos argumentos sale */    
  {       
    printf("Debe ingresar al menos 1 parametro\n");
    fflush(stdout);
    return 1;
  }
  else
  {
    int i=0;
    for ( i=0; i<argc; i++)
      printf("El argumento nro %d es \"%s\"\n",i,argv[i]);
  } 
  return 0; 
}
