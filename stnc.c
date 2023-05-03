#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

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
    int socket_fd1, n1;
    struct sockaddr_in server_addr1;
    char buffer[1024] = {0};

    // create a UDP socket
    if ((socket_fd1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr1, 0, sizeof(server_addr1));

    // Filling server information
    server_addr1.sin_family = AF_INET; // IPv4
    server_addr1.sin_port = htons(atoi(port));
    server_addr1.sin_addr.s_addr = INADDR_ANY;

    char message1[MAX_LEN];
    char message2[MAX_LEN];
    strcpy(message1, type);
    strcpy(message2, param);
    sendto(socket_fd1, (const char *)message1, strlen(message1), MSG_CONFIRM, (const struct sockaddr *)&server_addr1, sizeof(server_addr1));
    sendto(socket_fd1, (const char *)message2, strlen(message2), MSG_CONFIRM, (const struct sockaddr *)&server_addr1, sizeof(server_addr1));

    printf("type and param sent to server.\n");
    n1 = recvfrom(socket_fd1, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *)&server_addr1, (socklen_t *)sizeof(server_addr1));
    buffer[n1] = '\0';
    close(socket_fd1);

    if (strcmp(param, "tcp") == 0)
    {
        if (strcmp(type, "ipv6") == 0)
        {
            int client_fd, valread;
            struct sockaddr_in6 server_addr;
            char buffer[1024] = {0};
            const char *filename = "/home/yuval/Desktop/os_matala3/100MB.bin"; // switch with any 100MB+ file
            FILE *file = fopen(filename, "r");

            // Creating socket file descriptor
            if ((client_fd = socket(AF_INET6, SOCK_STREAM, 0)) == 0)
            {
                perror("socket failed");
                exit(EXIT_FAILURE);
            }

            // Initialize the server address structure
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin6_family = AF_INET6;
            server_addr.sin6_port = htons(atoi(port) + 1);

            // Convert the server IP address from text to binary format
            if (inet_pton(AF_INET6, ip, &server_addr.sin6_addr) < 0)
            {
                perror("inet_pton");
                exit(EXIT_FAILURE);
            }

            // Connect to the server
            if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                perror("connect");
                exit(EXIT_FAILURE);
            }

            printf("Connected to %s:%d\n", ip, atoi(port) + 1);

            // Send the file data to the server
            long bytes_sent = 0;
            while (!feof(file))
            {
                int num_read = fread(buffer, sizeof(char), sizeof(buffer), file);
                int num_sent = send(client_fd, buffer, num_read, 0);
                bytes_sent += num_sent;
            }

            printf("Sent %ld bytes\n", bytes_sent);

            // Close the client socket and file
            close(client_fd);
            fclose(file);

            return 0;
        }
        else if (strcmp(type, "ipv4") == 0)
        {
            const char *filename = "/home/yuval/Desktop/os_matala3/100MB.bin"; // switch with any 100MB+ file
            FILE *file = fopen(filename, "r");
            int client_fd = 0, valread;
            struct sockaddr_in serv_addr;
            char buffer[MAX_MSG_LEN] = {0};

            if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("socket creation error");
                return -1;
            }
            memset(&serv_addr, '0', sizeof(serv_addr));

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(atoi(port) + 1);

            // Converting IPv4 address from text to binary form
            if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
            {
                perror("Invalid address/ Address not supported");
                return -1;
            }
            if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("Connection Failed");
                return -1;
            }
            long bytes_sent = 0;
            while (!feof(file))
            {
                int num_read = fread(buffer, sizeof(char), sizeof(buffer), file);
                int num_sent = send(client_fd, buffer, num_read, 0);
                bytes_sent += num_sent;
            }

            printf("Sent %ld bytes\n", bytes_sent);

            // Close the client socket and file
            close(client_fd);
            fclose(file);
            return 0;
        }
    }
    return 0;
}

int server_test(char *port, int test, int quiet)
{
    char client_type[MAX_LEN];
    char client_param[MAX_LEN];
    int socket_fd1, length1, n1;
    struct sockaddr_in server_addr1, client_addr1;
    char buffer[1024];
    clock_t start_time, end_time;
    double elapsed_time;

    // create a UDP socket
    if ((socket_fd1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr1, 0, sizeof(server_addr1));
    memset(&client_addr1, 0, sizeof(client_addr1));

    // Filling server information
    server_addr1.sin_family = AF_INET; // IPv4
    server_addr1.sin_addr.s_addr = INADDR_ANY;
    server_addr1.sin_port = htons(atoi(port));

    // Bind the socket with the server address
    if (bind(socket_fd1, (const struct sockaddr *)&server_addr1, sizeof(server_addr1)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        // Wait for client request
        length1 = sizeof(client_addr1);
        int i = 0;
        while (i < 2)
        {
            socklen_t length = sizeof(client_addr1);
            n1 = recvfrom(socket_fd1, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *)&client_addr1, &length);
            buffer[n1] = '\0';
            if (i == 0)
            {
                strcpy(client_type, buffer);
            }
            else
            {
                strcpy(client_param, buffer);
            }
            i++;
            memset(buffer, 0, sizeof(buffer));
        }
        char *message = "done";
        sendto(socket_fd1, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&client_addr1, length1);

        if (test == 1)
        {
            if (strcmp(client_type, "ipv6") == 0 && strcmp(client_param, "tcp") == 0)
            {
                int server_fd, client_fd, valread;
                struct sockaddr_storage server_addr, client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                char buffer[MAX_MSG_LEN] = {0};

                // Creating socket file descriptor
                if ((server_fd = socket(AF_INET6, SOCK_STREAM, 0)) == 0)
                {
                    perror("socket failed");
                    exit(EXIT_FAILURE);
                }

                // Initialize the server address structure
                memset(&server_addr, 0, sizeof(server_addr));
                ((struct sockaddr_in6 *)&server_addr)->sin6_family = AF_INET6;
                ((struct sockaddr_in6 *)&server_addr)->sin6_port = htons(atoi(port) + 1);
                ((struct sockaddr_in6 *)&server_addr)->sin6_addr = in6addr_any;

                // Bind the socket to the server address
                if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    perror("bind failed");
                    exit(EXIT_FAILURE);
                }

                // Listen for incoming connections
                if (listen(server_fd, 1) < 0)
                {
                    perror("listen failed");
                    exit(EXIT_FAILURE);
                }

                printf("Listening on port %d...\n", atoi(port) + 1);

                // Accept incoming connections
                if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
                {
                    perror("accept failed");
                    exit(EXIT_FAILURE);
                }

                printf("Accepted connection from %s:%d\n",
                       inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr),
                       ntohs(((struct sockaddr_in *)&client_addr)->sin_port));

                // Receive data from the client and start measuring time
                start_time = clock();
                long bytes_received = 0;
                while ((valread = recv(client_fd, buffer, MAX_MSG_LEN, 0)) > 0)
                {
                    bytes_received += valread;
                }

                printf("Received %ld bytes\n", bytes_received);
                end_time = clock();
                elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0;

                printf("%s_%s, %f\n", client_type, client_param, elapsed_time);

                // Close the client socket, server socket
                close(client_fd);
                close(server_fd);
            }
            else if (strcmp(client_type, "ipv4") == 0 && strcmp(client_param, "tcp") == 0)
            {
                int server_fd, client_fd, valread;
                struct sockaddr_in address;
                int opt = 1;
                int addrlen = sizeof(address);
                char buffer[MAX_MSG_LEN] = {0};

                // Creating socket file descriptor
                if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
                {
                    perror("socket failed");
                    exit(EXIT_FAILURE);
                }

                // Forcefully attaching socket to the port 8080
                if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                               &opt, sizeof(opt)))
                {
                    perror("setsockopt");
                    exit(EXIT_FAILURE);
                }
                address.sin_family = AF_INET;
                address.sin_addr.s_addr = INADDR_ANY;
                address.sin_port = htons(atoi(port) + 1);

                // Binding the socket to the IP address and port
                if (bind(server_fd, (struct sockaddr *)&address,
                         sizeof(address)) < 0)
                {
                    perror("bind failed");
                    exit(EXIT_FAILURE);
                }

                // Listening for incoming connections
                if (listen(server_fd, 3) < 0)
                {
                    perror("listen");
                    exit(EXIT_FAILURE);
                }

                printf("Listening on port %d...\n", atoi(port) + 1);

                // Accepting incoming connection
                if ((client_fd = accept(server_fd, (struct sockaddr *)&address,
                                        (socklen_t *)&addrlen)) < 0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                // Reading file contents sent by client
                start_time = clock();
                long bytes_received = 0;
                while ((valread = recv(client_fd, buffer, MAX_MSG_LEN, 0)) > 0)
                {
                    bytes_received += valread;
                }
                printf("Received %ld bytes\n", bytes_received);
                end_time = clock();
                elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0;
                printf("%s_%s, %f\n", client_type, client_param, elapsed_time);

                // Close the client socket, server socket
                close(client_fd);
                close(server_fd);
            }
            memset(client_type, 0, sizeof(client_type));
            memset(client_param, 0, sizeof(client_param));
            length1 = 0;
        }
    }
    close(socket_fd1);
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
            server_test(port, test_flag, quiet_flag);
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
