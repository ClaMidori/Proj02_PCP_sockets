#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h> 

#define LINHAS 2000 
#define COLS 2000   

int **alocar_matriz(int n, int m) {
    int *dados = (int *)malloc(n * m * sizeof(int));
    if (dados == NULL) {
        return NULL;
    }
    int **matriz = (int **)malloc(n * sizeof(int *));
    if (matriz == NULL) {
        free(dados);
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        matriz[i] = &(dados[i * m]);
    }
    return matriz;
}

void liberar_matriz(int **matriz) {
    if (matriz != NULL) {
        free(matriz[0]); 
        free(matriz);   
    }
}

int enviar_todos(int sock, const void *buf, size_t n) {
    const char *ptr = (const char *)buf;
    size_t total_enviado = 0;
    while (total_enviado < n) {
        ssize_t enviado = send(sock, ptr + total_enviado, n - total_enviado, 0);
        if (enviado < 0) {
            perror("Erro ao enviar dados");
            return -1; // Erro
        }
        if (enviado == 0) {
            printf("Aviso: Conexão fechada pelo par durante o envio.\n");
            return 0; // Conexão fechada
        }
        total_enviado += enviado;
    }
    return 1; // Sucesso
}

int receber_todos(int sock, void *buf, size_t n) {
    char *ptr = (char *)buf;
    size_t total_recebido = 0;
    while (total_recebido < n) {
        ssize_t recebido = recv(sock, ptr + total_recebido, n - total_recebido, 0);
        if (recebido < 0) {
            perror("Erro ao receber dados");
            return -1; // Erro
        }
        if (recebido == 0) {
            printf("Aviso: Conexão fechada pelo par durante o recebimento.\n");
            return 0; // Conexão fechada
        }
        total_recebido += recebido;
    }
    return 1; // Sucesso
}

void aplicar_stencil_2d(int **matriz_entrada, int **matriz_saida,
                        int linhas_buffer, int cols_buffer,
                        int linhas_saida, int cols_saida,
                        int halo_cima, int halo_esq) {

    // Calcula os halos que faltavam (inferindo)
    int halo_baixo = linhas_buffer - linhas_saida - halo_cima;
    int halo_dir = cols_buffer - cols_saida - halo_esq;

    // Loop PELA MATRIZ DE SAÍDA (o chunk real)
    for (int i = 0; i < linhas_saida; i++) {
        for (int j = 0; j < cols_saida; j++) {

            // 'r' e 'c' são as coordenadas no buffer de ENTRADA
            int r = i + halo_cima;
            int c = j + halo_esq;

            // Determina se este pixel (r, c) está na borda TOTAL da imagem
            // Para isso, precisamos saber qual era a linha/coluna original
            // do chunk, o que não temos...
            
            // Correção: Vamos usar a informação dos halos!
            // Se halo_cima == 0, então i == 0 é a borda total.
            // Se halo_esq == 0, então j == 0 é a borda total.
            // Se halo_baixo == 0, então i == (linhas_saida - 1) é a borda total.
            // Se halo_dir == 0, então j == (cols_saida - 1) é a borda total.

            int borda_total_cima = (halo_cima == 0 && i == 0);
            int borda_total_baixo = (halo_baixo == 0 && i == (linhas_saida - 1));
            int borda_total_esq = (halo_esq == 0 && j == 0);
            int borda_total_dir = (halo_dir == 0 && j == (cols_saida - 1));

            // Se o pixel estiver em CIMA, BAIXO, ESQ ou DIR da imagem INTEIRA...
            if (borda_total_cima || borda_total_baixo || borda_total_esq || borda_total_dir) {
                // ...nós não aplicamos o stencil. Apenas copiamos o valor.
                // Isto previne o acesso a matriz_entrada[-1][j] (Segfault)
                matriz_saida[i][j] = matriz_entrada[r][c];

            } else {
                // É um pixel interior, 100% seguro aplicar o stencil.
                long long soma = (long long)matriz_entrada[r][c] +     // Centro
                                 matriz_entrada[r - 1][c] + // Cima
                                 matriz_entrada[r + 1][c] + // Baixo
                                 matriz_entrada[r][c - 1] + // Esquerda
                                 matriz_entrada[r][c + 1];  // Direita
                
                // Usamos round() para a média
                matriz_saida[i][j] = (int)round(soma / 5.0);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ip_servidor> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip_servidor = argv[1];
    int porta = atoi(argv[2]);

    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Criar o socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porta);

    // Converte o endereço IP de string para binário
    if (inet_pton(AF_INET, ip_servidor, &serv_addr.sin_addr) <= 0) {
        perror("Endereço IP inválido ou não suportado");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 2. Conectar ao servidor
    printf("Conectando ao servidor %s:%d...\n", ip_servidor, porta);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro na conexão");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Conectado!\n");

    // 3. Receber as dimensões do chunk
    // O servidor envia 6 inteiros
    int dimensoes[6]; // {linhas_buffer, cols_buffer, linhas_saida, cols_saida, halo_cima, halo_esq}
    if (receber_todos(sock, dimensoes, 6 * sizeof(int)) <= 0) {
        fprintf(stderr, "Falha ao receber dimensões do chunk.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int linhas_buffer = dimensoes[0];
    int cols_buffer = dimensoes[1];
    int linhas_saida = dimensoes[2];
    int cols_saida = dimensoes[3];
    int halo_cima = dimensoes[4];
    int halo_esq = dimensoes[5];

    printf("Recebido chunk para processar (Buffer: %dx%d, Saída: %dx%d)\n",
           linhas_buffer, cols_buffer, linhas_saida, cols_saida);

    // 4. Alocar matrizes
    int **matriz_entrada = alocar_matriz(linhas_buffer, cols_buffer);
    int **matriz_saida = alocar_matriz(linhas_saida, cols_saida);

    if (matriz_entrada == NULL || matriz_saida == NULL) {
        fprintf(stderr, "Erro: Falha ao alocar matrizes.\n");
        close(sock);
        liberar_matriz(matriz_entrada);
        liberar_matriz(matriz_saida);
        exit(EXIT_FAILURE);
    }

    // 5. Receber os dados da imagem (o chunk com halos)
    size_t bytes_receber = (size_t)linhas_buffer * cols_buffer * sizeof(int);
    printf("Recebendo dados do chunk... (%zu bytes)\n", bytes_receber);
    
    if (receber_todos(sock, matriz_entrada[0], bytes_receber) <= 0) {
        fprintf(stderr, "Falha ao receber dados da imagem.\n");
        close(sock);
        liberar_matriz(matriz_entrada);
        liberar_matriz(matriz_saida);
        exit(EXIT_FAILURE);
    }

    // 6. Processar o stencil
    printf("Dados recebidos. Processando stencil...\n");
    aplicar_stencil_2d(matriz_entrada, matriz_saida,
                       linhas_buffer, cols_buffer,
                       linhas_saida, cols_saida,
                       halo_cima, halo_esq);
    printf("Processamento concluído.\n");

    // 7. Enviar o resultado (matriz_saida) de volta
    size_t bytes_enviar = (size_t)linhas_saida * cols_saida * sizeof(int);
    printf("Enviando resultado... (%zu bytes)\n", bytes_enviar);
    if (enviar_todos(sock, matriz_saida[0], bytes_enviar) <= 0) {
        fprintf(stderr, "Falha ao enviar resultado.\n");
    }

    // 8. Limpeza
    printf("Trabalho concluído. Desconectando.\n");
    liberar_matriz(matriz_entrada);
    liberar_matriz(matriz_saida);
    close(sock);

    return 0;
}