// Veiculo
#include "settings.h"

// Variaveis Globais
int fd_cliente, fd_controlador, running = 1;
char namepipe[25];
dadosCliente cliente;
dadosControlador controlador;
dadosVeiculo veiculo;
pthread_t tid[1];
TH td[1];

// CTRL C
void handler_sigalrm(int s, siginfo_t *i, void *v) {
    td[0].continuar = 0;
    unlink(namepipe);
    printf("\nFeed terminado. Adeus!\n");
    exit(1);
}