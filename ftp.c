#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define MAXCHUNK 2048000;

int main(int argc, char *  argv[]) { 

int serverport = atoi(argv[1]);

int sockfd;
sockfd = ftp(serverport);
}

int ftp(int serverport) { 

	//Address of the FTP server to be connected	
	struct sockaddr_in ftpaddr;
	bzero(&ftpaddr, sizeof(ftpaddr));
	ftpaddr.sin_family = AF_INET;
	ftpaddr.sin_port = htons(21);
	inet_pton(AF_INET, "128.112.136.60", &ftpaddr.sin_addr); 
	
	//Creating a control socket to connect to the ftp server 
	int ftpsock;
	ftpsock = socket(AF_INET,SOCK_STREAM, 0);

	//Connecting the control socket to the ftp server on port 21
	int res = connect(ftpsock, (struct sockaddr *) &ftpaddr, sizeof(ftpaddr));
	
	char buf[8192];

	if (res < 0) { 
	printf(" Failed to connect to socket \n ");
        exit(0);
	} 

    if(recv(ftpsock, buf, 8192, 0) >= 1) {
	printf("received: %s \n", buf);
    }

    if(strncmp(buf, "220", 3) == 0) {
                bzero(buf, 8192);
	   printf("Connection is open, moving to send \n");
    } else {
        printf("Error, line 62\n");
        exit(0);
    }

    if(send(ftpsock, "USER anonymous\n", 15, 0) >= 1) {
        printf("sent back \"USER anonymous\" \n");
    } else {
        printf("Error, line 69\n");
        exit(0);
    }

    if(recv(ftpsock, buf, 8192, 0) >= 1) {
        printf("Received: %s\n", buf);
    } else {
        printf("error, line 76\n");
        exit(0);
    }

    if(strncmp(buf, "331", 3) == 0) {
        bzero(buf, 8192);
        printf("Successfully sent username, moving to password\n");
    } else {
        exit(0);
    }
	
	if(send(ftpsock, "PASS rr574@njit.edu\n", 20, 0) >= 1) {
        printf("Sent\n");
    } else {
        exit(0);
    }
    
    char code[4];
    
    if(recv(ftpsock, buf, 8192, 0)) {
        strncpy(code, buf, 3);
        printf("Received: %s", buf);
        bzero(buf, 8192);
        while(recv(ftpsock, buf, 8192, 0) >= 1) {
            printf("Received: %s", buf);
            if(strstr(buf, "230 ")) {
                bzero(buf, 8192);
                break;
            } else {
                bzero(buf, 8192);
            }
        }
    } else {
        exit(0);
    }

    if(strncmp(code, "230", 3) == 0) {
        bzero(buf, 8192);
        printf("Successfully logged onto the server\n");
	} else {
	    exit(0);
    }

 	if(send(ftpsock, "PASV\n", 5, 0) >= 1) {
	printf("Sent\n");
	} else {
	exit(0);
	}

	if(recv(ftpsock, buf, 8192, 0) >= 1) {
	printf("Received: %s", buf);

	}
		
	if(strncmp(buf, "227", 3) == 0) { 
        printf("Ready to begin data transfer\n"); 
	} else {
        printf("Did not receive 227 after passive\n");
        exit(0);
	}
    
    printf("Attempting to parse last message.\n");

    //parsing return of 227 to get ipaddr and port of the server 

    char str1[50];
    char ip[15];
    int i=0;
    int count=0;
    int cc = 0;
    int pos=0;
    int pos2=0;
    char port[10];
    char p1[50];
    char p2[50];
	
    char * s = strchr(buf, '(');
    char * e = strchr(buf, ')');
    strncpy(str1, s + 1, e - s - 1);

    for (i=0; i<=strlen(str1); i++){
    if (str1[i] == ',') {
    cc++;
    }

    if (str1[i] == ',' && count < 3) {      
    str1[i] = '.';      
    count++;
    }

    if (cc==3  && count ==3) {																			         pos = i;																			
    }																						
    }
    
    for (i=0; i<=pos; i++){
    ip[i] = str1[i];
    }

    for (i=pos+2; i<=strlen(str1); i++){
    port[i-(pos+2)] = str1[i];
    }

    bzero(p1, 50);
    bzero(p2, 30);

    for (i=0; i<=strlen(port);i++){
    if (port[i] == ','){
    break;
    }
    p1[i] = port[i];
    }
	
    for (i=strlen(p1)+1; i<=strlen(port); i++){
    p2[i-(strlen(p1)+1)] = port[i];
    }
	
    int s1 = atoi(p1);
    int s2 = atoi(p2);
    int num = 256;

    int mul = s1 * num;
    int portn = mul + s2;
    printf("port is %d", portn);
	
    //Creating datasocket	
    int datasock; 
    datasock = socket(AF_INET, SOCK_STREAM, 0);
		
    //Creating server address with port and ip address returned by the server		
    struct sockaddr_in dataaddr;
    bzero(&dataaddr, sizeof(dataaddr));
    dataaddr.sin_family = AF_INET;
    dataaddr.sin_port = htons(portn);
    inet_pton(AF_INET, ip, &dataaddr.sin_addr);

    //Connecting datasocket to the ftp server
    int res2 = connect(datasock, (struct sockaddr *) &dataaddr, sizeof(dataaddr));
    if (res2 < 0) {
            printf(" Failed to connect to socket \n ");
	    exit(0);
    }
    else { 
    printf("Connected to data port\n");
    }
    
    FILE *samplefile;
    int bytesRead = 0;
    int bytesWritten = 0;
    int fileSize = 0;
    struct timeval tv;

    // switch the comments to try the larger file
//    if(send(ftpsock, "RETR /pub/cs226/netflix.zip\n", 28, 0) >= 1) {
    if(send(ftpsock, "RETR /reports/2016/984.pdf\n", 27, 0) >= 1) {
        printf("Sent retrieve request\n");
    } else {
        printf("Error on retrieve request");
        exit(0);
    }
    
    if(recv(ftpsock, buf, 8192, 0) >= 1) {
        printf("Receiving: %s \n", buf); 
    }

    int fres = 0;
    fd_set rfds;
    // open the file before the loop
    samplefile = fopen("samplefile", "w");
    int done = 0;
    
    while (!done) {
        // set up our group of files to watch
        FD_ZERO(&rfds);
        FD_SET(ftpsock, &rfds);
        FD_SET(datasock, &rfds);
        // set timeout (not working as expected)
        //tv.tv_sec = 1;
        //tv.tv_usec = 0;
    
        // check if any of them have readable data
        fres = select(datasock + 1, &rfds, NULL, NULL, NULL);
        
        if(fres > 0) {
            // check if it's the control port
            if(FD_ISSET(ftpsock, &rfds)) {
                // it's probably the end of file signal
                // set done to true (liable to break...)
                done = 1;
            }

            // check the data port - read more into the file
            if(FD_ISSET(datasock, &rfds)) {
                bzero(buf, 8192);
                bytesRead = read(datasock, buf, 8192);
                if (bytesRead > 0) {
                    fileSize = fileSize + bytesRead; 
                    bytesWritten = fwrite(buf, 1, bytesRead, samplefile);
printf("Bytes read: %d, bytes written, %d, total filesize: %d\n",
       bytesRead, bytesWritten, fileSize);

                } else {
                    //printf("couldn't read anything from the socket\n");
                }
            }
        } else {
            // neither one is ready - loop again
        }
    }
    if(recv(ftpsock, buf, 8192, 0) >= 1) {
        printf("Server should have a done message for us\nReceiving: %s \n", buf);
    }

    if(strncmp(buf, "226", 3) == 0) {
    printf(" Yes the server has sent everything \n");
    }
    close(datasock);
	
    if(send(ftpsock, "QUIT \n", 7, 0) >= 1) {
    printf("closing connection to the server \n");
    }
    close(ftpsock);

} 

