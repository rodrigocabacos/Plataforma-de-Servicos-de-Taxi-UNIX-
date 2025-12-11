// CONTROLADOR
#include "settings.h"

// Variaveis Globais
int running = 1;
int segundos;
char namepipe[25];

opcao opt;

TH td[3];
pthread_t tid[3];

// CRTL C
void handler_sigalrm(int s, siginfo_t *i, void *v)
{
    printf("\n[SHUTDOWN] A iniciar encerramento de emergencia...");
    printf("\n[SHUTDOWN] A avisar clientes para saírem...\n");

    dadosControlador controlador;
    Tipo tipo;

    memset(&controlador, 0, sizeof(dadosControlador));
    strcpy(controlador.msg, "[SHUTDOWN] O Servidor foi encerrado pelo Administrador.");

    tipo.tipo = 4;  //TERMINAR

    for (int k = 0; k < NCLIENTES; k++) 
    {
        if (listaUtilizadores[k].utilizador.nome[0] != '\0') 
        {
            char fifo_destino[64];
            snprintf(fifo_destino, sizeof(fifo_destino), CONTROL_FIFO, listaUtilizadores[k].utilizador.user_pid);
            
            // Usamos O_NONBLOCK para o servidor não bloquear se um cliente já tiver morrido
            int fd_adeus = open(fifo_destino, O_WRONLY | O_NONBLOCK);
            
            if (fd_adeus != -1) {
                write(fd_adeus, &tipo, sizeof(Tipo));
                write(fd_adeus, &controlador, sizeof(dadosControlador));
                close(fd_adeus);
            }
        }
    }

    running = 0;

    td[0].continuar = 0;
    td[1].continuar = 0;
    td[2].continuar = 0;
    td[3].continuar = 0;

    // FORÇAR A PARAGEM DO TECLADO
    pthread_cancel(tid[1]);

    // DESPERTAR AS THREADS
    int fd_threads = open("CLIENTE_PIPE", O_WRONLY | O_NONBLOCK);
    if (fd_threads != -1) {
        dadosCliente dummy;
        write(fd_threads, &dummy, sizeof(dadosCliente));
        close(fd_threads);
    }

    unlink(CLIENTE_FIFO);
    printf("\nServidor terminado. Adeus CRTL C!\n");
    exit(0);
}

int removeUser(char *nomeUser)
{
    for (int i = 0; i < listaUtilizadores[i].nr_users; i++)
    {
        if (strcmp(nomeUser, listaUtilizadores[i].utilizador.nome) == 0)
        {
            listaUtilizadores[i].utilizador.nome[0] = '\0';
            listaUtilizadores->nr_users--;
            if (listaUtilizadores->nr_users < 0) listaUtilizadores->nr_users = 0;
            return 1;
        }
    }
    return 0;
}

int removeViagem(int *idViagem)
{
    for (int i = 0; i < listaViagens.num_viagens; i++)
    {
        if (*idViagem == listaViagens.viagem[i].id)
        {
            listaViagens.viagem[i].destino[0] = '\0';
            listaViagens.viagem[i].distancia = 0;
            listaViagens.viagem[i].hora = 0;

            listaViagens.num_viagens--;

            if (listaViagens.num_viagens < 0) listaViagens.num_viagens = 0;
            return 1;
        }
    }
    return 0;
}

// FUNÇÃO VERIFICACLIENTE para verificar e registrar utilizadores no servidor
int VerificaCliente(dadosCliente cliente)
{
    // Verifica se o utilizador já existe
    for (int i = 0; i < NCLIENTES; i++)
    {
        if (strcmp(cliente.utilizador.nome, listaUtilizadores[i].utilizador.nome) == 0)
        {
            return 0;
        }
    }

    // Adiciona o utilizador se houver espaço
    for (int i = 0; i < NCLIENTES; i++)
    {
        if (listaUtilizadores[i].utilizador.nome[0] == '\0')
        {
            listaUtilizadores->nr_users++;

            strcpy(listaUtilizadores[i].utilizador.nome, cliente.utilizador.nome);
            listaUtilizadores[i].utilizador.user_pid = cliente.utilizador.user_pid;
            return 1;
        }
    }
    return -1;
}

// FUNÇÃO EXTRAI PALAVRAS CHAVES
void extrairPalavras(char *cliente, opcao *op)
{
    int i = 0, j = 0;

    // Copiar a primeira palavra para op.opcao1
    while (cliente[i] != ' ' && cliente[i] != '\0')
    {
        op->opcao1[i] = cliente[i];
        i++;
    }
    op->opcao1[i] = '\0';

    while (cliente[i] == ' ') { i++; }

    // Copiar a segunda palavra para op.opcao2
    while (cliente[i] != ' ' && cliente[i] != '\0')
    {
        op->opcao2[j] = cliente[i];
        i++;
        j++;
    }
    op->opcao2[j] = '\0';

    while (cliente[i] == ' ') { i++; }
    j = 0;

    // Copiar a terceira palavra para op.opcao3
    while (cliente[i] != ' ' && cliente[i] != '\0')
    {
        op->opcao3[j] = cliente[i];
        i++;
        j++;
    }
    op->opcao3[j] = '\0';

    while (cliente[i] == ' ') { i++; }
    j = 0;

    // Copiar o restante para op.opcao4
    while (cliente[i] != '\0')
    {
        op->opcao4[j] = cliente[i];
        i++;
        j++;
    }
    op->opcao4[j] = '\0';
}

void extrairPalavras2(char *input)
{
    int i = 0, j = 0;

    // Copiar a primeira palavra para op.opcao1
    while (input[i] != ' ' && input[i] != '\0')
    {
        opt.opcao1[i] = input[i];
        i++;
    }
    opt.opcao1[i] = '\0';

    while (input[i] == ' ')
    {
        i++;
    }

    // Copiar a segunda palavra para op.opcao2
    while (input[i] != ' ' && input[i] != '\0')
    {
        opt.opcao2[j] = input[i];
        i++;
        j++;
    }
    opt.opcao2[j] = '\0';
}

// THREAD DE TECLADO PARA O CONTROLADOR
void *AtendeControlador(void *tha)
{
    TH *ptd = (TH *)tha;

    dadosControlador controlador;   // Substitui a global 'controlador'

    Tipo tipo;

    int fd_cliente, fd_controlador;

    char input[100];
    while (ptd->continuar)
    {
        printf("\n");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';
        extrairPalavras2(input);
        if (strcmp(input, "listar") == 0)
        {
            int found = 0;
            for (int i = 0; i < NVIAGENS; i++)
            {
                if (listaViagens.viagem[i].origem[0] != '\0')
                { 
                    printf("Viagem [%d] --- Cliente: %s | Hora: %02dh:%02dm:%02ds | Origem: %s | Destino: %s | Distancia: %d km \n",
                        listaViagens.viagem[i].id,
                        listaViagens.viagem[i].utilizador.nome,
                        listaViagens.viagem[i].hora / 3600, listaViagens.viagem[i].hora % 3600 / 60, listaViagens.viagem[i].hora % 60,
                        listaViagens.viagem[i].origem,
                        listaViagens.viagem[i].destino[0] ? listaViagens.viagem[i].destino : "(indefinido)",
                        listaViagens.viagem[i].distancia
                    );
                    found = 1;
                }
            }
            if (found == 0)
            {
                printf("\nNão existem viagens agendadas.\n");
            }
        }
        else if (strcmp(input, "utiliz") == 0)
        {
            int found = 0;
            for (int i = 0; i < NCLIENTES; i++)
            {
                if (listaUtilizadores[i].utilizador.nome[0] != '\0')
                {
                    printf("User %s\n", listaUtilizadores[i].utilizador.nome);
                    found = 1;
                }
            }
            if (found == 0)
            {
                printf("\nNão existem utilizadores conectados.\n");
            }
        }
        else if (strcmp(opt.opcao1, "frota") == 0)
        {
            /* int found = 0;
            for (int i = 0; i < listaViagens.num_viagens; i++)
            {
                if (listaViagens.viagem[i].destino[0] != '\0' && listaViagens.viagem[i].veiculo.distanciaPercorrida > 0)
                { 
                    printf("Viagem %s:\n", listaViagens.viagem[i].id);
                    printf("- Progresso da viagem (%): %d%\n", atoi(listaViagens.viagem[i].veiculo.distanciaPercorrida / atoi(listaViagens.viagem[i].distancia)));
                    found = 1;
                }
            }
            if (found == 0)
            {
                printf("\nNao existe nenhuma viagem a decorrer.\n");
            } */
        }
        else if (strcmp(opt.opcao1, "cancelar") == 0)
        {
            // APAGAR TODAS AS VIAGENS
            int found = 0;
            if (strcmp(opt.opcao2, "0") == 0) {
                for (int i = 0; i < listaViagens.num_viagens; i++)
                {
                    removeViagem(&listaViagens.viagem[i].id);
                    printf("Viagem com id \"%d\" cancelada\n", listaViagens.viagem[i].id);
                    found = 1;
                }
            }
            if (found == 0)
                printf("\nNão existe nenhuma viagem agendada.\n");

            // APAGAR A VIAGEM COM O ID CORRETO
            for (int i = 0; i < listaViagens.num_viagens; i++)
            {
                if (listaViagens.viagem[i].id == atoi(opt.opcao2))
                { 
                    if(removeViagem(&listaViagens.viagem[i].id) != 0)
                        printf("Viagem com id \"%d\" cancelada\n", listaViagens.viagem[i].id);
                    else
                        printf("Viagem com id \"%d\" nao encontrada\n", listaViagens.viagem[i].id);
                }
            }
        }
        else if (strcmp(opt.opcao1, "km") == 0)
        {
            printf("Total de km percorridos por todos os veiculos: %d\n", controlador.distanciaTotal);
        }
        else if (strcmp(opt.opcao1, "hora") == 0)
        {
            printf("Horas do servidor atual: %02d:%02d:%02d\n", segundos / 3600, segundos / 60, segundos % 60);
        }
        else if (strcmp(input, "terminar") == 0)
        {
            td[0].continuar = 0;
            td[1].continuar = 0;
            td[2].continuar = 0;
            td[3].continuar = 0;

            char aux[10];
            fd_cliente = open(CLIENTE_FIFO, O_WRONLY);
            write(fd_cliente, &aux, sizeof(aux));
            for (int j = 0; j < NCLIENTES; j++)
            {
                if (listaUtilizadores[j].utilizador.nome[0] != '\0')
                {
                    char fifo_path[30];
                    snprintf(fifo_path, sizeof(fifo_path), CONTROL_FIFO, listaUtilizadores[j].utilizador.user_pid);

                    fd_controlador = open(fifo_path, O_WRONLY);
                    if (fd_controlador != -1)
                    {
                        tipo.tipo = 4;
                        strcpy(controlador.msg, "[SHUTDOWN] Controlador Encerrou.\n");

                        // Escreve os dados no FIFO
                        if (write(fd_controlador, &tipo, sizeof(Tipo)) == -1)
                        {
                            perror("[ERRO] Erro ao escrever tipo no FIFO.");
                        }
                        if (write(fd_controlador, &controlador, sizeof(dadosControlador)) == -1)
                        {
                            perror("[ERRO] Erro ao escrever controlador no FIFO.");
                        }
                        close(fd_controlador);
                    }
                }
            }
            close(fd_cliente);
        }
    }
    printf("Thread AtendeControlador a terminar...\n");
    pthread_exit(NULL);
}

// THREAD RECEBE PEDIDOS DO CLIENTE
void *AtendeCliente(void *tha)
{
    TH *ptd = (TH *)tha;
    size_t size;
    dadosCliente cliente;       // Substitui a global 'cliente'
    dadosControlador controlador;   // Substitui a global 'controlador'
    opcao op;                     // Local para esta thread
    Tipo tipo;

    int fd_cliente, fd_controlador;

    if (mkfifo(ptd->fifo, 0600) == -1 && errno != EEXIST) {
        perror("[ERRO] Erro ao criar fifo servidor.");
    }
    
    fd_cliente = open(ptd->fifo, O_RDWR);
    if (fd_cliente == -1) {
        perror("[ERRO] Erro fatal ao abrir pipe publico");
        ptd->continuar = 0;
        pthread_exit(NULL);
    }

    while (ptd->continuar)
    {
        size = read(fd_cliente, &cliente, sizeof(dadosCliente));

        if (size == -1) {
            perror("[ERRO] Falha no read"); // Vai dizer-te exatamente o que se passa (ex: Bad file descriptor)
            sleep(1); // Para não entupir o ecrã enquanto debugas
            break; // Sai do loop se der erro grave
        }
        
        if (size == sizeof(dadosCliente))
        {
            // Segurança da String
            cliente.msg[sizeof(cliente.msg) - 1] = '\0';

            // Extrai Palavras-Chave da mensagem
            extrairPalavras(cliente.msg, &op);

            printf("\n[PEDIDO] Cliente %d (%s) pede: %s, %s, %s, %s\n\n", 
                   cliente.utilizador.user_pid, 
                   cliente.utilizador.nome, 
                   op.opcao1, op.opcao2, op.opcao3, op.opcao4);

            // EFETUA LOGIN
            if (strcmp(op.opcao1, "login") == 0)
            {
                int verifica = VerificaCliente(cliente);
                char msgInicial[2048]; // Buffer grande para caber o menu todo
                int pos = 0;

                memset(&controlador, 0, sizeof(dadosControlador));

                if (verifica > 0)
                {
                    printf("- Utilizador [%s] de pid [%d] entrou.\n", cliente.utilizador.nome, cliente.utilizador.user_pid);
                    printf("- Quantidade de users conectados: [%d]\n\n", listaUtilizadores->nr_users);

                    controlador.entrar = 1;
                    tipo.tipo = 5;
                    pos += sprintf(msgInicial + pos, "-Bem-vindo(a), %s!\n", cliente.utilizador.nome);
                    pos += sprintf(msgInicial + pos, "---//---//---//---//- Bem vindo(a) ao Servidor! -//---//---//---//--- \n");
                    pos += sprintf(msgInicial + pos, "- Insira 'agendar <hora> <local> <distancia> para agendar uma viagem.\n");
                    pos += sprintf(msgInicial + pos, "- Insira 'cancelar <id>' para cancelar uma viagem agendada.\n");
                    pos += sprintf(msgInicial + pos, "- Insira 'consultar' para consultar todos os serviços agendados.\n");
                    pos += sprintf(msgInicial + pos, "- Insira 'terminar' para terminar o programa.\n\n");
                }
                else if (verifica == 0)
                {
                    controlador.entrar = 0;
                    tipo.tipo = 0;
                    pos += sprintf(msgInicial + pos, "[ERRO] Este utilizador já existe ou está online.\n");
                }
                else if (verifica == -1)
                {
                    controlador.entrar = 0;
                    tipo.tipo = 0;
                    pos += sprintf(msgInicial + pos, "[ERRO] Servidor cheio ou erro interno.\n");
                }
                else
                {
                    controlador.entrar = 0;
                    tipo.tipo = 0;
                    pos += sprintf(msgInicial + pos, "[ERRO] Erro ao entrar no servidor.\n");
                }
                strcpy(controlador.msg, msgInicial);
                fd_controlador = open(cliente.fifo, O_WRONLY);
                if (fd_controlador != -1)
                {
                    tipo.tipo = 5;
                    write(fd_controlador, &tipo, sizeof(Tipo));
                    write(fd_controlador, &controlador, sizeof(dadosControlador));
                    close(fd_controlador);
                }
            }
            // INSERIU AGENDAR
            if (strcmp(op.opcao1, "agendar") == 0)
            {
                int hora = atoi(op.opcao2);
                const char *origem = op.opcao3;
                int distancia = atoi(op.opcao4);

                printf("cliente [%d] acedeu a %s\n", cliente.utilizador.user_pid, op.opcao1);

                if (hora < 0 || origem[0] == '\0' || distancia <= 0 || listaViagens.num_viagens >= NVIAGENS) {
                    tipo.tipo = 0;
                    if (listaViagens.num_viagens >= NVIAGENS) 
                        strcpy(cliente.aviso, "[AVISO] Limite de viagens atingido.");

                    fd_controlador = open(cliente.fifo, O_WRONLY);
                    write(fd_controlador, &tipo, sizeof(Tipo));
                    write(fd_controlador, &cliente, sizeof(dadosCliente));
                    close(fd_controlador);
                    continue;
                }
                else
                {
                    strcpy(cliente.aviso, "[AVISO] Parametros invalidos para agendar uma viagem.");
                }

                pthread_mutex_lock(&listaViagens_mutex);

                int slot = -1;
                // procurar um slot livre
                for (int i = 0; i < NVIAGENS; i++) {
                    if (listaViagens.viagem[i].origem[0] == '\0') {
                        slot = i;
                        break;
                    }
                }
                
                // Se não houver buracos, tenta adicionar no fim
                if (slot == -1 && listaViagens.num_viagens < NVIAGENS) {
                    slot = listaViagens.num_viagens;
                }

                if (slot != -1) {
                    memset(&listaViagens.viagem[slot], 0, sizeof(Viagem));

                    listaViagens.viagem[slot].id = slot + 1;
                    listaViagens.viagem[slot].hora = hora;
                    listaViagens.viagem[slot].distancia = distancia;

                    strncpy(listaViagens.viagem[slot].origem, origem, sizeof(listaViagens.viagem[slot].origem) - 1);
                    strcpy(listaViagens.viagem[slot].utilizador.nome, cliente.utilizador.nome);

                    // destino pode ser definido mais tarde (por exemplo quando for atribuída a um veículo)
                    listaViagens.viagem[slot].destino[0] = '\0';

                    if (slot == listaViagens.num_viagens) {
                        listaViagens.num_viagens++;
                    }

                    // mensagem para os utilizadores
                    memset(&controlador, 0, sizeof(controlador));
                    controlador.cliente.viagens[0] = listaViagens.viagem[slot];
                }

                pthread_mutex_unlock(&listaViagens_mutex);

                // notificar todos os utilizadores conectados
                if (slot != -1) {
                    for (int i = 0; i < NCLIENTES; i++)
                    {
                        if (listaUtilizadores[i].utilizador.nome[0] != '\0' && listaUtilizadores[i].utilizador.user_pid != cliente.utilizador.user_pid)
                        {
                            char nomefifo[64];
                            int size = snprintf(nomefifo, sizeof(nomefifo), CONTROL_FIFO, listaUtilizadores[i].utilizador.user_pid);
                            if (size < 0 || size >= (int)sizeof(nomefifo)) continue; // nome demasiado grande — salta

                            fd_controlador = open(nomefifo, O_WRONLY | O_NONBLOCK);
                            if (fd_controlador != -1) { 
                                tipo.tipo = 1;
                                write(fd_controlador, &tipo, sizeof(Tipo));
                                write(fd_controlador, &controlador, sizeof(dadosControlador));
                                close(fd_controlador);
                            }
                        }
                    }

                    // imprimir resumo para o servidor
                    int h = hora / 3600;
                    int m = (hora % 3600) / 60;
                    int s = hora % 60;

                    printf("User [%s] agendou viagem às %02dh:%02dm:%02ds, origem [%s], %d km\n",
                        cliente.utilizador.nome, h, m, s, origem, distancia);
                    snprintf(controlador.msg, sizeof(controlador.msg), "[AVISO] Viagem agendada com sucesso.\nNome: %s | ID %d | Hora: %02dh:%02dm:%02ds | Origem: %s | Distancia: %d km \n",
                        listaViagens.viagem[slot].utilizador.nome,
                        listaViagens.viagem[slot].id,
                        h, m, s,
                        listaViagens.viagem[slot].origem,
                        listaViagens.viagem[slot].distancia
                    );

                    fd_controlador = open(cliente.fifo, O_WRONLY);
                    if (fd_controlador != -1) {

                        tipo.tipo = 1;
                        write(fd_controlador, &tipo, sizeof(Tipo));
                        write(fd_controlador, &controlador, sizeof(dadosControlador));
                        close(fd_controlador);
                    }
                }
                else
                {
                    printf("[ERRO] Lista de viagens cheia.");
                }
            }
            // INSERIU SUBSCRIBE
            else if (strcmp(op.opcao1, "cancelar") == 0)
            {
                int id = atoi(op.opcao2);

                printf("cliente [%d] acedeu a [%s] com id [%d]\n", cliente.utilizador.user_pid, op.opcao1, id);

                // validar id
                if (id < 0 || id >= NVIAGENS) {
                    tipo.tipo = 0;
                    strcpy(cliente.aviso, "[AVISO] ID de viagem inválido.");

                    fd_controlador = open(cliente.fifo, O_WRONLY);
                    write(fd_controlador, &tipo, sizeof(Tipo));
                    write(fd_controlador, &cliente, sizeof(dadosCliente));
                    close(fd_controlador);
                    continue;
                }
                pthread_mutex_lock(&listaViagens_mutex);

                for (int i = 0; i < NVIAGENS; i++)
            {
                if (listaViagens.viagem[i].destino[0] != '\0')
                { 
                    printf("Viagem %d:\n", listaViagens.viagem[i].id);
                    printf("- Hora: %02d:%02d:%02d\n", listaViagens.viagem[i].hora / 3600, listaViagens.viagem[i].hora % 3600 / 60, listaViagens.viagem[i].hora % 60);
                    printf("- Destino: %s\n", listaViagens.viagem[i].destino);
                    printf("- Distancia: %d\n", listaViagens.viagem[i].distancia);
                }
            }
                // existe?
                if (listaViagens.viagem[id - 1].origem[0] == '\0') {
                    pthread_mutex_unlock(&listaViagens_mutex);
                    tipo.tipo = 0;
                    strcpy(cliente.aviso, "[AVISO] Viagem não existe.");

                    fd_controlador = open(cliente.fifo, O_WRONLY);
                    write(fd_controlador, &tipo, sizeof(Tipo));
                    write(fd_controlador, &cliente, sizeof(dadosCliente));
                    close(fd_controlador);
                    continue;
                }
                // opcional: verificar se foi criada por este utilizador
                printf("cliente1: %s", listaViagens.viagem[id - 1].utilizador.nome);
                printf("cliente2: %s", cliente.utilizador.nome);
                if (strcmp(listaViagens.viagem[id - 1].utilizador.nome, cliente.utilizador.nome) != 0) {
                    pthread_mutex_unlock(&listaViagens_mutex);
                    tipo.tipo = 0;
                    strcpy(cliente.aviso, "[AVISO] So pode cancelar viagens que agendou.");

                    fd_controlador = open(cliente.fifo, O_WRONLY);
                    write(fd_controlador, &tipo, sizeof(Tipo));
                    write(fd_controlador, &cliente, sizeof(dadosCliente));
                    close(fd_controlador);
                    continue;
                }
                // limpar slot
                memset(&listaViagens.viagem[id - 1], 0, sizeof(Viagem));

                // atualizar contador
                // (opcional – depende do teu design)
                if (id == listaViagens.num_viagens - 1) {
                    while (listaViagens.num_viagens > 0 &&
                        listaViagens.viagem[listaViagens.num_viagens - 1].origem[0] == '\0')
                    {
                        listaViagens.num_viagens--;
                    }
                }
                pthread_mutex_unlock(&listaViagens_mutex);
                // preparar resposta ao cliente
                tipo.tipo = 2;
                memset(&controlador, 0, sizeof(controlador));
                snprintf(controlador.msg, sizeof(controlador.msg), "[AVISO] Viagem [%d] cancelada com sucesso.", id);

                // enviar resposta direta ao cliente
                fd_controlador = open(cliente.fifo, O_WRONLY);
                write(fd_controlador, &tipo, sizeof(Tipo));
                write(fd_controlador, &controlador, sizeof(dadosControlador));
                close(fd_controlador);

                printf("User [%s] cancelou viagem ID %d\n", cliente.utilizador.nome, id);
            }
            // INSERIU UNSUBSCRIBE
            else if (strcmp(op.opcao1, "consultar") == 0)
            {
                printf("cliente [%d] acedeu a consultar viagens\n", cliente.utilizador.user_pid);

                memset(&controlador, 0, sizeof(dadosControlador));
                pthread_mutex_lock(&listaViagens_mutex);

                char listaViagensCliente[4096];
                int pos = 0;

                pos += sprintf(listaViagensCliente + pos, "\n------ VIAGENS AGENDADAS ------\n");

                int found = 0;
                for (int i = 0; i < listaViagens.num_viagens; i++)
                {
                    if (listaViagens.viagem[i].origem[0] != '\0')
                    {
                        int h = listaViagens.viagem[i].hora / 3600;
                        int m = (listaViagens.viagem[i].hora % 3600) / 60;
                        int s = listaViagens.viagem[i].hora % 60;

                        pos += snprintf(listaViagensCliente + pos, sizeof(listaViagensCliente),
                            "Nome: %s | ID %d | Hora: %02dh:%02dm:%02ds | Origem: %s | Distancia: %d km \n",
                            listaViagens.viagem[i].utilizador.nome,
                            listaViagens.viagem[i].id,
                            h, m, s,
                            listaViagens.viagem[i].origem,
                            listaViagens.viagem[i].distancia
                        );

                        found = 1;
                    }
                }

                if (!found) pos += sprintf(listaViagensCliente + pos, "Não existem viagens agendadas.\n");

                pos += sprintf(listaViagensCliente + pos, "--------------------------------\n");

                strcpy(controlador.msg, listaViagensCliente);

                pthread_mutex_unlock(&listaViagens_mutex);
                
                // enviar toda a lista de viagens ao cliente
                fd_controlador = open(cliente.fifo, O_WRONLY);
                if (fd_controlador != -1) {
                    tipo.tipo = 3;
                    write(fd_controlador, &tipo, sizeof(Tipo));
                    write(fd_controlador, &controlador, sizeof(dadosControlador));
                    close(fd_controlador);
                }

                printf("Lista de viagens enviada ao cliente [%s]\n", cliente.utilizador.nome);
            }
            // INSERIU EXIT
            else if (strcmp(op.opcao1, "terminar") == 0)
            {
                printf("\nUtilizador [%s] de pid [%d] saiu.\n", cliente.utilizador.nome, cliente.utilizador.user_pid);
                printf("Quantidade de users conectados: [%d]\n\n", listaUtilizadores->nr_users);

                pthread_mutex_lock(&listaViagens_mutex);

                int viagens_removidas = 0;

                // 2. PERCORRER TODAS AS VIAGENS
                for (int i = 0; i < NVIAGENS; i++) 
                {
                    // Se a viagem existe (tem origem) E o nome pertence ao cliente que vai sair
                    if (listaViagens.viagem[i].origem[0] != '\0' && strcmp(listaViagens.viagem[i].utilizador.nome, cliente.utilizador.nome) == 0) 
                    {
                        memset(&listaViagens.viagem[i], 0, sizeof(Viagem));
                        viagens_removidas++;
                    }
                }

                // 3. ATUALIZAR CONTADOR
                while (listaViagens.num_viagens > 0 && listaViagens.viagem[listaViagens.num_viagens - 1].origem[0] == '\0') 
                {
                    listaViagens.num_viagens--;
                }

                pthread_mutex_unlock(&listaViagens_mutex);

                printf("[INFO] Foram canceladas %d viagens do utilizador %s.\n", viagens_removidas, cliente.utilizador.nome);

                removeUser(cliente.utilizador.nome);

                fd_controlador = open(cliente.fifo, O_WRONLY);
                if (fd_controlador != -1)
                {
                    tipo.tipo = 4;
                    memset(&controlador, 0, sizeof(dadosControlador));
                    strcpy(controlador.msg, "[AVISO] Sessao terminada. Todas as suas viagens foram canceladas.");
                    write(fd_controlador, &tipo, sizeof(Tipo));
                    write(fd_controlador, &controlador, sizeof(dadosControlador));
                    close(fd_controlador);
                }
            }
        }
    }
    close(fd_cliente);
    unlink(ptd->fifo);
    printf("Thread Atende cliente a terminar...\n");
    pthread_exit(NULL);
}

void *AtendeVeiculo(void *tha){
    TH *ptd = (TH *)tha;
    /* size_t size;
    dadosCliente cliente;       // Substitui a global 'cliente'
    dadosControlador controlador;   // Substitui a global 'controlador'
    opcao op;                     // Local para esta thread
    Tipo tipo;

    int fd_cliente, fd_controlador; */
    mkfifo(ptd->fifo, 0600);

    while (ptd->continuar){
        sleep(10000);
    }
    //close(fd_cliente);
    unlink(ptd->fifo);
    pthread_exit(NULL);
}

// THREAD TEMPORIZADOR
void *Temporizador(void *tha)
{
    TH *ptd = (TH *)tha;

    while (ptd->continuar)
    {
        sleep(1);
        segundos++;
    }
    printf("Thread Temporizador a terminar...\n");
    pthread_exit(NULL);
}

int main()
{
    /*----------------------- INICIALIZAÇÃO de VARIAVEIS e OUTROS -----------------------*/
    setbuf(stdout, NULL);

    // Inicializa variáveis
    memset(&listaUtilizadores, 0, sizeof(listaUtilizadores));
    memset(&listaViagens, 0, sizeof(listaViagens));
    memset(&listaVeiculos, 0, sizeof(listaVeiculos));

    // Configuração do handler para CTRL+C
    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Inicializa e cria threads
    td[0].continuar = 1;
    strcpy(td[0].fifo, "CLIENTE_PIPE");
    pthread_create(&tid[0], NULL, &AtendeCliente, (void *)&td[0]);

    td[1].continuar = 1;
    pthread_create(&tid[1], NULL, &AtendeControlador, (void *)&td[1]);

    td[2].continuar = 1;
    pthread_create(&tid[2], NULL, &Temporizador, (void *)&td[2]);

    /* td[3].continuar = 1;
    strcpy(td[0].fifo, "VEICULO_PIPE");
    pthread_create(&tid[3], NULL, &AtendeVeiculo, (void *)&td[3]); */

    /*----------------------- Manual do Controlador -----------------------*/
    printf("---//---//---//---//--- Olá Administrador! ---//---//---//---//--- \n");
    printf("- Insira 'listar' para obter uma lista dos serviços agendados.\n");
    printf("- Insira 'utiliz' para obter uma lista de utilizadores atualmente conectados.\n");
    printf("- Insira 'frota' para ver a percentagem total completa da viagem.\n");
    printf("- Insira 'cancelar <id>' para cancelar um serviço em progresso/agendado através do seu id.\n");
    printf("- Insira 'km' para mostrar o total de quilómetros percorridos por todos os veículos.\n");
    printf("- Insira 'hora' para mostrar a hora atual da Plataforma.\n");
    printf("- Insira 'terminar' para encerrar a Plataforma\n\n");
    /*-----------------------------------------------------------------*/
    printf("Servidor criado. A aguardar mensagens...\n");

    // ESPERA AS THREADS TERMINAREM
    pthread_join(tid[1], NULL);
    pthread_join(tid[0], NULL);
    pthread_join(tid[2], NULL);
    pthread_join(tid[3], NULL);

    printf("\nFIM\n");
    pthread_exit(NULL);
    return 0;
}