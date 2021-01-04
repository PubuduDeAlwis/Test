/*
	Name	- K.K.P De Alwis
	Reg.No	- IT18106848
	Name	- D.N Munasinghe
	Reg.No	- It18204162
	Module	- Network Programming
	Batch	- 3rd Year June Intake
*/

#include "server.h"

//getExtention () -> function that used to find the content type of a given file
const char *getExtention(const char* path){
	const char *lastDot = strrchr(path,'.');
	if(lastDot){

		if(strcmp(lastDot,".css") == 0) 
			return "text/css";
		if(strcmp(lastDot,".csv") == 0)
			return "text/csv";
		if(strcmp(lastDot,".gif") == 0)
			return "image/gif";
		if(strcmp(lastDot,".htm") == 0)
			return "text/html";
		if(strcmp(lastDot,".html") == 0)
			return "text/html";
		if(strcmp(lastDot,".ico") == 0)
			return "image/x-icon";
		if(strcmp(lastDot,".jpeg") == 0 )
			return "image/jpeg";
		if(strcmp(lastDot,".jpg") == 0)
			return "image/jpeg";
		if(strcmp(lastDot,".js") == 0)
			return "application/javascript";
		if(strcmp(lastDot,".json") == 0)
			return "application/json";
		if(strcmp(lastDot,".png") == 0)
			return "image/png";
		if(strcmp(lastDot,".pdf") == 0)
			return "application/pdf";
		if(strcmp(lastDot,".svg") == 0)
			return "image/svg+xml";
		if(strcmp(lastDot,".txt") == 0)
			return "text/plain";
	}
	return "application/octect-stream";
}

//createSokcet() -> used to get a listening socket as a return value
SOCKET createSocket(const char* host, const char *port){
	printf("Configuring local address...\n");
	struct addrinfo config;
	memset(&config, 0, sizeof(config));
	config.ai_family = AF_INET;
	config.ai_socktype = SOCK_STREAM;
	config.ai_flags = AI_PASSIVE;

	struct addrinfo *server_addr;

	getaddrinfo(host, port, &config, &server_addr);

	printf("Creating socket...\n");

	SOCKET s_listen;
	s_listen = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol);
       if(!ISVALIDSOCKET(s_listen)){
		fprintf(stderr,"listen socket creation failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
       }

       printf("Binding listen socket with server local address...\n");
       if(bind(s_listen, server_addr->ai_addr, server_addr->ai_addrlen)){
		fprintf(stderr,"Server listen socket binding failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
       }
       freeaddrinfo(server_addr);

       printf("Socket is listening..\n");
       if(listen(s_listen,10)<0){
		fprintf(stderr,"listen() failed. (%d)\n",GETSOCKETERRNO());
		exit(1);
       }

       return s_listen;		       
}

#define MAX_REQUEST_SIZE 2047 
//store information about each connected clients
struct clientInfo {
	socklen_t address_len;
	struct sockaddr_storage address;
	SOCKET clientSocket;
	char request[MAX_REQUEST_SIZE+1];
	int received;
	struct clientInfo *next;
};

static struct clientInfo *clients = 0;

//getClient()->takes SOCKET variable and searches our linked list for the correspoding client info data structure
struct clientInfo *getClient(SOCKET s){
		
	struct clientInfo *ci = clients;

	while(ci){
		if(ci->clientSocket == s)
			break;
		ci = ci-> next;
	}
	if(ci)
		return ci;

	struct clientInfo *new = (struct clientInfo *) calloc(1, sizeof(struct clientInfo));

	if(!new){
		fprintf (stderr, "Out of memory.\n");
		exit(1);
	}

	new->address_len = sizeof(new->address);
	new->next = clients;
	clients = new;
	return new;

}

//dropClient() -> closes the connection to a client and remove it form the clients linked list.
void dropClient(struct clientInfo *client){
	CLOSESOCKET(client -> clientSocket);

	struct clientInfo **point = &clients;

	while(*point){

		if(*point == client){
			*point = client->next;
			free(client);
			return;
		}
		point = &(*point)->next;
	}
	fprintf(stderr,"drop_client not found.\n");
	exit(1);
	
}

//get_client_address() -> convert given client's IP address into text.
	
const char *getClientAddr(struct clientInfo *ci){
	static char address_buff[100];
	getnameinfo((SA *)&ci->address, ci->address_len, address_buff, sizeof(address_buff), 0, 0, NI_NUMERICHOST);

	return address_buff;
}
	
//wait_on_clients() -> wait for data form multiple clients
	
fd_set waitOnClients(SOCKET server){

	fd_set reads;
	FD_ZERO(&reads);
	FD_SET(server, &reads);
	SOCKET maxSocket = server;

	struct clientInfo *ci = clients;

	while(ci){
		FD_SET(ci->clientSocket, &reads);
		if(ci->clientSocket > maxSocket)
			maxSocket = ci->clientSocket;
		ci = ci->next;
	}

	if(select(maxSocket+1, &reads, 0, 0, 0) < 0){
		fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}

	return reads;

}

//send400() -> Response to a HTTP request of a client that server does not understand
	
void send400(struct clientInfo *client){
	const char *error400 =	"HTTP/1.1 400 Bad Request\r\n"
				"Connection: close\r\n"
				"Content-Length: 11\r\n\r\nBAD Request";
	send(client->clientSocket, error400, strlen(error400), 0);
	dropClient(client);
				
}

//send404() -> Response to a requested resource is not found

void send404(struct clientInfo *client){
	const char *error404 =	"HTTP/1.1 404 Not Found\r\n"
				"Connection: close\r\n"
				"Content-Length: 9\r\n\r\nNot Found";
	send(client->clientSocket, error404, strlen(error404), 0);
	dropClient(client);
}

//serve_resource() -> sends a connected client a requested resource
void serveGetResource(struct clientInfo *client, const char *path){

	printf("serve_resource %s %s\n", getClientAddr(client), path);

	if(strcmp(path,"/")==0)
		path = "/index.html";

	if(strlen(path)>100){
		send400(client);
		return;
	}

	if(strstr(path,"..")){
		send404(client);
		return;
	}

	char full_path[128];
	sprintf(full_path,"public%s",path);

	FILE *fp = fopen(full_path,"rb");

	if(!fp){
		send404(client);
		return;
	}

	fseek(fp, 0L, SEEK_END);
	size_t cl = ftell(fp);
	rewind(fp);

	const char *ct = getExtention(full_path);
#define BSIZE 1024
	char buffer[BSIZE];

	sprintf(buffer,"HTTP/1.1 200 OK\r\n");
	send(client->clientSocket, buffer, strlen(buffer), 0);

	sprintf(buffer,"Connection: close\r\n");
	send(client->clientSocket, buffer, strlen(buffer), 0);
	
	sprintf(buffer,"Content-Length: %u\r\n",cl);
	send(client->clientSocket, buffer, strlen(buffer), 0);

	sprintf(buffer,"Content-Type: %s\r\n",ct);
	send(client->clientSocket, buffer, strlen(buffer), 0);

	sprintf(buffer,"\r\n");
	send(client->clientSocket, buffer, strlen(buffer), 0);

	int r = fread(buffer, 1, BSIZE, fp);

	while(r){
		send(client->clientSocket, buffer, r, 0);
		r = fread(buffer, 1, BSIZE, fp);

	}

	fclose(fp);
	dropClient(client);

}



int main(int argc,char *argv[]){
	
	if(argc<2){
		fprintf(stderr,"USEAGE:Server <port number>\n");
		return 1;
	}
	SOCKET server = createSocket("0", argv[1]);

	while(1){
		fd_set reads;
		reads = waitOnClients(server);

		if(FD_ISSET(server, &reads)){
			
			struct clientInfo *client = getClient(-1);
			
			client->clientSocket = accept(server, (SA *) &client->address, &(client->address_len));

			if(!ISVALIDSOCKET(client->clientSocket)){
				fprintf(stderr, "accept() failed. (%d)\n",GETSOCKETERRNO());
				return 1;
			}
	

			printf("New connection from %s.\n", getClientAddr(client));
		}

		struct clientInfo *client = clients;
		while(client){
			struct clientInfo *next = client->next;
			
			if(FD_ISSET(client->clientSocket, &reads)){
			
				if(MAX_REQUEST_SIZE == client->received){
					send400(client);
					client = next;
					continue;
				}

			int r = recv(client->clientSocket, client->request + client->received, MAX_REQUEST_SIZE - client->received,0);
				if(r < 1){
					printf("Unexpected disconnet from %s.\n", getClientAddr(client));
					dropClient(client);
				}
				else{
					client->received += r;
					client->request[client->received] = 0;


					char *q = strstr(client->request, "\r\n\r\n");
				
					if(q){
						

						if(strncmp("GET /", client->request, 5)==0){
							*q =0;
							char *path = client->request+4;
							char *endPath = strstr(path, " ");
							if(!endPath){
								send400(client);
							}
							else{
								*endPath = 0;
								serveGetResource(client, path);
							}
						}else if(strncmp("PUT /",client->request,5)==0){
							char *content = strstr(client->request,"\r\n\r\n");
							
							content +=4;
							printf("%s",content);
	
							FILE *fp;
							fp = fopen("public/client_upload.txt","a");
							fprintf(fp,"%s",content);

							size_t cl = strlen(content);
							char *path = client->request+5;
							char buff[2048];
							sprintf(buff,"HTTP/1.1 200 OK\r\n");
							sprintf(buff + strlen(buff),"Content-Type: text/plain\r\n");
							sprintf(buff + strlen(buff),"Content-Length: %u\r\n",cl);
							sprintf(buff + strlen(buff),"File Received\r\n");
							sprintf(buff + strlen(buff),"\r\n");
							sprintf(buff + strlen(buff),"Success\r\n");
							send(client->clientSocket, buff, sizeof(buff), 0);

							fclose(fp);
						}else{
							*q = 0;
							send400(client);
							return 1;
						}
					}//if(q)
				}
			}

			client = next;
		}
	}

	printf("\nClosing socket...\n");
	CLOSESOCKET(server);
	printf("Finished.\n");

	return 0;
}
