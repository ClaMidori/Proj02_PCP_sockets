/* client.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // Para inet_pton
// <netdb.h> seria necessário para resolver nomes (ex: "google.com")

#define PORT 8080       // Porta do servidor
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1" // IP do servidor (localhost)

// Função auxiliar para Lidar com erros
void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sockfd; // Descritor do socket do cliente
    struct sockaddr_in serv_addr; // Endereço do servidor
    char buffer[BUFFER_SIZE];
    int n;

    // 1. Criar o socket (IPv4, TCP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERRO ao abrir o socket");
    }

    // 2. Preparar a estrutura do endereço do servidor
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;       // Família (IPv4)
    serv_addr.sin_port = htons(PORT);     // Porta

    // Converte o IP de string para o formato binário
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        error("ERRO: Endereço IP inválido ou não suportado");
    }

    // 3. Conectar (Connect) ao servidor
    // Esta chamada é BLOQUEANTE: tenta se conectar ao endereço/porta
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERRO ao conectar");
    }

    printf("Conectado ao servidor %s!\n", SERVER_IP);

    // --- Início da comunicação ---

    // 4. Escrever (Write) dados para o servidor
    printf("Digite uma mensagem: ");
    memset(buffer, 0, BUFFER_SIZE);
    fgets(buffer, BUFFER_SIZE - 1, stdin); // Lê do teclado (stdin)

    // Remove o '\n' que o fgets adiciona
    buffer[strcspn(buffer, "\n")] = 0; 

    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
        error("ERRO ao escrever no socket");
    }

    // 5. Ler (Read) a resposta do servidor
    memset(buffer, 0, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n < 0) {
        error("ERRO ao ler do socket");
    }
    printf("Servidor diz: %s\n", buffer);

    // 6. Fechar o socket
    close(sockfd);
    
    printf("Conexão fechada.\n");
    return 0;
}