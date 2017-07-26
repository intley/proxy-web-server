/* Programming Project 1
 * CS 656 106
 * WG1: Daniel Pavlick, Ian Hall, Rahul Rajendran, Ronald Anazco
 * 
 * Required function authors (with varying contributions from other members):
 * init(): Ian Hall
 * go(): Ian Hall
 * parse(): Daniel Pavlick
 * dnslookup(): Daniel Pavlick, Rahul Rajendran
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

// maximum string size
#define MAXBYTES 65535

int init(int serverPort);
int create_socket();
void error_out(char * err);
struct sockaddr_in create_server_addr();
void bind_wrap(int fd, struct sockaddr *serveraddr,socklen_t serveraddrlen);

void go(int sockfd);
void start_listening(int fd);
int accept_wrap(int sockfd,struct sockaddr *sockaddr,socklen_t *clientaddrlen);
int read_from_socket(int fd,char * buff,int max);

int add_new_content(char * data);
int parse(char haystack[], char domain[], char port[], char path[]);
int dnslookup(char * host, char * port, char ip[]);

void debugPrint(char message[]);

int debug = 0;

int main(int argc, char * argv[]){

	// check for valid port number
	if (argc < 2) {
		error_out("Please supply a port number.");
	}

	int serverPort = atoi(argv[1]);
	if (serverPort == 0 || serverPort > 65535) {
		error_out("Invalid port number specified.");
	}

	// enable debug mode if requested
	if (argc == 3) {
		debug = 1;
	}

	printf("Stage 1 program by ra74 listening on port (%s)\n", argv[1]);

	// Create socket
	int sockfd;
	sockfd = init(serverPort);

	// Process connections
	go(sockfd);

}

int init(int serverPort) {

	// Create socket
	int sockfd;
	sockfd = create_socket();

	// Set server address and port
	// These will be casted to sockaddr (general-purpose socket struct) from sockaddr_in (IP-specific socketaddr struct)
	struct sockaddr_in serveraddr;
	serveraddr = create_server_addr(serverPort);

	// Bind socket
	bind_wrap(sockfd,(struct sockaddr *) &serveraddr,sizeof(serveraddr));
	return sockfd;
}

// create listening socket for server
int create_socket(){
	int sock;
	sock = socket(AF_INET,SOCK_STREAM,0);
	if (sock < 0){
		error_out("Failed to create socket");
	}
	return sock;
}

void error_out(char * err){
	fprintf(stdout,"%s%s",err,"\n");
	exit(1);
}

// configure server addr/port to bind socket to
struct sockaddr_in create_server_addr(int serverPort){
	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET; // set addr family
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // bind to all available interfaces
	serveraddr.sin_port = htons(serverPort); // set port #
	return serveraddr;
}

// bind socket to server addr/port
void bind_wrap(int fd, struct sockaddr *serveraddr,socklen_t serveraddrlen){
	int res = bind(fd,serveraddr,serveraddrlen);
	if (res < 0){
		error_out("Error binding socket");
	}
}

void go(int sockfd){

	int connfd,res;

	// Begin listening
	struct sockaddr_in clientaddr;
	start_listening(sockfd);

	// Listen for and accept connections
	socklen_t clientaddrlen;
	for ( ; ; ){
		// Accept connection
		clientaddrlen=sizeof(clientaddr);
		connfd=accept_wrap(sockfd,(struct sockaddr *) &clientaddr,&clientaddrlen);

		// Receive data
		char received_data[MAXBYTES];
		res = read_from_socket(connfd,received_data+strlen(received_data),MAXBYTES); // read data
		if (res < 0){
			close(connfd);
			memset(received_data,0,MAXBYTES);
			continue;
		}
		debugPrint(received_data);

		char * response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"; // always generate 200 for now
		char response[MAXBYTES];
		int res;
		res = add_new_content(received_data); // parse it
		if (res == 0) {
			// successfully parsed
			// Generate response
			// Add valid response header first
			strcpy(response,response_header);
			strcat(response,received_data);
			write(connfd,response,strlen(response)); // send it back with new data
		} else {
			debugPrint("Error with adding new content.\n");
		}

		// clean up after successful or unsuccessful responses
		close(connfd);
		memset(received_data,0,MAXBYTES);
		memset(response,0,MAXBYTES);
	}
}

// listen on server socket
void start_listening(int fd){
	int backlog = 1024;
	int res = listen(fd, backlog);
	if (res < 0){
		error_out("Error listening on socket");
	}
}

// try to accept connection
int accept_wrap(int sockfd,struct sockaddr *sockaddr,socklen_t *clientaddrlen){
	int res;
	debugPrint("Accepting...\n");
	for ( ; ; ){
		if ((res = accept(sockfd,sockaddr,clientaddrlen)) < 0){
			// if protocl error or software error, retry connecting
			// otherwise fail out
			if (errno == EPROTO || errno == ECONNABORTED){
				continue;
			}
			else{
				fprintf(stdout,"unable to accept\n");
			}
		}
		else {
			break;
		}
	}
	debugPrint("Accepted\n");
	return res;
}

// read socket data until max size is reached or \r\n\r\n is all that remains
int read_from_socket(int fd,char * buff,int max){
	int res;
	int curr = 0;
	while ( curr < max ){
		res = read(fd,buff+curr,max);
		if (res <= 0){
			return 0;
		}
		// This handles telnet, where requests are line-buffered, so each read gets back a single line
		if (strcmp(buff+curr,"\r\n") == 0){
			return 0;
		}
		curr+=res;
		// This handles browser, where request is sent in one buffer
		if (strcmp(buff+curr-4,"\r\n\r\n") == 0){
			printf("%s\n",buff);
			return 0;
		}
	}
	return -1;
}

// append parsed content
int add_new_content(char * data) {

	// containers for the parse function to fill with the
	// appropriate information
	char domain[MAXBYTES];
	char port[MAXBYTES];
	char path[MAXBYTES];
	char ipaddr[MAXBYTES];

	// open and close pre tags
	char * preopen = "<pre>";
	char * preopenstyle = "<pre style=\"color:red;\">";
	char * preclose = "</pre>";

	int res;
	res = parse(data, domain, port, path);
	if (res != 0) {
		debugPrint("error encountered while parsing.\n");
		return -1;
	}

	res = dnslookup(domain,port,ipaddr);
	if (res != 0) {
		debugPrint("Error encountered on DNS lookup.\n");
		return -1;
	}

	// Wrap parsed data in <pre> tags
	char tmp[MAXBYTES];
	strcpy(tmp,preopen);
	strcat(tmp,data);
	strcat(tmp,preclose);
	strcpy(data,tmp);

	// Generate red text, with proper stylized pre tag and obtained IP address
	sprintf(data+strlen(data),"%s  HOSTIP =  %s (%s)\n",preopenstyle,domain,ipaddr);
	sprintf(data+strlen(data),"  PORT =  %s\n",port);
	sprintf(data+strlen(data),"  PATH =  %s\n%s",path,preclose);

	return 0;
}

// parses content into files
int parse(char haystack[], char domain[], char port[], char path[]) {

	// first check for a valid http/ftp request
	// (i.e. "GET http/ftp::// ...")
	int needle;

	if (strncmp(haystack, "GET http://",11) == 0){
		needle = 11;
	}
	else if (strncmp(haystack, "GET ftp://",10) == 0){
		needle = 10;
	}
	else {
		debugPrint("Error with parsing request.\n");
		return -1;
	}
          	
	// zero out containers
	memset(domain, 0, MAXBYTES);
	memset(port, 0, MAXBYTES);
	memset(path, 0, MAXBYTES);

	// scan through the domain
	while(haystack[needle] != '/' && 
			haystack[needle] != ' ' &&
			haystack[needle] != ':') {
		needle++;
	}

	// compute domain length and copy the content
	int length = needle - 11;
	strncpy(domain, haystack + 11, length);

	int portBegin = needle;
	// the port begins with a : or is not specified
	if (haystack[portBegin] != ':') {
		// not specified, set to 80
		strncpy(port, "80", 2);
	} else {
		// we have a colon, scan for the port number
		portBegin++;
		needle++;
		while(haystack[needle] != '/' && haystack[needle] != ' ') {
			needle++;
		}

		// compute port length and copy the content
		length = needle - portBegin;
		strncpy(port, haystack + portBegin, length);
	}

	int pathBegin = needle;
	// the path begins with a / or is not specified
	if (haystack[pathBegin] != '/') {
		// no path specified, set to root ("/")
		strncpy(path, "/", 1);
	} else {
		// we have a slash, scan for the full path
		while(haystack[needle] != ' ') {
			needle++;
		}

		// compute path length and copy the content
		length = needle - pathBegin;
		strncpy(path, haystack + pathBegin, length);
	}

	return 0;
}

int dnslookup(char host[], char port[], char ip[]) {
	/* contains guidelines for what addresses to retrieve */
	struct addrinfo hints;
	/* will be filled with information about the host */
	struct addrinfo *nsadd;
	/*retrieves ipv4 addresses */
	hints.ai_family=AF_INET; 
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=0;
	hints.ai_flags=AI_PASSIVE;

	memset(ip, 0, MAXBYTES);

	/* the first parameter would be the hostname or IP address
	 * second parameter is the port no. */
	int res;
	res = getaddrinfo(host, port, &hints, &nsadd);
	if (res != 0) {
		debugPrint("Error with getting address info.\n");
		return -1;
	}

	/* containers for information from res struct */
	char hostname[MAXBYTES],ipaddr[256],service[256];
	/* retrieve's ip address
	 * service will get overwritten in the next block */
	res = getnameinfo(nsadd->ai_addr,nsadd->ai_addrlen,
			ipaddr,sizeof(ipaddr),
			service,sizeof(service),
			NI_NUMERICHOST);
	if (res != 0) {
		debugPrint("Error with retrieving server name info.\n");
		return -1;
	}

	res = getnameinfo(nsadd->ai_addr,nsadd->ai_addrlen,
			hostname,sizeof(hostname),
			service,sizeof(service),
			NI_NUMERICSERV);
	if (res != 0) {
		debugPrint("Error with retrieving server name info.\n");
		return -1;
	}

	memset(host, 0, MAXBYTES);
	strncpy(host, hostname, MAXBYTES);
	strncpy(ip,ipaddr,256);

	return 0;
}

// convenience function for printing non-critical error messages
// only when in debug mode
void debugPrint(char message[]) {
	if (debug) {
		printf("%s", message);
	}
}
