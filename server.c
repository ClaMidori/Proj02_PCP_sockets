#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt =1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE]={0};

    // catch error
    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) ==0){
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // set socket options
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // setup address - IPv4
    address.sin_family = AF_INET;

    address.sin_addr.s_addr = INADDR_ANY; // aceita conexões de qualquer interface
    address.sin_port = htons(PORT); // set apara o PORT 8080

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind fails");
        exit(EXIT_FAILURE);
    }

    // escutar por conexões
    if(listen(server_fd,3)<0){
        perror("failed");
    }

    printf("server listening on port %d\n", PORT);

    //aceitar conexões
    if((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen))<0){
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("connection accepted");

    // read from client and print
    ssize_t valread;
    while((valread = read(new_socket, buffer, BUFFER_SIZE))>0){
        printf("Client %s", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    //close
    close(server_fd);
    return 0;

}