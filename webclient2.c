/*
	Name	- K.K.P De Alwis
	Reg.No	- IT18106848
	Name	- D.N Munasinghe
	Reg.No	- It18204162
	Module	- Network Programming
	Batch	- 3rd Year June Intake
*/

#include "client.h"
#define TIMEOUT 5.0

//function to pass given url
void parseURL(char *url, char **hostname, char **port){

	printf("URL: %s\n",url);

	char *p;
	p = strstr(url,"://");

	char *protocol = 0;

	if(p){
		protocol = url;
		*p = 0;
		p += 3;
	}
	else{
		p = url;
	}

	if(protocol){
		if(strcmp(protocol,"http")){
			fprintf(stderr,"Unknown protocol '%s'. 'http' is supported.\n",protocol);
			exit(1);
		}
	}

	*hostname = p;
	while (*p && *p != ':' && *p != '/' && *p != '#')
		++p;

	*port ="80";
	if(*p == ':'){
		*p++ = 0;
		*port = p;
	}
	while(*p && *p != '/' && *p != '#')
		++p;

	if(*p=='/' || *p == '#')
		*p=0;

	printf("hostname: %s\n", *hostname);
	printf("port: %s\n", *port);
}

//send http request
void sendRequest(SOCKET s, char *hostname, char *port, char *path, char *method){
	char buffer[2048];
	char *fileread;
	if(strcmp("GET",method)==0){
		sprintf(buffer,"GET /%s HTTP/1.1\r\n",path);
		sprintf(buffer + strlen(buffer), "HOST: %s:%s\r\n",hostname,port);
		sprintf(buffer + strlen(buffer), "Connection: close\r\n");
		sprintf(buffer + strlen(buffer), "User-Agent: honpwc web_get 1.0\r\n");
		sprintf(buffer + strlen(buffer), "\r\n");

		send(s,buffer,strlen(buffer),0);
		printf("Sent Headers:\n%s",buffer);
	}
	else if(strcmp("PUT",method)==0){

		if(strstr(path,"..")){
			exit(1);
		}
		char full_path[128];
		sprintf(full_path,"client/%s",path);
		FILE *fp = fopen(full_path,"rb");
		if(!fp){
			fprintf(stderr,"ERROR: File can not be found");
			exit(1);
		}
		fseek(fp, 0L, SEEK_END);
		long  cl = ftell(fp);
		rewind(fp);
		fileread = calloc(1, cl+1);
		if(!fileread) fclose(fp),fprintf(stderr,"memory allocation fails"),exit(1);

		if(1!=fread(fileread,cl,1,fp))
			fclose(fp),free(fileread),fprintf(stderr,"Read fails"),exit(1);

		sprintf(buffer,"PUT /%s HTTP1.1\r\n",full_path);
		sprintf(buffer + strlen(buffer),"HOST: %s:%s\r\n",hostname,port);
		sprintf(buffer + strlen(buffer), "Content-Type: text/plain\r\n");
		sprintf(buffer + strlen(buffer), "Content-Length: %u\r\n",cl);
		sprintf(buffer + strlen(buffer),"\r\n");
		sprintf(buffer + strlen(buffer),"%s",fileread);

		send(s,buffer,strlen(buffer),0);
		printf("Sent Headers:\n%s",buffer);
		
		fclose(fp);
		free(fileread);
		
	}else{
		sprintf(buffer,"%s /%s HTTP1.1\r\n",method,path);
		sprintf(buffer + strlen(buffer),"\r\n");
		send(s,buffer,strlen(buffer),0);
		printf("Sent Headers: \n%s",buffer);
	}
}


//connect to HTTP web server
SOCKET connetToHost(char *hostname, char *port){
	
	printf("Configuring remote address...\n");
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *server_addr;
	if(getaddrinfo(hostname, port, &hints, &server_addr)){
		fprintf(stderr,"getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}
	printf("Remote address is: ");
	char address_buffer[100];
	char service_buffer[100];

	getnameinfo(server_addr->ai_addr, server_addr->ai_addrlen,address_buffer,sizeof(address_buffer),service_buffer,sizeof(service_buffer),NI_NUMERICHOST);
	printf("%s %s\n",address_buffer, service_buffer);

	printf("Creating socket...\n");
	SOCKET server;
	server = socket(server_addr->ai_family,server_addr->ai_socktype,server_addr->ai_protocol);
	if(!ISVALIDSOCKET(server)){
		fprintf(stderr,"socket() failed. (%d)\n",GETSOCKETERRNO());
		exit(1);
	}

	printf("Connecting...\n");
	if(connect(server,server_addr->ai_addr,server_addr->ai_addrlen)){
		fprintf(stderr,"connect() failed. (%d)\n",GETSOCKETERRNO());
		exit(1);
	}
	freeaddrinfo(server_addr);
	printf("Connected.\n");

	return server;
}

int main(int argc, char *argv[]){

	if(argc<4){
		fprintf(stderr,"useage: web get <url> <GET/PUT><File_Name>\n");
		return 1;
	}
	char *url = argv[1];
	char *method=argv[2];
	char *path = argv[3];
	char *hostname, *port;
	parseURL (url, &hostname, &port);

	SOCKET server = connetToHost(hostname, port);

	sendRequest(server, hostname, port, path, method);

	const clock_t startTime = clock();
	
	#define RESPONSE_SIZE 32768
	char response[RESPONSE_SIZE+1];
	char *p = response, *q;
	char *end = response + RESPONSE_SIZE;
	char *body = 0;

	enum {length, chunked, connection};
	int encoding = 0;
	int remaining = 0;

	while(1){
		if((clock()-startTime)/ CLOCKS_PER_SEC>TIMEOUT){
			fprintf(stderr,"timeout after %.2f seconds.\n",TIMEOUT);
			return 1;
		}

		if(p == end){
			fprintf(stderr,"out of buffer space\n");
			return 1;
		}

		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(server, &reads);

		struct timeval timeout;
		timeout.tv_sec =0;
		timeout.tv_usec = 200000;

		if(select(server+1,&reads,0,0,&timeout)<0){
			fprintf(stderr,"select() failed.(%d)\n",GETSOCKETERRNO());
			return 1;
		}
		
		if(FD_ISSET(server,&reads)){
			int bytes_received = recv(server, p, end-p,0);
			if(bytes_received<1){
				if(encoding == connection && body){
					printf("%.*s",(int)(end-body),body);
				}
				printf("\nConnection closed by peer.\n");
				break;
			}

			p+=bytes_received;
			*p = 0;

			if(!body && (body = strstr(response,"\r\n\r\n"))){

				*body = 0;
				body += 4;
				printf("Received Headers:\n%s\n",response);
				
				q = strstr(response,"\nContent-Length:");
				if(q){
					encoding = length;
					q = strchr(q,' ');
					q += 1;
					remaining = strtol(q,0,10);
				}
				else{
					q = strstr(response,"\nTranfer-Encoding: chunked");
					if(q){
						encoding = chunked;
						remaining = 0;
					}
					else{
						encoding = connection;
					}
				}
				printf("\nReceived Body:\n");
			}

			if(body){
				if(encoding ==length){
					if(p-body >= remaining){
						printf("%.*s", remaining, body);
						break;
					}
					else if(encoding == chunked){
						
						do{
							
							if(remaining == 0){
								if((q = strstr(body,"\r\n"))){
									remaining = strtol(body,0,16);
									if(!remaining) goto finish;
									body = q + 2;
								}else{
									break;
								}
							}

							if(remaining && p - body >= remaining){
								printf("%.*s",remaining, body);
								body += remaining + 2;
								remaining = 0;
							}
						}while(!remaining);
					}
				}
			}

		}
	}
finish:
	printf("\nClosing socket.\n");
	CLOSESOCKET(server);

	printf("Finished.\n");
	return 0;
}
