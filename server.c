#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <math.h>


#define BUFFLEN 3072

struct topics {
	char* info;
	int SF; 
	int unsubscribe;
};
struct client{
	int socket;
	char* IP;
	char* ID;
	int port;
	int status;
	int topicNo;
	int msgNo;
	struct topics* t[100];
	char* mesage[100];
};


void error(const char* msg){
	perror(msg);
	exit(1);
}
struct topics* insertTopic(char* topic, int SF) {
	struct topics* aux = (struct topics*)malloc(sizeof(struct topics));
	aux->info = strdup(topic);
	aux->SF = SF;
	aux->unsubscribe = 0;
	return aux;
}

struct client* insertClient(struct sockaddr_in cli_addrTCP, char* ID, int socket) {
	struct client *aux = (struct client*)malloc(sizeof(struct client));
	aux->IP = (char*) malloc(50 * sizeof(char));
	aux->IP = inet_ntoa(cli_addrTCP.sin_addr);
	aux->ID = strdup(ID);
	aux->port = ntohs(cli_addrTCP.sin_port);
	aux->status = 1;
	aux->topicNo = 0;
	aux->msgNo = 0;
	aux->socket = socket;

	return aux;
}



void parseUDP(char* buffer, char* result) {
	char* topic = (char*)malloc(50 * sizeof(char));
	char valueC[BUFFLEN];
	sprintf(topic, "%s", buffer);
	int type = buffer[50];
	int sign;
	uint32_t number1, number2;
	int value;
	float valueF;

	if(type == 0) {
		sign = buffer[51];
		number1 = (unsigned char) buffer[52] << 24 | (unsigned char) buffer[53] << 16 | (unsigned char) buffer[54] << 8 | (unsigned char) buffer[55];
		if(sign == 1) 
			value = number1 * (-1);
		else 
			value = number1;
		sprintf(result, "%s - INT - %d", topic, value);

	}else if(type == 1) {
		valueF = ((unsigned char) buffer[51] << 8 | (unsigned char) buffer[52]) / (float) 100;
		sprintf(result, "%s - SHORT_REAL - %.2f", topic, valueF);

	} else if(type == 2) {
		sign = buffer[51];
		number1 = (unsigned char) buffer[52] << 24 | (unsigned char) buffer[53] << 16 | (unsigned char) buffer[54] << 8 | (unsigned char) buffer[55];
		number2 = (unsigned char)(buffer[56]);
		if(sign == 1) {
			valueF = (float)( number1 / pow(10, number2)); 
			valueF = (-1) * valueF;
		} else
			valueF = (float) (number1 / pow(10, number2));
		sprintf(result, "%s - FLOAT - %.4f", topic, valueF);

	} else if(type == 3) {
		sprintf(valueC, "%s", buffer + 51);
		sprintf(result, "%s - STRING - %s", topic, valueC);
	}
}


int main(int argc, char *argv[]) {


	struct client* a[100000];
	int o = 1;
	int sockUDP, sockTCP, new_sock, portNo, bindUDP, bindTCP, ret;
	int i, n;
	char buffer[BUFFLEN], act[BUFFLEN];
	struct sockaddr_in serv_addr, cli_addrTCP, cli_addrUDP;
	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds
	char* UDPmsg;
	int clNo = 0;
	//deschid socket UDP
	sockUDP = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sockUDP < 0, "socketUDP");

	//deschid socket TCP
	sockTCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockTCP < 0, "socketTCP");
	
	//initialzari struct sockaddr_in pentru a asculta pe un port
	portNo = atoi(argv[1]);
	DIE(portNo == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portNo);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// Legare proprietati de socketi
    bindUDP = bind(sockUDP, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
    DIE(bindUDP < 0, "bind UDP");

	bindTCP = bind(sockTCP, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
    DIE(bindTCP < 0, "bind TCP");

    ret = listen(sockTCP, 1000);
	DIE(ret < 0, "listen");
	setsockopt(sockTCP, IPPROTO_TCP, TCP_NODELAY, &o, sizeof(o));
	setsockopt(sockUDP, IPPROTO_UDP, TCP_NODELAY, &o, sizeof(o));

	fdmax = sockUDP;
	if (sockTCP > sockUDP) { 
		fdmax = sockTCP;
	}
	
	// se goleste multimea de descriptori de citire read_fds si multimea temporara tmp_fds
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
    // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockUDP, &read_fds);
    FD_SET(sockTCP, &read_fds);
    FD_SET(0, &read_fds);
    while(1) {
    	tmp_fds = read_fds;

    	ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {

			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockTCP) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen), pe care serverul o accepta
					
					//printf("am ajuns pana aici\n");
					socklen_t cli_len = sizeof(cli_addrTCP);
					new_sock = accept(i, (struct sockaddr *) &cli_addrTCP, &cli_len);
					setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, &o, sizeof(o));
					DIE(new_sock < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					memset(buffer,0,sizeof(buffer));
					n = recv(new_sock, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					
					int k=0;
					for(int j = 0; j < clNo; j++) {
						if(strcmp(a[j]->ID, buffer)==0){
							if(a[j]->status == 0){
								FD_SET(new_sock, &read_fds);

								if (new_sock > fdmax) { 
									fdmax = new_sock;
								}
								printf("reconectare\n");
								a[j]->status = 1;
								a[j]->socket = new_sock;
							} 
							k = 1;
						}
					}
					if(k == 0){
					FD_SET(new_sock, &read_fds);

					if (new_sock > fdmax) { 
						fdmax = new_sock;
					}

					a[clNo] = insertClient(cli_addrTCP, buffer, new_sock);
					clNo++;
					printf("New client %s connected from %s:%d.\n", buffer,
							inet_ntoa(cli_addrTCP.sin_addr), ntohs(cli_addrTCP.sin_port));
					}
				} else if(i == sockUDP) {
					// primesc mesaje de la clientii UDP
					socklen_t cli_len = sizeof(cli_addrUDP);
					n = recvfrom(i, buffer, sizeof(buffer), 0, (struct sockaddr*)&cli_addrUDP, &cli_len);
					DIE(n < 0, "Error reading from sockUDP");
					char arg[1600];
					n = sprintf(arg,"%s:%d ",inet_ntoa(cli_addrUDP.sin_addr), ntohs(cli_addrUDP.sin_port));
					
					UDPmsg = (char*)malloc(BUFFLEN * sizeof(char));
					parseUDP(buffer, UDPmsg);
					sprintf(arg+n,"%s",UDPmsg);
					

					for(int j = 0; j < clNo; j++) {
						for(int k = 0; k < a[j]->topicNo; k++)
							if(strcmp(a[j]->t[k]->info, buffer) == 0){
								if(a[j]->status == 1) {
									n = send(a[j]->socket, arg, strlen(arg), 0);
									DIE(n < 0, "send");
								}
							}
					}
					
				} else if(i == 0) {
					n = read(i, buffer, sizeof(buffer));
					if(strcmp(buffer, "exit") == 0){
						close(sockUDP);
						close(sockTCP);
						return 0;
					}
				} else {
					// s-au primit date pe unul din socketii de client TCP,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					socklen_t cli_len = sizeof(cli_addrTCP);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						for(int j = 0; j < clNo; j++) {
							if(a[j]->socket == i){
								a[j]->status = 0;
								printf("Client %s disconnected.\n", a[j]->ID);
							}
						}
						
						close(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else { 

						char* act;
						char*topic;
						char* val;
						int SF;
						val = NULL;
						val = strtok(buffer, " ");
						
						act = strdup(val);
						val = strtok(NULL, " ");
						
						topic = strdup(val);
						
						val = strtok(NULL, " ");
						
						if(val != NULL)
							SF = atoi(val);
						

						if(strcmp(act, "unsubscribe") == 0){
							for(int k = 0; k < clNo; k++){
								
								if(a[k]->socket == i){
									for(int j = 0; j < a[k]->topicNo; j++) {
										if(strcmp(topic, a[k]->t[j]->info)) {
											a[k]->t[j]->unsubscribe = 1;
											printf("%s\n", a[k]->t[j]->info);
										}
									}
								}
							}	
						} else {
							for(int k = 0; k < clNo; k++){
								
								if(a[k]->socket == i){
									
									a[k]->t[a[k]->topicNo] = insertTopic(topic, SF);
									a[k]->topicNo++;
									
								}
							}	
						}
						
					}
		
				}	
			}
		}
    }
}