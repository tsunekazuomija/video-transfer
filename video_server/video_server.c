#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define FILENAME0 "./tmp/tick.jpg"
#define FILENAME1 "./tmp/tock.jpg"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        error("ERROR on binding");
    }

    listen(sockfd, 5);

    printf("Server is listening on port %d...\n", port);

    int newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
    if (newsockfd < 0) {
        error("ERROR on accept");
    }

    printf("Client connected\n");

    char *cmd_init = "fswebcam -r 640x480 --jpeg 85 ./tmp/tick.jpg";
    if (system(cmd_init) < 0) {
        printf("ERROR displaying image");
    }
    sleep(1);

    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    int mode = 0; // fnameの切り替え用
    char *fname0 = "./tmp/tick.jpg";
    char *fname1 = "./tmp/tock.jpg";

    while(1) {
        FILE *fp;
        char *cmd;
        if (mode == 0) {
            fp = fopen(fname0, "rb");
            if (fp == NULL) {
                // error("ERROR opening file");
                fclose(fp);
                continue;
            }
            cmd = "fswebcam -r 640x480 --jpeg 85 ./tmp/tock.jpg";
        } else {
            fp = fopen(fname1, "rb");
            if (fp == NULL) {
                // error("ERROR opening file");
                fclose(fp);
                continue;
            }
            cmd = "fswebcam -r 640x480 --jpeg 85 ./tmp/tick.jpg";
        }
        if (system(cmd) < 0) {
            printf("ERROR displaying image");
            // pass
        }

        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
            int num_send = (int) bytes_read;
            // 送信する予定のデータのバイト数を送信
            if (send(newsockfd, &num_send, sizeof(int), 0) < 0) {
                error("ERROR writing to socket");
            }
            printf("1\n");
            // 確認()
            int response;
            int read_res = recv(newsockfd, &response, sizeof(int), 0);
            if (read_res < 0) {
                error("ERROR reading from socket");
            }
            printf("2\n");
            if (response != num_send) {
                error("ERROR: data size mismatch");
            }
            printf("3\n");
            // バイナリデータを送信
            if (send(newsockfd, buffer, bytes_read, 0) < 0) {
                error("ERROR writing to socket");
            }
            printf("4\n");
            int send_success;
            int read_res2 = recv(newsockfd, &send_success, sizeof(int), 0);
            if (read_res2 < 0) {
                error("ERROR reading from socket");
            }
            printf("5\n");
            if (send_success != num_send) {
                error("ERROR: data size mismatch");
            }
            printf("6\n");
        }

        int num_send = 0;
        if (send(newsockfd, &num_send, sizeof(int), 0) < 0) {
            error("ERROR writing to socket");
        }
        int response;
        int read_res = recv(newsockfd, &response, sizeof(int), 0);
        if (read_res < 0) {
            error("ERROR reading from socket");
        }
        if (response != num_send) {
            error("ERROR: data size mismatch");
        }
        printf("File sent successfully\n");
        fclose(fp);

        if (mode == 0) {
            mode = 1;
        } else {
            mode = 0;
        }
    }
    printf("File closed\n");
    close(newsockfd);
    printf("Socket closed1\n");
    close(sockfd);
    printf("Socket closed2\n");

    return 0;
}