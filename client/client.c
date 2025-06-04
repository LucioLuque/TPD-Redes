#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2 ) {
        printf("Debe ingresar al menos 1 parametro\n");
        fflush(stdout);
        return 1;
    } else {
        for (int i = 0; i < argc; i++)
            printf("El argumento nro %d es \"%s\"\n", i, argv[i]);
    }
    return 0;
}
