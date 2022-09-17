#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define LISTENER_IP "0.0.0.0"

#define BASE_RESPONSE "HTTP/1.1 200 OK\r\nServer: picohttp/0.1\r\nContent-Type: text/html\r\n\n"
#define NOT_FOUND "HTTP/1.1 404 Not Found\r\nServer: picohttp/0.1\r\nContent-Type: text/html\r\n\n<html>\n<head>\n<title>\n404 Not Found\n</title>\n</head>\n<body>\n<h3>\n404 Not Found\n</h3>\n</body>\n</html>\n"

char* craft_response(char* filename)
{
	char* response = malloc(8192);
		
	char* data = malloc(8192);
	
	FILE* fp;

	fp = fopen(filename, "r");

	if(fp != NULL)
	{
		size_t filesize = fread(data, 1, 8192, fp);
		
		memcpy(response, BASE_RESPONSE, strlen(BASE_RESPONSE));
		
		memcpy(response+strlen(BASE_RESPONSE), data, filesize);
		
		return response;
	}

	return "error";
}

int compare_filename(char* local_filename, char* request_filename)
{
	if(strcmp(request_filename, "/") != 0)
	{
		char* token = strtok(request_filename, "/");
		char* cleared_request_filename = strtok(token, "\0");
		if(strcmp(cleared_request_filename, local_filename) == 0)
		{
			return 0;
		}
		return -1;
	}
	return -1;
}

char* parse_request(char* request)
{
	char* request_type = strtok(request, " ");
	char* location = strtok(NULL, " ");
	char* http_version = strtok(NULL, "\n");

	if(strcmp(request_type, "GET") == 0 || strcmp(request_type, "POST") == 	0)
	{
		if(strcmp(http_version, "HTTP/1.1\r") == 0)
		{
			return location;	
		}
		return "false";
	}
	return "false";
}

void listener(int PORT, char* filename)
{
	struct sockaddr_in listenersock;						//Accessing the sockaddr_in structure in netinet/in.h

	struct sockaddr_in clientsock;							//Client structure
	int clientsize = sizeof(clientsock);						//The size of the client structure

	char* client_request = malloc(512);						//Allocated space for client's request

	int sockfd;									//Listener socket file descriptor

	int clientfd;									//Client socket file descriptor

	listenersock.sin_family = AF_INET;						//Family: AF_INET
	listenersock.sin_port = htons(PORT);						//Port number conversion from little endian to big endian
	listenersock.sin_addr.s_addr = inet_addr(LISTENER_IP);				//IP address conversion from dotted decimal notation to binary

	sockfd = socket(AF_INET, SOCK_STREAM, 0);					//Initializing the socket file descriptor

	//Check for errors
	if(sockfd != -1)
	{	
		//Bind the listnersock structure
		if(bind(sockfd, (const struct sockaddr *)&listenersock, sizeof(listenersock)) != -1)
		{	
		printf("Listening on %s:%i\n", LISTENER_IP, PORT);
			//Start listening, set backlog to -1 in order to not assign a limit to the pending connections queue
			while(listen(sockfd, -1) != -1)
			{
				//Accept the client and assign a file descriptor
				clientfd = accept(sockfd, (struct sockaddr *)&clientsock, (socklen_t *)&clientsize);
				if(clientfd != -1)
				{
					int clientPORT = ntohs(clientsock.sin_port);				//Client port number
					char* clientIP = inet_ntoa(clientsock.sin_addr);			//Client IP address
					printf("[+] Connection from %s:%i\n", clientIP, clientPORT);

					if(recv(clientfd, client_request, 512, 0) != -1)
					{
						printf("[+] %s\n", client_request);
						char* parsed_location = parse_request(client_request);
						if(strcmp(parsed_location, "false") != 0)
						{
							int comparison = compare_filename(filename, parsed_location);
							if(comparison == 0)
							{
								char* data = craft_response(filename);
								if(strcmp(data, "error") != 0)
								{
								if(send(clientfd, data, strlen(data), 0) != -1)
								{
									printf("[+] %s\n", data);
								}
								close(clientfd);
							    }
							    else
							    {
							    	if(send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0) != -1)
							    	{
							    		printf("[+] %s\n", NOT_FOUND);
							    		close(clientfd);
							    	}
							    }
							}
							else
							{
								if(send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0) != -1)
								{
									printf("[+] %s\n", NOT_FOUND);
									close(clientfd);
								}
							}
						}	
					}
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	if(argc == 3)
	{
		int PORT = atoi(argv[1]);
		if(PORT != 0)
		{
			char* filename = argv[2];
			listener(PORT, filename);
		}
		else
		{
			printf("Ports: 1-65535\n");
		}
	}
	else
	{
		printf("picohttp v0.1 by bigkahuna\n\nUsage: %s <PORT> <HTML FILE>\n", argv[0]);
	}
	return 0;
}
