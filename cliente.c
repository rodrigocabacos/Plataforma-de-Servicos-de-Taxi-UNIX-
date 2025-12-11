// Cliente
#include "settings.h"

// Variaveis Globais
int fd_cliente, fd_controlador, running = 1;
char namepipe[25];

dadosCliente cliente;
dadosVeiculo veiculo;

pthread_t tid[1];
TH td[1];

// CTRL C
void handler_sigalrm(int s, siginfo_t *i, void *v) {
    printf("\n[INFO] Recebi CTRL+C. A limpar e sair...\n");

    strcpy(cliente.msg, "terminar");

    int fd_controlador = open(CLIENTE_FIFO, O_WRONLY);
    
    if (fd_controlador != -1) {
        write(fd_controlador, &cliente, sizeof(dadosCliente));
        close(fd_controlador);
    }

    if (running) {
        running = 0;
        td[0].continuar = 0;
    }
    
    unlink(namepipe);
    unlink(cliente.fifo);

    printf("\nCliente terminado. Adeus!\n");
    
    exit(0);
}

// RECEBE MENSAGEM DO CONTROLADOR
void *AtendeControlador(void *tha) {
    TH *ptd = (TH *)tha;
    ssize_t size;
    Tipo tipo;

    dadosCliente clienteAtual;
    dadosControlador controlador;
    ListaViagens listaViagens;

    mkfifo(ptd->fifo, 0600);
    fd_controlador = open(ptd->fifo, O_RDWR );

    if (fd_controlador == -1) {
        perror("Erro ao abrir fd_controlador\n");
        pthread_exit(NULL);
    }

    while (ptd->continuar) {
        size = read(fd_controlador, &tipo, sizeof(Tipo));
        if (size == 0) {
            printf("Servidor desligou o pipe. A terminar...\n");
            td[0].continuar = 0;
            running = 0;
            break;
        }
        
        if (size == sizeof(tipo))
        {
            // Tipo Aviso
            if (tipo.tipo == 0) {
                if (read(fd_controlador, &clienteAtual, sizeof(dadosCliente)) > 0)
                    printf("[AVISO] %s\n", clienteAtual.aviso);
            }
            // Tipo Agendar
            else if (tipo.tipo == 1) {
                if (read(fd_controlador, &controlador, sizeof(dadosControlador)) > 0) {
                    Viagem *v = &controlador.cliente.viagens[0];

                    if (v->origem[0] != '\0') {
                        printf("[AVISO] Viagem [%d] registada --- Cliente: %s | Hora: %02dh:%02dm:%02ds | Origem: %s | Distancia: %dkm \n", 
                            v->id,
                            cliente.utilizador.nome,
                            v->hora / 3600, v->hora % 3600 / 60, v->hora % 60,
                            v->origem,
                            v->distancia
                        );
                    }
                }
            }
            // Tipo Cancelar
            else if (tipo.tipo == 2) {
                if(read(fd_controlador, &controlador, sizeof(dadosControlador)) > 0)
                {
                    printf("[AVISO] %s\n", controlador.msg);
                }
            }
            // Tipo Consultar
            else if (tipo.tipo == 3) {
                if (read(fd_controlador, &listaViagens, sizeof(ListaViagens)) > 0)
                {
                    printf("\n------ VIAGENS AGENDADAS ------\n");

                    int found = 0;
                    for (int i = 0; i < listaViagens.num_viagens; i++)
                    {
                        if (listaViagens.viagem[i].origem[0] != '\0')
                        {
                            int h = listaViagens.viagem[i].hora / 3600;
                            int m = (listaViagens.viagem[i].hora % 3600) / 60;
                            int s = listaViagens.viagem[i].hora % 60;

                            printf("Nome: %s | ID %d | Hora: %02dh:%02dm:%02ds | Origem: %s | Distancia: %d km \n",
                                listaViagens.viagem[i].utilizador.nome,
                                listaViagens.viagem[i].id,
                                h, m, s,
                                listaViagens.viagem[i].origem,
                                listaViagens.viagem[i].distancia
                            );

                            found = 1;
                        }
                    }

                    if (!found) printf("Não existem viagens agendadas.\n");
                    
                printf("--------------------------------\n\n");
                }
                else
                {
                    printf("ERRO: Falha ao ler fd_controlador\n");
                }
            }
            // Tipo Sair
            else if (tipo.tipo == 4) {
                if (read(fd_controlador, &controlador, sizeof(dadosControlador)) > 0) {
                    printf("%s\n", controlador.msg);
                    running = 0;
                    td[0].continuar = 0;
                    close(fd_controlador);
                    unlink(ptd->fifo);
                    exit(0);
                }
            }
            // Tipo Login
            else if (tipo.tipo == 5) {
                if (read(fd_controlador, &controlador, sizeof(dadosControlador)) > 0) {
                    if (controlador.entrar) {
                        cliente.entrar = 1;

                        printf("-Bem-vindo(a), %s!\n", cliente.utilizador.nome);

                        /*----------------------- TEXTO DE INICIO DE SESSÃO -----------------------*/
                        printf("---//---//---//---//- Bem vindo(a) ao Servidor! -//---//---//---//--- \n");
                        printf("- Insira 'agendar <hora> <local> <distancia> para agendar uma viagem.\n");
                        printf("- Insira 'cancelar' para cancelar uma viagem agendada.\n");
                        printf("- Insira 'consultar' para consultar todos os serviços agendados.\n");
                        printf("- Insira 'terminar' para terminar o programa.\n\n");

                        /*-------------------------------------------------------------------------*/
                    }
                    else {
                        cliente.entrar = 0;
                        printf("%s\n", controlador.msg);
                        running = 0;
                        td[0].continuar = 0;
                        close(fd_controlador);
                    }
                }
                else {
                    printf("ERRO A RECEBER LOGIN DO CLIENTE - TENTE OUTRA VEZ");
                }
            }
        }
        
    }
    close(fd_controlador);
    unlink(ptd->fifo);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    /*----------------------- Verificação -----------------------*/

    if (argc < 2) {
        printf("Argumentos Insuficientes. Insira '%s <nome_do_utilizador>'\n", argv[0]);
        return 0;
    }
    if (strlen(argv[1]) >= sizeof(cliente.utilizador.nome)) {
        printf("Nome de utilizador demasiado longo. Máximo permitido: %lu caracteres.\n", sizeof(cliente.utilizador.nome) - 1);
        return 1;
    }
    /*--------------------------------------------------------------*/

    /*----------------------- INICIALIZAÇÃO de VARIAVEIS e OUTROS -----------------------*/
    signal(SIGPIPE, SIG_IGN);
    setbuf(stdout, NULL);

    // Dados do cliente a 0 por segurança
    memset(&cliente, 0, sizeof(dadosCliente));

    cliente.utilizador.user_pid = getpid();
    sprintf(namepipe, CONTROL_FIFO, cliente.utilizador.user_pid);
    strcpy(cliente.fifo, namepipe);

    // Thread CTRL C
    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    if (mkfifo(cliente.fifo, 0600) == -1 && errno != EEXIST) {
        perror("Erro criar fifo servidor");
    }

    // Controla Threads
    td[0].continuar = 1;
    strcpy(td[0].fifo, namepipe);

    // Remover Lixo Antigo
    pthread_create(&tid[0], NULL, &AtendeControlador, (void *)&td[0]);

    usleep(100000);

    /*--------------------------------------------------------------*/

    /*----------------------- ABRE FIFO PARA ENVIAR MSG-----------------------*/
    fd_cliente = open(CLIENTE_FIFO, O_WRONLY);
    if (fd_cliente == -1)
    {
        perror("Erro abrir o cliente\n");
        td[0].continuar = 1;
        pthread_join(tid[0], NULL);
        unlink(cliente.fifo);
        return 2;
    }
    /*---------------------------------------------------------*/
    strcpy(cliente.utilizador.nome, argv[1]);

    cliente.entrar = 0; // ESTADO DO LOGIN (POR EFETUAR)
    
    strcpy(cliente.msg, "login");
    write(fd_cliente, &cliente, sizeof(dadosCliente));

    fflush(stdin);
    /*----------------------- ENVIO DE PEDIDOS AO CONTROLADOR -----------------------*/
    while (running)
    {
        if (cliente.entrar)
        {
            sleep(1);

            /*----------------------- LÊ A MENSSAGEM DO USUARIO -----------------------*/
            printf("Digite a mensagem a enviar (máximo %d caracteres):\n> ", 300);
            fgets(cliente.msg, sizeof(cliente.msg), stdin);

            // REMOVE '\n' DA STRING
            cliente.msg[strcspn(cliente.msg, "\n")] = '\0';
            /*-------------------------------------------------------------------------*/

            /*----------------------- VERIFICA A INTENÇÃO DO USUARIO -----------------------*/
            // CASO DESEJE TERMINAR "exit"
            if (strcmp(cliente.msg, "terminar") == 0) { 
                running = 0; 
            }
            /*-------------------------------------------------------------------------------*/

            /*----------------------- ENVIA A MENSAGEM PARA O SERVIDOR -----------------------*/
            write(fd_cliente, &cliente, sizeof(dadosCliente));
        }
        else
        {
        }
    }
    /*-------------------------------------------------------------------------------*/

    // ENCERRAMENTO DO PROGRAMA

    // Fecha pipe de envio
    close(fd_cliente);

    // Para a thread de escuta
    td[0].continuar = 0;
    
    pthread_join(tid[0], NULL);

    // Apaga o ficheiro pipe
    unlink(cliente.fifo);
    unlink(namepipe);

    printf("FIM.\n");
    return 0;
}