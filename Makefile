# Makefile

# Compilador C
CC = gcc

# Flags de compilação:
# -Wall: Habilita todos os avisos (warning)
# -g:    Adiciona informações de debug (para usar com gdb)
# -o:    Especifica o nome do arquivo de saída
CFLAGS = -Wall -g

# Alvos (binários que queremos criar)
TARGETS = server client

# Regra 'all' (padrão): compila todos os alvos
all: $(TARGETS)

# Regra para criar o 'server'
server: server.c
	$(CC) $(CFLAGS) -o server server.c

# Regra para criar o 'client'
client: client.c
	$(CC) $(CFLAGS) -o client client.c

# Regra 'clean': remove os arquivos compilados
clean:
	rm -f $(TARGETS) *.o