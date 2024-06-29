#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define FILENAME0 "./tmp/tick.jpg" // 保存するファイル名
#define FILENAME1 "./tmp/tock.jpg" // 保存するファイル名

void error(const char *msg) {
    perror(msg);
    exit(1);
}

typedef struct {
    GtkWidget *image;
    char *current_file;
    int sockfd;
    int mode;
    int updating; // ファイルが更新中かどうかを示すフラグ
} ImageData;

gboolean update_image(gpointer data) {
    ImageData *img_data = (ImageData *)data;
    struct stat buffer;
    if (!img_data->updating && stat(img_data->current_file, &buffer) == 0) { // ファイルが存在し、更新中でないかチェック
        gtk_image_set_from_file(GTK_IMAGE(img_data->image), img_data->current_file);
    } else {
        printf("Failed to update image: file %s does not exist or is being updated.\n", img_data->current_file);
    }
    return FALSE;  // 一度だけ実行するためFALSEを返す
}

void *receive_files(void *arg) {
    ImageData *img_data = (ImageData *)arg;
    int sockfd = img_data->sockfd;
    int mode = img_data->mode;
    char *fname0 = FILENAME0;
    char *fname1 = FILENAME1;

    while (1) {
        FILE *fp;
        if (mode == 0) {
            fp = fopen(fname0, "wb");
            if (fp == NULL) {
                printf("ERROR opening file %s\n", fname0);
                continue;
            }
            img_data->current_file = fname0;
        } else {
            fp = fopen(fname1, "wb");
            if (fp == NULL) {
                printf("ERROR opening file %s\n", fname1);
                continue;
            }
            img_data->current_file = fname1;
        }

        img_data->updating = 1; // 更新中フラグを立てる

        ssize_t bytes_received;
        char buffer[BUFFER_SIZE];

        while (1) {
            int num_recv;
            if (recv(sockfd, &num_recv, sizeof(int), 0) < 0) {
                error("ERROR reading from socket");
            }
            send(sockfd, &num_recv, sizeof(int), 0);
            if (num_recv == 0) {
                break;
            }

            int received = 0;
            while (received < num_recv) {
                bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
                if (bytes_received < 0) {
                    error("ERROR receiving file");
                }
                fwrite(buffer, 1, bytes_received, fp);
                received += bytes_received;
            }
            send(sockfd, &received, sizeof(int), 0);
        }
        fclose(fp);

        img_data->updating = 0; // 更新中フラグを下げる
        g_idle_add(update_image, img_data);

        mode = 1 - mode;  // toggle mode
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        error("Invalid address or address not supported");
    }

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        error("ERROR connecting");
    }

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *image = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(window), image);

    ImageData img_data;
    img_data.image = image;
    img_data.sockfd = sockfd;
    img_data.mode = 0;
    img_data.updating = 0;

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, receive_files, &img_data);

    gtk_widget_show_all(window);

    gtk_main();

    close(sockfd);
    pthread_join(thread_id, NULL);

    return 0;
}
