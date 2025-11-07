#   make all      - Compila tudo (gerador, servidor, cliente)
#   make servidor - Compila apenas o servidor
#   make cliente  - Compila apenas o cliente
#   make gerador  - Compila apenas o gerador de imagem
#   make clean    - Remove os arquivos compilados

# Compilador C
CC = gcc

# Flags de compilação:
# -O2 = Otimização nível 2 (bom para performance)
# -Wall = Mostra todos os warnings
# -Wextra = Mostra warnings extras
# -std=c11 = Usa o padrão C de 2011
# -lm = Linka a biblioteca matemática (para 'round')
CFLAGS = -std=c11 -Wall -Wextra -O2 -lm

# Alvos
all: gerador servidor cliente

servidor: server.c
	$(CC) $(CFLAGS) -o servidor server.c $(CFLAGS)

cliente: client.c
	$(CC) $(CFLAGS) -o cliente client.c $(CFLAGS)

gerador: geraImagem.c
	$(CC) $(CFLAGS) -o gera geraImagem.c $(CFLAGS)

clean:
	rm -f servidor cliente gera *.o