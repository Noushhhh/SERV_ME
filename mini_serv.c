#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ARGS "Wrong number of arguments\n"
#define BUFFER_SIZE 2000000
#define FATAL "Fatal error\n"
#define MAX_PORTS 65000

int highest_fd, server_fd, current_id = 0;
int clients[MAX_PORTS];
char message_buffer[BUFFER_SIZE + 100];
fd_set read_fds, write_fds, active_fds;

void printError(char *msg)
{
    if (server_fd > 2)
        close(server_fd);
    write(2, msg, strlen(msg));
    exit(1);
}

void replyToAll(int sender_fd)
{
	for (int fd = 2; fd <= highest_fd; fd++)
		if (fd != sender_fd && FD_ISSET(fd, &write_fds))
			if (send(fd, message_buffer, strlen(message_buffer), 0) < 0)
				handle_error(FATAL);
}

int main(int ac, char **av)
{
    int         client_fd;
    socklen_t   addr_len;
    struct      sockaddr_in server_addr, cli_addr;

    if (ac != 2)
        error(ARGS);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        error(FATAL);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(atoi(av[1]));

    if (bind(server_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr)) != 0)
        error(FATAL);
    
    if (listen(server_fd, 4096) != 0)
        error(FATAL);

    addr_len = sizeof(cli_addr);
    highest_fd = server_fd;
    FD_ZERO(&active_fds);
    FD_SET(server_fd, &active_fds);

    while (1)
    {
        read_fds = write_fds = active_fds;
        if (select(highest_fd + 1, &read_fds, &write_fds, 0, 0) < 0)
            continue;

        if (FD_ISSET(server_fd, &read_fds))
        {
            client_fd = accept(server_fd, (struct sockaddr *) &cli_addr, &addr_len);
            if (client_fd < 0)
                printError(FATAL);
            
            sprintf(message_buffer, "server: client %d just arrived\n", current_id);
            clients[client_fd] = current_id++;
            FD_SET(client_fd, &active_fds);
            replyToAll(client_fd);
            highest_fd = (client_fd > highest_fd) ? client_fd : highest_fd ;
            continue;
        }

        for (int fd = 2; fd <= highest_fd; fd++)
        {
            if (FD_ISSET(fd, &read_fds))
            {
                int bytes_received = 1;
                char msg[BUFFER_SIZE];
                memset(&msg, 0, sizeof(msg));

                while (bytes_received == 1 && msg[strlen(msg) - 1] != '\n')
                    bytes_received = recv(fd, msg + strlen(msg), 1, 0);
                
                if (bytes_received <= 0)
                {
                    sprintf(message_buffer, "server: client %d just left \n", clients[fd]);
                    FD_CLR(fd, &active_fds);
                    close(fd);
                }
                else
                    sprintf(message_buffer, "client %d: %s", clients[fd], msg);

                replyToAll(fd);
            }
        }
    }
    return (0);
}