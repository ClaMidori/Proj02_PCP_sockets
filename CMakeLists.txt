# Versão mínima do CMake necessária
cmake_minimum_required(VERSION 3.10)

# Define o nome do projeto e a linguagem (C)
project(SocketExample C)

# --- Targets (Alvos) ---
# Diz ao CMake para criar um executável chamado "server"
# a partir do arquivo fonte "server.c"
add_executable(server server.c)

# Diz ao CMake para criar um executável chamado "client"
# a partir do arquivo fonte "client.c"
add_executable(sckt server.c)


# --- Tratamento de Bibliotecas (O mais importante!) ---

# Em sistemas POSIX (Linux, macOS), as funções de socket
# geralmente estão na biblioteca C padrão (libc) e não
# precisam de linkagem especial.

# NO ENTANTO, no Windows, você DEVE linkar com a biblioteca Winsock (Ws2_32).
# O CMake torna isso fácil:
if(WIN32)
    # target_link_libraries é a forma moderna de adicionar libs
    # Isso diz: "Se estiver no Windows, adicione a lib ws2_32.lib
    # aos nossos executáveis 'server' e 'client'"
    target_link_libraries(server PRIVATE ws2_32.lib)
    target_link_libraries(client PRIVATE ws2_32.lib)
endif()

# [EXTRA] Se/Quando você for adicionar Threads (ex: pthreads)
# para um servidor concorrente, você faria assim:
#
# find_package(Threads REQUIRED)
# target_link_libraries(server PRIVATE Threads::Threads)
#
# O CMake encontra a 'pthread' no Linux ou a API de Threads do Windows
# automaticamente para você.