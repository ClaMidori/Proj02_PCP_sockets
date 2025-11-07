# Proj02_PCP_sockets
Como segundo projeto para a disciplina de Programação Concorrente e Paralela, será implementado uma "renderização falsa de imagens" em um sistema cliente-servidor, cuja comunicação será feita usando sockets.

## Para compilar o projeto

```bash
make all      # Compila tudo
make servidor # Compila apenas o servidor
make cliente  # Compila apenas o cliente
make gerador  # Compila apenas o gerador de imagem
make clean    # Remove os arquivos compilados
```

### Gerando a imagem original
No mesmo terminal, rode o gerador (só precisa fazer isso uma vez):

```bash
./gera
```
Isso criará o arquivo `imagem_original.txt`.

### Iniciando o servidor
O servidor precisa saber a porta e quantos clientes ele deve esperar.

```bash
# Exemplo: ./servidor <porta> <grid_rows> <grid_cols>
./servidor 8080 2 2
```

O servidor vai iniciar e ficará travado, esperando 4 clientes se conectarem (de acordo com o exemplo)

### Iniciando os clientes
Em outro terminal iremos iniciar os clientes:

```bash
# Exemplo: ./cliente <ip_servidor> <porta>
./cliente 127.0.0.1 8080
./cliente 127.0.0.1 8080
./cliente 127.0.0.1 8080
./cliente 127.0.0.1 8080
```

## Resultados esperados

1.  O servidor (no Terminal 1) aceitará o Cliente 1 (Terminal 2), enviará o primeiro pedaço (linhas 0-499 + "halo"), receberá o resultado e o desconectará.
2.  Depois, ele aceitará o Cliente 2, enviará o segundo pedaço (linhas 500-999 + halos), receberá o resultado...
3.  Ele fará isso para os 4 clientes, *um de cada vez*.
4.  Quando o 4º cliente terminar, o servidor montará a imagem final e salvará em `imagem_renderizada.txt`.