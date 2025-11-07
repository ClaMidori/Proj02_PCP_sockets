#include <stdio.h>  
#include <stdlib.h> 
#include <time.h>   

#define NOME_ARQUIVO "imagem_original.txt"
#define NUM_LINHAS 2000
#define NUM_INTEIROS 2000
#define VALOR_MAX 1023 

int main() {
    FILE *arquivo;

    srand(time(NULL));

    arquivo = fopen(NOME_ARQUIVO, "w");

    if (arquivo == NULL) {
        printf("Erro: Não foi possível criar o arquivo %s\n", NOME_ARQUIVO);
        return 1;
    }

    printf("Iniciando a geração do arquivo \"%s\"...\n", NOME_ARQUIVO);

    for (int i = 0; i < NUM_LINHAS; i++) {
        
        for (int j = 0; j < NUM_INTEIROS; j++) {
            
            int num_aleatorio = rand() % (VALOR_MAX + 1);

            fprintf(arquivo, "%d", num_aleatorio);

            if (j < NUM_INTEIROS - 1) {
                fprintf(arquivo, " ");
            }
        }
        
        fprintf(arquivo, "\n");
    }

    fclose(arquivo);

    printf("Arquivo \"%s\" gerado com sucesso!\n", NOME_ARQUIVO);

    return 0;
}