#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 8080
int op = 0;

void parse_operation(char *buffer, int s)
{
    char sender[1024] = {0};
    char *p = strtok(buffer, " \n");
    if (strcmp(p, "LIST") == 0)
    {
        op = 1;
        int n = send(s, p, strlen(p), 0);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }
    }
    else if (strcmp(p, "GET") == 0)
    {
        op = 2;
        strcpy(sender, "");
        strcat(sender, "GET~");

        p = strtok(NULL, " \n");
        strcat(sender, p);
        strcat(sender, "~");

        p = strtok(NULL, " \n");
        strcat(sender, p);
        strcat(sender, "~");

        int n = send(s, sender, strlen(sender), 0);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }
    }
    else if (strcmp(p, "PUT") == 0)
    {
        op = 3;
        p = strtok(NULL, " \n");
        char r[1024], buf[20];

        strcpy(sender, "");
        strcat(sender, "PUT~");
        strcat(sender, p);
        strcat(sender, "~");

        p = strtok(NULL, " \n");

        int fd = open(p, O_RDONLY);
        if (fd < 0)
        {
            printf("Fisierul nu exista, clientul nu il poate incarca pe server!\n");
            return;
        }
        int b = 0;
        int sum = 0;
        while (b = read(fd, r, 1024))
        {
            sum += b;
        }

        lseek(fd, 0, SEEK_SET);

        strcat(sender, p);
        strcat(sender, "~");

        sprintf(buf, "%d", sum);
        strcat(sender, buf);
        strcat(sender, "~");

        char *buffer2 = (char *)malloc(sum);
        read(fd, buffer2, sum);

        strcat(sender, buffer2);
        strcat(sender, "~");
        int n = send(s, sender, strlen(sender), 0);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }

        free(buffer2);
    }
    else if (strcmp(p, "DELETE") == 0)
    {
        op = 4;
        p = strtok(NULL, " \n");
        strcpy(sender, "");
        strcat(sender, "DELETE~");
        strcat(sender, p);
        strcat(sender, "~");

        p = strtok(NULL, " \n");
        strcat(sender, p);
        strcat(sender, "~");
        int n = send(s, sender, strlen(sender), 0);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }
    }
    else if (strcmp(p, "UPDATE") == 0)
    {
        op = 5;
        strcpy(sender, "");
        strcat(sender, "UPDATE~");
        p = strtok(NULL, " \n");
        while (p)
        {
            strcat(sender, p);
            strcat(sender, "~");
            p = strtok(NULL, " \n");
        }

        int n = send(s, sender, strlen(sender), 0);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }
    }
    else if (strcmp(p, "SEARCH") == 0)
    {
        op = 6;
        strcpy(sender, "");
        strcat(sender, "SEARCH~");
        p = strtok(NULL, " \n");
        strcat(sender, p);
        strcat(sender, "~");
        p = strtok(NULL, " \n");
        strcat(sender, p);
        strcat(sender, "~");

        int n = send(s, sender, strlen(sender), 0);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }
    }
}

void parse_receive(char *buffer)
{
    char *p = strtok(buffer, "~");
    if (op == 0)
    {
        printf("Eroare: Nu s-a executat nicio operatie!\n");
    }
    else if (strcmp(p, "Success") == 0)
    {
        printf("Status: %s\n", p);
        if (op == 1)
        {
            p = strtok(NULL, "~\n");
            printf("Dimensiune: %s\n", p);
            p = strtok(NULL, "~\n");
            char *pp = strtok(p, "\\0");
            while (pp != NULL)
            {
                printf("%s\t", pp);
                pp = strtok(NULL, "\\0");
            }
            printf("\n");
        }
        else if (op == 2)
        {
            p = strtok(NULL, "~\n");
            printf("Dimensiune: %s\n", p);
            p = strtok(NULL, "~");
            printf("%s\n", p);
        }
        if (op == 6)
        {
            p = strtok(NULL, "~\n");
            char *pp = strtok(p, "\\0");
            while (pp != NULL)
            {
                printf("%s\t", pp);
                pp = strtok(NULL, "\\0");
            }
            printf("\n");
        }
    }
    else if (strcmp(p, "Fail") == 0)
    {
        printf("Status: %s\n", p);
    }
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        exit(1);
    }

    printf("> ");
    bzero(buffer, 1024);
    fgets(buffer, 1023, stdin);

    parse_operation(buffer, sockfd);

    bzero(buffer, 1024);
    int size = 0;
    n = recv(sockfd, &size, sizeof(int), 0);
    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    char *buffer2 = (char *)malloc(size);
    n = recv(sockfd, buffer2, size, 0);
    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    parse_receive(buffer2);

    close(sockfd);
    return 0;
}