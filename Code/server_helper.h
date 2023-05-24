
int aux = 0;

struct file
{
    int dim_nume;
    char *nume;
    int dim;
    // char** cuv_frec;
    int read;
    int write;
};

int _log(int op, char *fis)
{
    int fd;
    char buffer[40];
    if (aux == 0)
    {
        remove("log.txt");
        fd = open("log.txt", O_WRONLY | O_CREAT, 0664);
        if (fd < 0)
        {
            return -1;
        }
    }
    else
    {
        fd = open("log.txt", O_WRONLY | O_APPEND);
        if (fd < 0)
        {
            return -1;
        }
    }

    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d-%m-%y %H:%M:%S ", timeinfo);

    char buf[10] = "";
    if (op == 1)
    {
        strcpy(buf, "LIST");
    }
    else if (op == 2)
    {
        strcpy(buf, "GET");
    }
    else if (op == 3)
    {
        strcpy(buf, "PUT");
    }
    else if (op == 4)
    {
        strcpy(buf, "DELETE");
    }
    else if (op == 5)
    {
        strcpy(buf, "UPDATE");
    }
    else if (op == 6)
    {
        strcpy(buf, "SEARCH");
    }

    strcat(buffer, buf);
    strcat(buffer, " ");

    strcat(buffer, fis);
    strcat(buffer, "\n");

    int a = write(fd, buffer, strlen(buffer));
    if (a < 0)
    {
        printf("Eroare scriere.\n");
    }

    aux++;
    close(fd);
    return 0;
};


