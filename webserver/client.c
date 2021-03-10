#include "socket.h"
#include "client.h"
#include "utils.h"
#include "../parselib/http_parse.h"
#define BUFF_LENGTH 1024

char* fgets_or_exit(char* buffer, int size, FILE* stream) {
    if(fgets(buffer, size, stream) == NULL) {
        exit(1);
    }
    return buffer;
}

void skip_headers(FILE* client) {
    char buff[BUFF_LENGTH];
    fgets_or_exit(buff, BUFF_LENGTH, client);
    while(!(buff[0] == '\n' || (buff[0] == '\r' && buff[1] == '\n'))) {
        fgets_or_exit(buff, BUFF_LENGTH, client);
    }
}

void send_status(FILE* client, int code, const char* reason_phrase) {
    fprintf(client, "HTTP/1.1 %d %s\r\n", code, reason_phrase);
}

void send_response(FILE* client, int code, const char* reason_phrase, const char* message_body, FILE* fichier) {
    send_status(client, code, reason_phrase);
    if(fichier != NULL) {
        fprintf(client, "Connection: close\r\nContent-Length: %d\n\r\n", get_file_size(fichier));
        fflush(client);

        if(copy(fichier, client) == -1) {
            perror("Erreur de transmission du fichier");
            exit(1);
        }
    }
    else {
        fprintf(client, "Connection: close\r\nContent-Length: %ld\n\r\n", strlen(message_body));
        fflush(client);
    }
}

void traitement_client(int socket_client, char* buff, FILE* client) {
    if(socket_client == -1) {
		perror("accept socket client");
    }

	printf("Connexion établie\n");

    http_request req;

    fgets_or_exit(buff, BUFF_LENGTH, client);
    int parse = parse_http_request(buff, &req);
    skip_headers(client);
    FILE* fichier = NULL;
    char* target;
    if(parse == -1) {
        if(req.method == HTTP_UNSUPPORTED) {
            send_response(client, 405, "Method Not Allowed", "Method Not Allowed\r\n", NULL);
        }
        else {
            send_response(client, 400, "Bad Request", "Bad Request\r\n", NULL);
        }
    } 
    else {
        target = rewrite_target(req.target);
        fichier = check_and_open(target, "./www/");
        if (fichier == NULL){
            send_response(client, 404, "Not Found", "Not Found\r\n", NULL);
            exit(1);
        }
        send_response(client, 200, "OK", NULL, fichier);
    }
    exit(0);
}
