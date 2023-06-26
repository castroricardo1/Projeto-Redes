#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_CLIENTS 10
#define MAX_ROOMS 5
#define MAX_NAME_LENGTH 20

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    int socket;
} Client;

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    int limit;
    int num_clients;
    Client* clients[MAX_CLIENTS];
} Room;

Room* rooms[MAX_ROOMS];
int num_rooms = 0;
int max_sd; // Valor máximo do descritor de arquivo (socket) para a chamada select()
fd_set read_fds; // Conjunto de descritores de arquivo para leitura

void send_message(int socket, const char* message) {
    write(socket, message, strlen(message));
}

void send_clients_list(int socket, Room* room) {
    char clients_list[1000];
    sprintf(clients_list, "===== Clientes Conectados Na Sala =====\n");
    for (int i = 0; i < room->num_clients; i++) {
        sprintf(clients_list + strlen(clients_list), "%s\n", room->clients[i]->name);
    }
    send_message(socket, clients_list);
}

Room* find_room_by_id(int room_id) {
    for (int i = 0; i < num_rooms; i++) {
        if (rooms[i]->id == room_id) {
            return rooms[i];
        }
    }
    return NULL;
}

Room* create_room(int room_id, const char* room_name, int limit) {
    if (num_rooms >= MAX_ROOMS) {
        return NULL;
    }

    Room* new_room = (Room*)malloc(sizeof(Room));
    new_room->id = room_id;
    strcpy(new_room->name, room_name);
    new_room->limit = limit;
    new_room->num_clients = 0;
    rooms[num_rooms] = new_room;
    num_rooms++;

    return new_room;
}

void remove_client_from_room(Client* client, Room* room) {
    for (int i = 0; i < room->num_clients; i++) {
        if (room->clients[i] == client) {
            for (int j = i; j < room->num_clients - 1; j++) {
                room->clients[j] = room->clients[j + 1];
            }
            room->num_clients--;
            break;
        }
    }
}

void handle_client_message(Client* client, const char* message) {
    Room* current_room = NULL;
    for (int i = 0; i < num_rooms; i++) {
        Room* room = rooms[i];
        for (int j = 0; j < room->num_clients; j++) {
            if (room->clients[j] == client) {
                current_room = room;
                break;
            }
        }
    }

    if (strncmp(message, "/listar", 7) == 0) {
        send_clients_list(client->socket, current_room);
    } else if (strncmp(message, "/sair", 5) == 0) {
        if (current_room != NULL) {
            remove_client_from_room(client, current_room);
            send_message(client->socket, "Você saiu da sala.\n");
        } else {
            send_message(client->socket, "Você não está em nenhuma sala.\n");
        }
    } else if (strncmp(message, "/trocar_sala", 12) == 0) {
        if (current_room != NULL) {
            remove_client_from_room(client, current_room);
            current_room = NULL;
        }

        int room_id = atoi(message + 13);
        if (room_id >= 0) {
            Room* new_room = find_room_by_id(room_id);
            if (new_room != NULL) {
                if (new_room->num_clients < new_room->limit) {
                    new_room->clients[new_room->num_clients++] = client;
                    current_room = new_room;
                    send_message(client->socket, "Você entrou na nova sala.\n");
                } else {
                    send_message(client->socket, "A sala está cheia.\n");
                }
            } else {
                send_message(client->socket, "Sala não encontrada.\n");
            }
        } else if (room_id == -1) {
            send_message(client->socket, "ID da sala não pode ser -1. Digite outro ID válido.\n");
        } else {
            send_message(client->socket, "ID da sala inválido. Digite um ID válido.\n");
        }
    } else {
        if (current_room != NULL) {
            for (int i = 0; i < current_room->num_clients; i++) {
                if (current_room->clients[i] != client) {
                    char formatted_message[1100];
                    sprintf(formatted_message, "[%s] => %s", client->name, message);
                    send_message(current_room->clients[i]->socket, formatted_message);
                }
            }
        } else {
            send_message(client->socket, "Você não está em nenhuma sala.\n");
        }
    }
}

void handle_new_connection(int server_socket) {
    int client_socket;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
    if (client_socket == -1) {
        perror("Erro ao aceitar a conexão");
        return;
    }

    printf("Cliente conectado\n");

    Client* client = (Client*)malloc(sizeof(Client));
    client->id = client_socket;
    memset(client->name, 0, MAX_NAME_LENGTH);
    client->socket = client_socket;

    send_message(client->socket, "Insira seu nome:\n");
    read(client->socket, client->name, MAX_NAME_LENGTH);

    send_message(client->socket, "Insira o ID da sala (-1 para criar uma nova):\n");
    char room_id_str[10];
    read(client->socket, room_id_str, sizeof(room_id_str));

    int room_id = atoi(room_id_str);
    Room* room = find_room_by_id(room_id);
    if (room != NULL) {
        if (room->num_clients < room->limit) {
            room->clients[room->num_clients++] = client;
            char enter_message[100];
            sprintf(enter_message, "[%s] entrou na sala.\n", client->name);
            for (int i = 0; i < room->num_clients; i++) {
                if (room->clients[i] != client) {
                    send_message(room->clients[i]->socket, enter_message);
                }
            }
            send_message(client->socket, "Você entrou na sala existente.\n");
        } else {
            send_message(client->socket, "A sala está cheia.\n");
            close(client->socket);
            free(client);
            return;
        }
    } else if (room_id == -1) {
        send_message(client->socket, "Insira o limite da sala:\n");
        char limit_str[10];
        read(client->socket, limit_str, sizeof(limit_str));
        int limit = atoi(limit_str);
        room = create_room(num_rooms + 1, "Nova Sala", limit);
        if (room != NULL) {
            room->clients[room->num_clients++] = client;
            send_message(client->socket, "Você entrou na nova sala.\n");
            send_message(client->socket, "ID da sala criada: ");
            char room_id_info[10];
            sprintf(room_id_info, "%d\n", room->id);
            send_message(client->socket, room_id_info);
        } else {
            send_message(client->socket, "Não foi possível criar a sala.\n");
            close(client->socket);
            free(client);
            return;
        }
    } else {
        send_message(client->socket, "Sala não encontrada.\n");
        close(client->socket);
        free(client);
        return;
    }

    // Adiciona o descritor de arquivo do novo cliente ao conjunto
    FD_SET(client_socket, &read_fds);

    // Atualiza o valor máximo do descritor de arquivo
    if (client_socket > max_sd)
        max_sd = client_socket;

    printf("Cliente adicionado à sala\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Uso: %s [IP] [PORTA]\n", argv[0]);
        return 1;
    }

    int server_socket, client_socket, max_clients = 0;
    int activity, i, valread, sd;
    struct sockaddr_in server_address;
    socklen_t client_address_len = sizeof(server_address);
    char buffer[1000];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Erro ao criar o socket");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Erro ao associar o socket ao endereço");
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Erro ao escutar por conexões");
        return 1;
    }

    printf("Servidor de bate-papo iniciado. Aguardando conexões...\n");

    FD_ZERO(&read_fds); // Limpa o conjunto de descritores de arquivo para leitura
    FD_SET(server_socket, &read_fds); // Adiciona o socket do servidor ao conjunto
    max_sd = server_socket; // Valor máximo do descritor de arquivo (socket)

    while (1) {
        fd_set temp_fds = read_fds; // Copia o conjunto de descritores de arquivo para leitura

        // Aguarda atividade em algum descritor de arquivo usando a chamada select()
        activity = select(max_sd + 1, &temp_fds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            printf("Erro ao aguardar por atividade de descritor de arquivo\n");
        }

        // Novo cliente se conectou
        if (FD_ISSET(server_socket, &temp_fds)) {
            handle_new_connection(server_socket);
        }

        // Verifica por atividade nos descritores de arquivo dos clientes
        for (i = 0; i < num_rooms; i++) {
            Room* room = rooms[i];
            for (int j = 0; j < room->num_clients; j++) {
                sd = room->clients[j]->socket;

                if (FD_ISSET(sd, &temp_fds)) {
                    valread = read(sd, buffer, sizeof(buffer));
                    if (valread == 0) {
                        // Cliente desconectou
                        struct sockaddr_in client_address;
                        socklen_t client_address_len = sizeof(client_address);
                        getpeername(sd, (struct sockaddr*)&client_address, &client_address_len);
                        printf("Cliente desconectado, endereço IP: %s, porta: %d\n",
                            inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                        // Fecha o descritor de arquivo e remove o cliente da sala
                        close(sd);
                        remove_client_from_room(room->clients[j], room);
                        free(room->clients[j]);

                        // Remove o descritor de arquivo do conjunto
                        FD_CLR(sd, &read_fds);

                        printf("Cliente removido da sala\n");
                    } else {
                        // Trata a mensagem do cliente
                        buffer[valread] = '\0';
                        handle_client_message(room->clients[j], buffer);
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
