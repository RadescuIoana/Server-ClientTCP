#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

#define BUFFLEN 3072

void error(const char* msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	int sock_fd, portNo, n, ret;
	int o = 1;
	int fdmax;
	int SF;
	fd_set read_fds;
	fd_set tmp_fds;
	struct sockaddr_in serv_addr;
	char buffer[BUFFLEN];
	char* act = (char*) malloc(BUFFLEN);

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_fd < 0, "socket");

	portNo = atoi(argv[3]);
	DIE(portNo == 0, "atoi");

	memset((char*)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portNo);
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sock_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &o, sizeof(o));
	FD_SET(sock_fd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sock_fd;
	ret = send(sock_fd, argv[1],  strlen(argv[1]), 0);
	DIE(ret < 0, "send");

	while(1) {

		tmp_fds = read_fds;

    	ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) {
					//memset(buffer, 0, strlen(buffer));

					n = read(i, buffer, sizeof(buffer));
					ret = send(sock_fd, buffer, strlen(buffer), 0);
					DIE(ret < 0, "send");

				} else if(i == sock_fd){
					//memset(buffer, 0, sizeof(buffer));
					n = recv(sock_fd, buffer, strlen(buffer), 0);
					
					
					DIE(n < 0, "recv");
					if(n == 0){
						
						close(sock_fd);
						return 0;
					}
					printf("%s\n", buffer);
				}
			}
		}
		
	}

	close(sock_fd);
	return 0;
}
