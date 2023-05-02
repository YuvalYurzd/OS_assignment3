#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/time.h>

#define MAX_LEN 100
#define MAX_MSG_LEN 1024
#define SO_REUSEPORT 15
#define POLL_SIZE 2

void printusage()
{
    printf("Usage for client: stnc -c [IP] [PORT] [-p] <type> <param>\n");
    printf("Usage for server: stnc -s [PORT] [-p] [-q]\n");
    exit(0);
}

int client(char *ip, char *port)
{
    int sock = 0, bytes_read;
    int bytes_sent;
    struct sockaddr_in serv_addr;
    char buffer[MAX_MSG_LEN] = {0};
    struct pollfd fds[POLL_SIZE];

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nSocket creation error\n");
        exit(0);
    }

    // Set server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported\n");
        exit(0);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection failed\n");
        exit(0);
    }

    // Initialize pollfd array
    memset(fds, 0, sizeof(fds));
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sock;
    fds[1].events = POLLIN;

    printf("Connected to chat server on port %d\n", atoi(port));

    while (1)
    {
        // Poll for events
        if (poll(fds, POLL_SIZE, -1) < 0)
        {
            printf("\nPoll failed\n");
            exit(0);
        }

        // Handle user input
        if (fds[0].revents & POLLIN)
        {
            fgets(buffer, MAX_MSG_LEN, stdin);
            buffer[strlen(buffer) - 1] = '\0';
            bytes_sent = send(sock, buffer, strlen(buffer), 0);
            if (bytes_sent < 0)
            {
                perror("error in sending message");
                exit(0);
            }
            memset(buffer, 0, sizeof(buffer));
        }

        // Handle server messages
        if (fds[1].revents & POLLIN)
        {
            bytes_read = recv(sock, buffer, MAX_MSG_LEN, 0);
            if (bytes_read <= 0)
            {
                printf("error in receiving message");
                exit(0);
            }
            else
            {
                printf("Message received from server: %s\n", buffer);
            }
            memset(buffer, 0, sizeof(buffer));
        }
    }

    return 0;
}

int server(char *port)
{
    int server_fd, new_socket, activity, bytes_read, bytes_sent;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[MAX_MSG_LEN] = {0};
    int opt = 1;
    int clients[POLL_SIZE] = {0};
    struct pollfd fds[POLL_SIZE];

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(0);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(0);
    }

    // Set server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(port));

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(0);
    }
    printf("Chat server started on port %d\n", atoi(port));
    // Listen for connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(0);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept failed");
        exit(0);
    }
    else
    {
        printf("Started connection with client\n");
    }
    close(server_fd);

    // Initialize pollfd array
    memset(fds, 0, sizeof(fds));
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = new_socket;
    fds[1].events = POLLIN;

    while (1)
    {
        // Poll for events
        activity = poll(fds, POLL_SIZE, -1);
        if (activity < 0)
        {
            perror("poll failed");
            close(new_socket);
            exit(0);
        }

        if (fds[1].revents & POLLIN)
        {
            bytes_read = recv(new_socket, buffer, MAX_MSG_LEN, 0);
            if (bytes_read <= 0)
            {
                perror("error in receiving message");
                exit(0);
            }
            else
            {
                printf("Message received from client: %s\n", buffer);
                memset(buffer, 0, sizeof(buffer));
            }
        }
        if (fds[0].revents & POLLIN)
        {
            fgets(buffer, MAX_MSG_LEN, stdin);
            buffer[strlen(buffer) - 1] = '\0';
            bytes_sent = send(new_socket, buffer, strlen(buffer), 0);
            if (bytes_sent < 0)
            {
                perror("error in sending message");
                exit(0);
            }
            memset(buffer, 0, sizeof(buffer));
        }
    }

    return 0;
}

int client_test(char *ip, char *port, char *type, char *param)
{
    return 0;
}

int server_test(char *port, int test, int quiet)
{
    return 0;
}

int main(int argc, char *argv[])
{
    // decalre flags for all possible combinations
    int test_flag = 0, quiet_flag = 0;
    char param[MAX_LEN];
    char type[MAX_LEN];
    char port[MAX_LEN];
    char ip[MAX_LEN];

    if (argc < 3 || argc > 7)
    {
        printusage();
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "-s") == 0) // part A server
        {
            strcpy(port, argv[2]);
            server(port);
        }
        else
        {
            printusage();
        }
        server_test(port, test_flag, quiet_flag);
    }
    else if (argc == 4)
    {
        if (strcmp(argv[1], "-c") == 0) // part A client
        {
            strcpy(ip, argv[2]);
            strcpy(port, argv[3]);
            client(ip, port);
        }
        else if (strcmp(argv[1], "-s") == 0 && strcmp(argv[3], "-p") == 0) // Server with perform test flag enabled
        {
            strcpy(port, argv[2]);
            test_flag = 1;
            server(port);
        }
        else
        {
            printusage();
        }
    }
    else if (argc == 5)
    {
        // Server with perform test flag enabled and quiet flag enabled
        if (strcmp(argv[1], "-s") == 0 && strcmp(argv[3], "-p") == 0 && strcmp(argv[4], "-q") == 0)
        {
            strcpy(port, argv[2]);
            test_flag = 1;
            quiet_flag = 1;
            server(port);
        }
        else
        {
            printusage();
        }
        server_test(port, test_flag, quiet_flag);
    }
    else if (argc == 6)
    {
        printusage();
    }

    else if (argc == 7) // client with test flag enabled
    {
        if (strcmp(argv[1], "-c") == 0 && strcmp(argv[4], "-p") == 0)
        {
            test_flag = 1;
            strcpy(ip, argv[2]);
            strcpy(port, argv[3]);
            if (strcmp(argv[5], "ipv4") == 0 || strcmp(argv[5], "ipv6") == 0)
            {
                strcpy(type, argv[5]);
                if (strcmp(argv[6], "tcp") == 0 || strcmp(argv[6], "udp") == 0)
                {
                    strcpy(param, argv[6]);
                }
                else
                {
                    printusage();
                }
            }
            else if (strcmp(argv[5], "uds") == 0)
            {
                strcpy(type, argv[5]);
                if (strcmp(argv[6], "stream") == 0 || strcmp(argv[6], "dgram") == 0)
                {
                    strcpy(param, argv[6]);
                }
                else
                {
                    printusage();
                }
            }
            else if (strcmp(argv[5], "mmap") == 0 || strcmp(argv[5], "pipe") == 0)
            {
                strcpy(type, argv[5]);
                // fill the rest (file)
            }
            else
            {
                printusage();
            }
            client_test(ip, port, type, param);
        }
    }
    return 0;
}
