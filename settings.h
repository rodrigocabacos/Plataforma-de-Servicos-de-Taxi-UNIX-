//HEAD
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>

// DEFINIÇÃO DE PIPES
#define CLIENTE_FIFO "CLIENTE_PIPE"
#define CONTROL_FIFO "CONTROL_PIPE_%d"

// MAX Globals 
#define NCLIENTES 30
#define NVEICULOS 10
#define NVIAGENS 20

pthread_mutex_t listaViagens_mutex = PTHREAD_MUTEX_INITIALIZER;

// DEFINIÇÃO DE ESTRUTURAS
typedef struct{
    int tipo;
} Tipo;

typedef struct
{
    char fifo[20];
    int continuar;
    pthread_t tid;
} TH;

typedef struct
{
    char opcao1[20];  
    char opcao2[20];
    char opcao3[20];      
    char opcao4[20];   
} opcao;
opcao op;

typedef struct{
    char nome[50];
    int user_pid;
} Utilizador;

typedef struct{
    int ocupado;
    int distanciaPercorrida;
    int concluido;
    int hora;
    int viagemAtual;
} Veiculo;

typedef struct{
    Utilizador utilizador;
    int id;
    int hora;
    int distancia;
    char origem[20];
    char destino[20];
    int viagemAtual;
} Viagem;

typedef struct
{   
    Utilizador utilizador;
    char msg[4096];
    int entrar;
    char aviso[100];
    char fifo[20];
    int dentroVeiculo;
    int contacto;
    Viagem viagens[NVIAGENS];
} dadosCliente;

typedef struct
{
    dadosCliente cliente;
    char msg[4096];                    
    int expulso;
    int entrar;
    int distanciaTotal;
} dadosControlador;

typedef struct
{
    Utilizador utilizador;
    int expulso;
    int entrar;
} dadosVeiculo;

typedef struct{
    Utilizador utilizador;
    int nr_users;
}ListaUsers;
ListaUsers listaUtilizadores[NCLIENTES];

typedef struct{
    Veiculo veiculo[NVEICULOS];
    int num_veiculos;
}ListaVeiculos;
ListaVeiculos listaVeiculos;

typedef struct{
    Viagem viagem[NVIAGENS];
    int num_viagens;
}ListaViagens;
ListaViagens listaViagens;
