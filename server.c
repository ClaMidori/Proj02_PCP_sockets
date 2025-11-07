#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <arpa/inet.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <time.h>       
#include <math.h>       

#define LINHAS 2000
#define COLS 2000
#define ARQUIVO_ENTRADA "imagem_original.txt"
#define ARQUIVO_SAIDA "imagem_renderizada.txt"

int** alocar_matriz(int linhas, int cols) {
    int **matriz = (int **)malloc(linhas * sizeof(int *));
    if (matriz == NULL) {
        perror("Erro ao alocar ponteiros de linha");
        return NULL;
    }

    int *dados = (int *)malloc(linhas * cols * sizeof(int));
    if (dados == NULL) {
        perror("Erro ao alocar bloco de dados");
        free(matriz);
        return NULL;
    }

    // Aponta cada ponteiro de linha para o local correto no bloco de dados
    for (int i = 0; i < linhas; i++) {
        matriz[i] = dados + (i * cols);
    }
    return matriz;
}

void liberar_matriz(int **matriz) {
    if (matriz != NULL) {
        if (matriz[0] != NULL) {
            free(matriz[0]);
        }
        free(matriz);
    }
}

void carregar_imagem(int **matriz, const char* nome_arquivo) {
    FILE *f = fopen(nome_arquivo, "r");
    if (f == NULL) {
        perror("Erro ao abrir arquivo de entrada");
        exit(EXIT_FAILURE);
    }
    printf("Carregando %s...\n", nome_arquivo);
    for (int i = 0; i < LINHAS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (fscanf(f, "%d", &matriz[i][j]) != 1) {
                fprintf(stderr, "Erro ao ler dados da imagem.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    fclose(f);
}

void salvar_imagem(int **matriz, const char* nome_arquivo) {
    FILE *f = fopen(nome_arquivo, "w");
    if (f == NULL) {
        perror("Erro ao criar arquivo de saída");
        exit(EXIT_FAILURE);
    }
    printf("Salvando %s...\n", nome_arquivo);
    for (int i = 0; i < LINHAS; i++) {
        for (int j = 0; j < COLS; j++) {
            fprintf(f, "%d", matriz[i][j]);
            if (j < COLS - 1) {
                fprintf(f, " ");
            }
        }
        fprintf(f, "\n");
    }
    fclose(f);
    printf("Imagem salva.\n");
}

int enviar_todos(int socket, const void *buffer, size_t n) {
    const char *ptr = (const char *)buffer;
    ssize_t bytes_enviados;
    while (n > 0) {
        bytes_enviados = send(socket, ptr, n, 0);
        if (bytes_enviados <= 0) {
            perror("Erro no send");
            return -1; // Erro
        }
        ptr += bytes_enviados;
        n -= bytes_enviados;
    }
    return 0; // Sucesso
}

int receber_todos(int socket, void *buffer, size_t n) {
    char *ptr = (char *)buffer;
    ssize_t bytes_recebidos;
    while (n > 0) {
        bytes_recebidos = recv(socket, ptr, n, 0);
        if (bytes_recebidos <= 0) {
            if (bytes_recebidos == 0) {
                fprintf(stderr, "Cliente desconectou inesperadamente.\n");
            } else {
                perror("Erro no recv");
            }
            return -1; // Erro ou desconexão
        }
        ptr += bytes_recebidos;
        n -= bytes_recebidos;
    }
    return 0; // Sucesso
}

double calcular_delta(struct timespec *inicio, struct timespec *fim) {
    double delta_sec = (fim->tv_sec - inicio->tv_sec);
    double delta_nsec = (fim->tv_nsec - inicio->tv_nsec) / 1.0e9; // 1.0e9 = 1 bilhão
    return delta_sec + delta_nsec;
}

void enviar_chunk_2d(int sock_cliente, int grid_row, int grid_col, int grid_rows, int grid_cols, int** img_original) {
    int chunk_linhas = LINHAS / grid_rows;
    int chunk_cols = COLS / grid_cols;

    // Calcula os limites do chunk *deste* cliente
    int linha_inicio = grid_row * chunk_linhas;
    int col_inicio = grid_col * chunk_cols;

    // Determina o tamanho dos halos (0 ou 1)
    // Se o chunk está na borda, halo é 0 (não há vizinho)
    // Se está no meio, halo é 1
    int halo_cima = (grid_row == 0) ? 0 : 1;
    int halo_baixo = (grid_row == grid_rows - 1) ? 0 : 1;
    int halo_esq = (grid_col == 0) ? 0 : 1;
    int halo_dir = (grid_col == grid_cols - 1) ? 0 : 1;

    // Calcula o tamanho total do buffer a ser enviado
    int linhas_buffer = chunk_linhas + halo_cima + halo_baixo;
    int cols_buffer = chunk_cols + halo_esq + halo_dir;

    int dimensoes[6] = {
        linhas_buffer,  // 0: total de linhas no buffer (com halos)
        cols_buffer,   // 1: total de colunas no buffer (com halos)
        chunk_linhas,  // 2: linhas de SAÍDA (o chunk real)
        chunk_cols,    // 3: colunas de SAÍDA (o chunk real)
        halo_cima,     // 4: offset do halo superior
        halo_esq       // 5: offset do halo esquerdo
    };
    if (enviar_todos(sock_cliente, dimensoes, sizeof(dimensoes)) != 0) {
        fprintf(stderr, "Erro ao enviar dimensões para o cliente.\n");
        return;
    }

    int **buffer_envio = alocar_matriz(linhas_buffer, cols_buffer);
    if (buffer_envio == NULL) {
        fprintf(stderr, "Erro ao alocar buffer de envio.\n");
        return;
    }

    for (int i = 0; i < linhas_buffer; i++) {
        for (int j = 0; j < cols_buffer; j++) {
            // Calcula a posição (l, c) correspondente na imagem original
            int l = linha_inicio - halo_cima + i;
            int c = col_inicio - halo_esq + j;
            
            // Verificação de limites (segurança extra, embora a lógica dos halos deva prevenir)
            if (l >= 0 && l < LINHAS && c >= 0 && c < COLS) {
                buffer_envio[i][j] = img_original[l][c];
            } else {
                 buffer_envio[i][j] = 0; // Preenchimento (não deve acontecer)
            }
        }
    }

    // 4. Envia o buffer de dados (aproveitando a alocação contígua)
    size_t tamanho_buffer_bytes = (size_t)linhas_buffer * (size_t)cols_buffer * sizeof(int);
    printf("  Enviando chunk (%dx%d) para cliente... (%zu bytes)\n", linhas_buffer, cols_buffer, tamanho_buffer_bytes);
    if (enviar_todos(sock_cliente, buffer_envio[0], tamanho_buffer_bytes) != 0) {
        fprintf(stderr, "Erro ao enviar dados do chunk.\n");
    }

    liberar_matriz(buffer_envio);
}

void receber_chunk_2d(int sock_cliente, int grid_row, int grid_col, int grid_rows, int grid_cols, int** img_final) {
    int chunk_linhas = LINHAS / grid_rows;
    int chunk_cols = COLS / grid_cols;

    // Calcula os limites onde este chunk deve ser escrito na imagem final
    int linha_inicio = grid_row * chunk_linhas;
    int col_inicio = grid_col * chunk_cols;

    // 1. Aloca um buffer temporário para receber os dados
    //    O cliente só devolve o chunk processado (sem halos)
    int **buffer_receb = alocar_matriz(chunk_linhas, chunk_cols);
    if (buffer_receb == NULL) {
        fprintf(stderr, "Erro ao alocar buffer de recebimento.\n");
        return;
    }

    // 2. Recebe os dados
    size_t tamanho_buffer_bytes = (size_t)chunk_linhas * (size_t)chunk_cols * sizeof(int);
    printf("  Recebendo chunk processado (%dx%d)... (%zu bytes)\n", chunk_linhas, chunk_cols, tamanho_buffer_bytes);
    if (receber_todos(sock_cliente, buffer_receb[0], tamanho_buffer_bytes) != 0) {
        fprintf(stderr, "Erro ao receber dados processados.\n");
        liberar_matriz(buffer_receb);
        return;
    }

    // 3. Copia os dados do buffer para a imagem final
    for (int i = 0; i < chunk_linhas; i++) {
        for (int j = 0; j < chunk_cols; j++) {
            int l = linha_inicio + i;
            int c = col_inicio + j;

            // Medida de segurança: não sobrescrever as bordas da imagem final
            // O stencil só deve ser aplicado no "miolo" (1 a LINHAS-2, 1 a COLS-2)
            if (l > 0 && l < LINHAS - 1 && c > 0 && c < COLS - 1) {
                img_final[l][c] = buffer_receb[i][j];
            }
        }
    }

    liberar_matriz(buffer_receb);
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <porta> <grid_rows> <grid_cols>\n", argv[0]);
        fprintf(stderr, "Ex (1000x1000): %s 8080 2 2\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int porta = atoi(argv[1]);
    int grid_rows = atoi(argv[2]);
    int grid_cols = atoi(argv[3]);
    int num_clientes = grid_rows * grid_cols;

    if (num_clientes <= 0) {
         fprintf(stderr, "Erro: Grid deve ser pelo menos 1x1.\n");
         exit(EXIT_FAILURE);
    }
    if (LINHAS % grid_rows != 0 || COLS % grid_cols != 0) {
        fprintf(stderr, "Erro: As dimensões (2000x2000) devem ser divisíveis pelo grid (%dx%d).\n", grid_rows, grid_cols);
        exit(EXIT_FAILURE);
    }

    printf("Iniciando servidor com grid %dx%d (%d clientes).\n", grid_rows, grid_cols, num_clientes);

    double tempo_total_processamento = 0.0;

    // 1. Alocar e carregar a imagem original
    int **img_original = alocar_matriz(LINHAS, COLS);
    if (img_original == NULL) exit(EXIT_FAILURE);
    carregar_imagem(img_original, ARQUIVO_ENTRADA);

    // 2. Alocar a matriz final (e inicializar com a original, para manter as bordas)
    int **img_final = alocar_matriz(LINHAS, COLS);
    if (img_final == NULL) exit(EXIT_FAILURE);
    
    // Copia a imagem original para a final, para que as bordas (não processadas)
    // sejam preservadas.
    memcpy(img_final[0], img_original[0], (size_t)LINHAS * (size_t)COLS * sizeof(int));

    // 3. Configurar o socket do servidor
    int sock_servidor, sock_cliente;
    struct sockaddr_in end_servidor, end_cliente;
    socklen_t len_cliente;

    sock_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_servidor < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    
    // Permite reusar a porta imediatamente
    int opt = 1;
    if (setsockopt(sock_servidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
    }

    memset(&end_servidor, 0, sizeof(end_servidor));
    end_servidor.sin_family = AF_INET;
    end_servidor.sin_addr.s_addr = INADDR_ANY;
    end_servidor.sin_port = htons(porta);

    if (bind(sock_servidor, (struct sockaddr *)&end_servidor, sizeof(end_servidor)) < 0) {
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sock_servidor, num_clientes) < 0) {
        perror("Erro no listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor iniciado na porta %d. Aguardando %d clientes...\n", porta, num_clientes);

    // 4. Loop para aceitar e gerenciar cada cliente (Sequencialmente)
    int cliente_idx = 0;
    for (int r = 0; r < grid_rows; r++) {
        for (int c = 0; c < grid_cols; c++) {
            
            len_cliente = sizeof(end_cliente);
            sock_cliente = accept(sock_servidor, (struct sockaddr *)&end_cliente, &len_cliente);
            
            if (sock_cliente < 0) {
                perror("Erro no accept");
                c--; // Tenta aceitar este chunk novamente
                continue;
            }

            // --- INICIA CRONÔMETRO DO CLIENTE (APÓS ACCEPT) ---
            struct timespec inicio_cliente, fim_cliente;
            clock_gettime(CLOCK_MONOTONIC, &inicio_cliente);

            char ip_cliente[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &end_cliente.sin_addr, ip_cliente, INET_ADDRSTRLEN);
            printf("[Cliente %d/%d, Grid(%d,%d)] Conectado (IP: %s)\n", cliente_idx + 1, num_clientes, r, c, ip_cliente);

            // A. Envia o chunk de trabalho (com halos)
            enviar_chunk_2d(sock_cliente, r, c, grid_rows, grid_cols, img_original);

            // B. Recebe o chunk processado
            receber_chunk_2d(sock_cliente, r, c, grid_rows, grid_cols, img_final);

            close(sock_cliente);
            // --- FINALIZA CRONÔMETRO DO CLIENTE ---
            clock_gettime(CLOCK_MONOTONIC, &fim_cliente);
            double tempo_cliente = calcular_delta(&inicio_cliente, &fim_cliente);
            printf("[Cliente %d/%d, Grid(%d,%d)] Desconectado. (Tempo Gasto: %.8f s)\n", cliente_idx + 1, num_clientes, r, c, tempo_cliente);
            tempo_total_processamento += tempo_cliente; // Acumula o tempo

            cliente_idx++;
        }
    }

    // 5. Todos os clientes terminaram. Salvar a imagem final.
    printf("Renderização distribuída concluída.\n");

    // --- CRONÔMETRO PARA SALVAR IMAGEM ---
    struct timespec inicio_salvar, fim_salvar;
    clock_gettime(CLOCK_MONOTONIC, &inicio_salvar);
    salvar_imagem(img_final, ARQUIVO_SAIDA);

    clock_gettime(CLOCK_MONOTONIC, &fim_salvar);
    double tempo_salvar = calcular_delta(&inicio_salvar, &fim_salvar);
    tempo_total_processamento += tempo_salvar; // Adiciona o tempo de salvar

    printf("=========================================================\n");
    printf("Tempo de salvamento: %.8f segundos\n", tempo_salvar);
    printf("Tempo total de processamento (soma dos clientes + salvar): %.8f segundos\n", tempo_total_processamento);
    printf("=========================================================\n");

    // 6. Limpeza
    close(sock_servidor);
    liberar_matriz(img_original);
    liberar_matriz(img_final);

    return 0;
}