#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "server_helper.h"

#define PORT (8080)
#define BUF (1024)
#define MAX_CLIENTS (20)

typedef struct file file;
struct file f[256];
int nr_fis = 0;
int op = 0;
pthread_t tid[MAX_CLIENTS];
int clients_no = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int sign = 0;
void signal_handler(int signum)
{
    if (signum == SIGTERM || signum == SIGINT)
    {
        sign = 1;
        for (int i = 0; i < clients_no; i++)
        {
            pthread_join(tid[i], NULL);
        }
    }
}

void _send(int s, char *b)
{
    int n;
    int size = strlen(b);
    n = send(s, &size, sizeof(int), 0);

    n = send(s, b, size, 0);
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(1);
    }
}

void _list(int client_fd)
{
    _log(op, "");
    int sum = 0;
    if (nr_fis > 0)
    {
        for (int i = 0; i < nr_fis; i++)
        {
            sum += f[i].dim;
        }

        char *buffer = (char *)malloc(sum + nr_fis);
        strcpy(buffer, "");
        for (int i = 0; i < nr_fis; i++)
        {
            strcat(buffer, f[i].nume);
            strcat(buffer, "\\0");
        }
        printf("%s\n", buffer);

        char suma[20] = "";
        sprintf(suma, "%ld", strlen(buffer));
        char *rasp = (char *)malloc(strlen(buffer) + strlen(suma) + strlen("Success~"));
        strcpy(rasp, "");
        strcat(rasp, "Success~");
        strcat(rasp, suma);
        strcat(rasp, "~");
        strcat(rasp, buffer);
        strcat(rasp, "~");
        _send(client_fd, rasp);

        free(buffer);
        free(rasp);
    }
    else
    {
        char buffer[] = "Success~0~";
        _send(client_fd, buffer);
    }
}

void _get(char *p, int client_fd)
{
    _log(op, p);
    char buffer[1024] = "";
    int ok = 0;
    for (int i = 0; i < nr_fis; i++)
    {
        if (strcmp(p, f[i].nume) == 0)
        {
            pthread_mutex_lock(&mutex);
            while (f[i].write != 0)
            {
                pthread_cond_wait(&cond, &mutex);
            }

            f[i].read++;
            //sleep(10);
            int fd = open(p, O_RDONLY);
            if (fd < 0)
            {
                ok = 0;
                break;
            }

            int b = 0;
            int sum = 0;
            while (b = read(fd, buffer, BUF))
            {
                sum += b;
            }

            char suma[20];
            sprintf(suma, "%d", sum);

            lseek(fd, 0, SEEK_SET);

            char *buffer2 = (char *)malloc(sum);
            read(fd, buffer2, sum);

            char *rasp = (char *)malloc(strlen("Success~") + strlen(buffer2) + strlen(suma));
            strcpy(rasp, "");
            strcat(rasp, "Success~");
            strcat(rasp, suma);
            strcat(rasp, "~");
            strcat(rasp, buffer);
            strcat(rasp, "~");
            _send(client_fd, rasp);

            free(buffer2);
            free(rasp);
            ok++;

            f[i].read--;
            pthread_mutex_unlock(&mutex);
            pthread_cond_signal(&cond);
            break;
        }
    }

    if (ok == 0)
    {
        strcpy(buffer, "Fail~");
        _send(client_fd, buffer);
    }
}

void _put(char *b, int client_fd)
{
    char *p = strtok(b, " ~\n");
    f[nr_fis].dim_nume = atoi(p);
    p = strtok(NULL, " ~\n");
    f[nr_fis].nume = (char *)malloc(f[nr_fis].dim_nume);
    strcpy(f[nr_fis].nume, p);
    _log(op, p);
    char buffer[256];

    int fd = open(p, O_WRONLY | O_CREAT, 0664);
    if (fd == -1)
    {
        printf("Eroare la crearea fisierului.\n");
        return;
    }

    p = strtok(NULL, "~\n");
    f[nr_fis].dim = atoi(p);
    p = strtok(NULL, "~");

    int n = write(fd, p, f[nr_fis].dim);
    if (n == -1)
    {
        strcpy(buffer, "Fail~");
        _send(client_fd, buffer);
    }
    else
    {
        nr_fis++;
        strcpy(buffer, "Success~");
        _send(client_fd, buffer);
    }

    f[nr_fis].read = 0;
    f[nr_fis].write = 0;
    close(fd);
}

void _delete(char *p, int client_fd)
{
    int ok = -1;
    _log(op, p);
    for (int i = 0; i < nr_fis; i++)
    {
        if (strcmp(p, f[i].nume) == 0)
        {
            ok = i;
            break;
        }
    }

    if (nr_fis == 1 && ok > -1)
    {
        f[0].dim = 0;
        f[0].dim_nume = 0;
        strcpy(f[0].nume, "");
    }
    else if (nr_fis - 1 == ok && ok > -1)
    {
        f[ok].dim = 0;
        f[ok].dim_nume = 0;
        strcpy(f[ok].nume, "");
    }
    else if (ok > -1)
    {
        for (int i = ok; i < nr_fis - 1; i++)
        {
            f[i].dim = f[i + 1].dim;
            f[i].dim_nume = f[i + 1].dim_nume;
            strcpy(f[i].nume, f[i + 1].nume);
        }
    }

    char buffer[256];
    if (ok == -1)
    {
        strcpy(buffer, "Fail~");
        _send(client_fd, buffer);
    }
    else
    {
        strcpy(buffer, "Success~");
        _send(client_fd, buffer);
        nr_fis--;
    }
}

void _update(char *p, int client_fd)
{

    _log(op, p);
    char buffer[1024] = "";
    int ok = 0;
    for (int i = 0; i < nr_fis; i++)
    {
        if (strcmp(p, f[i].nume) == 0)
        {
            pthread_mutex_lock(&mutex);
            if (f[i].read != 0)
            {
                f[i].write++;
                pthread_cond_wait(&cond, &mutex);
            }
            else
            {
                f[i].write++;
            }

            //sleep(5);
            int fd = open(p, O_WRONLY);
            if (fd == -1)
            {
                ok = 0;
                break;
            }

            p = strtok(NULL, "~\n");
            int nr = atoi(p);
            lseek(fd, +nr, SEEK_SET);

            p = strtok(NULL, "~\n");
            nr = atoi(p);

            p = strtok(NULL, "~\n");
            write(fd, p, nr);

            ok++;
            close(fd);

            f[i].write--;
            pthread_mutex_unlock(&mutex);
            pthread_cond_signal(&cond);
        }
    }

    if (ok == 0)
    {
        strcpy(buffer, "Fail~");
        _send(client_fd, buffer);
    }
    else
    {
        strcpy(buffer, "Success~");
        _send(client_fd, buffer);
    }
}

int _find(char *c, char **buffer)
{
    for (int i = 0; i < nr_fis; i++)
    {
        char buf[1024] = "";
        int fd = open(f[i].nume, O_RDONLY);
        if (fd == -1)
        {
            return 0;
        }

        int b = 0;
        int sum = 0;
        int count = 0;
        while (b = read(fd, buf, BUF))
        {
            sum += b;
        }

        lseek(fd, 0, SEEK_SET);

        char *buffer2 = (char *)malloc(sum);
        read(fd, buffer2, sum);

        char *p = strtok(buffer2, " \n");
        while (p != NULL)
        {
            if (strcmp(p, c) == 0)
            {
                count++;
                break;
            }

            p = strtok(NULL, " \n");
        }

        if (count > 0)
        {
            strcat(*buffer, f[i].nume);
            strcat(*buffer, "\\0");
        }

        free(buffer2);
        close(fd);
    }

    printf("%s\n", *buffer);
    return 1;
}

void _search(char *p, int client_fd)
{
    char *buf = (char *)malloc(sizeof(char) * 1024);
    strcpy(buf, "");
    char buffer[1024] = "";
    int a = _find(p, &buf);
    if (a == 0)
    {
        strcpy(buffer, "Fail~");
        _send(client_fd, buffer);
    }
    else
    {
        strcpy(buffer, "Success~");
        strcat(buffer, buf);
        strcat(buffer, "~");
        _send(client_fd, buffer);
    }

    free(buf);
}

int parse_operation(int client_fd, char *buf)
{
    char *b = strtok(buf, " ~\n");

    if (strcmp(b, "LIST") == 0)
    {
        op = 1;
        _list(client_fd);
        return 1;
    }
    else if (strcmp(b, "GET") == 0)
    {
        op = 2;
        b = strtok(NULL, " ~\n");
        b = strtok(NULL, " ~\n");
        _get(b, client_fd);
        return 2;
    }
    else if (strcmp(b, "PUT") == 0)
    {
        op = 3;
        b = strtok(NULL, "\0");
        _put(b, client_fd);
        return 3;
    }
    else if (strcmp(b, "DELETE") == 0)
    {
        op = 4;
        b = strtok(NULL, " ~\n");
        b = strtok(NULL, " ~\n");
        _delete(b, client_fd);
        return 4;
    }
    else if (strcmp(b, "UPDATE") == 0)
    {
        op = 5;
        b = strtok(NULL, " ~\n");
        b = strtok(NULL, " ~\n");
        _update(b, client_fd);
        return 5;
    }
    else if (strcmp(b, "SEARCH") == 0)
    {
        op = 6;
        b = strtok(NULL, " ~\n");
        b = strtok(NULL, " ~\n");
        _search(b, client_fd);
        return 6;
    }
    else
    {
        return 0;
    }
}

void *handle_client(void *client_fd_ptr)
{
    int client_fd = *((int *)client_fd_ptr);
    char buffer[1024] = {0};
    int valread;

    valread = recv(client_fd, buffer, 1024, 0);

    op = parse_operation(client_fd, buffer);
    if (op == 0)
    {
        printf("Aceasta comanda nu este disponibila!\n");
        exit(-1);
    }

    close(client_fd);

    return NULL;
}

int main(int argc, char *argv[])
{
    int server_fd, client_fd[MAX_CLIENTS], valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct sigaction a;
    memset(&a, 0, sizeof(a));
    a.sa_handler = &signal_handler;
    sigaction(SIGTERM, &a, NULL);
    sigaction(SIGINT, &a, NULL);

    while (sign == 0)
    {
        if ((client_fd[clients_no] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            return 0;
        }

        if (pthread_create(&tid[clients_no], NULL, handle_client, &client_fd[clients_no]) != 0)
        {
            return 0;
        }

        clients_no++;
    }

    close(server_fd);
    return 0;
}