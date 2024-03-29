#include "ftclient.h"
	

int sock_control; 


/**
 * Receive a response from server
 * Returns -1 on error, return code on success
 */
int read_reply(){
	int retcode = 0;
	if (recv(sock_control, &retcode, sizeof retcode, 0) < 0) { /* >0 success =0 connection closed*/
		perror("client: error reading message from server\n");
		return -1;
	}	
	return ntohl(retcode);
}



/**
 * Print response message
 */
void print_reply(int rc) 
{
	switch (rc) {
		case 220:
			printf("220 Welcome, server ready.\n");
			break;
		case 221:
			printf("221 Goodbye!\n");
			break;
		case 226:
			printf("226 Closing data connection. Requested file action successful.\n");
			break;
		case 550:
			printf("550 Requested action not taken. File unavailable.\n");
			break;
	}
	
}




/**
 * Parse command in cstruct
 */ 
int ftclient_read_command(char* buf, int size, struct command *cstruct)
{
	memset(cstruct->code, 0, sizeof(cstruct->code));
	memset(cstruct->arg, 0, sizeof(cstruct->arg));
	
	printf("ftclient> ");	// prompt for input		
	fflush(stdout); 	

	// wait for user to enter a command
	read_input(buf, size);
		
	char *arg = NULL;
	arg = strtok (buf," "); 	/* cut buf by " " */
	

	if (arg != NULL){
		// store the argument if there is one
		strncpy(cstruct->arg, arg, strlen(arg));
	}

	// buf = command
	if (strcmp(buf, "list") == 0) {
		strcpy(cstruct->code, "LIST");		
	}
	else if (strcmp(buf, "get") == 0) {
		strcpy(cstruct->code, "RETR");		
	}
	else if (strcmp(buf, "quit") == 0) {
		strcpy(cstruct->code, "QUIT");		
	}
/*---------v1------------10.23*/
	else if (strcmp(buf, "mkdr") == 0) {
		strcpy(cstruct->code, "MKDR");
	}
	else if (strcmp(buf, "load") == 0) {
		strcpy(cstruct->code, "LOAD");
	}
	else if (strcmp(buf, "cddr") == 0) {
		strcpy(cstruct->code, "CDDR");
	}
/*---------end-----------10.23*/
	else {//invalid
		return -1;
	}
	
	// store code in beginning of buffer
	memset(buf, 0, 400);
	strcpy(buf, cstruct->code);
	
	arg = strtok (NULL, " ");
	// if there's an arg, append it to the buffer
	if (arg != NULL) {
		strcat(buf, " ");
		strncat(buf, cstruct->arg, strlen(cstruct->arg));
		printf("==/check: %s/==",&buf);
	}
	
	return 0;
}



/**
 * Do get <filename> command 
 */
int ftclient_get(int data_sock, int sock_control, char* arg)
{
    char data[MAXSIZE];
    int size;
    FILE* fd = fopen(arg, "w");
    
    while ((size = recv(data_sock, data, MAXSIZE, 0)) > 0) {
        fwrite(data, 1, size, fd);
    }

    if (size < 0) {
        perror("error\n");
    }

    fclose(fd);
    return 0;
}


/**
 * Open data connection
 */
int ftclient_open_conn(int sock_con)
{
	int sock_listen = socket_create(CLIENT_PORT_ID);

	// send an ACK on control conn
	int ack = 1;
	if ((send(sock_con, (char*) &ack, sizeof(ack), 0)) < 0) {
		printf("client: ack write error :%d\n", errno);
		exit(1);
	}		

	int sock_conn = socket_accept(sock_listen);
	close(sock_listen);
	return sock_conn;
}




/** 
 * Do list commmand
 */
int ftclient_list(int sock_data, int sock_con)
{
	size_t num_recvd;			// number of bytes received with recv()
	char buf[MAXSIZE];			// hold a filename received from server
	int tmp = 0;

	// Wait for server starting message
	if (recv(sock_con, &tmp, sizeof tmp, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	while ((num_recvd = recv(sock_data, buf, MAXSIZE, 0)) > 0) {
        	printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	
	if (num_recvd < 0) {
	        perror("error");
	}

	// Wait for server done message
	if (recv(sock_con, &tmp, sizeof tmp, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}
	return 0;
}



/**
 * Input: cmd struct with an a code and an arg
 * Concats code + arg into a string and sends to server
 */
int ftclient_send_cmd(struct command *cmd)
{
	char buffer[MAXSIZE];
	int rc;

	sprintf(buffer, "%s %s", cmd->code, cmd->arg);
	
	// Send command string to server
	/* from arg1 to another end with buffer in size */
	rc = send(sock_control, buffer, (int)strlen(buffer), 0);	
	if (rc < 0) {
		perror("Error sending command to server");
		return -1;
	}
	
	return 0;
}

/**
 *  Send mkdr command to server
 */
void ftclient_mkdr(int data_sock, int sock_control, struct command *cs)
{
	ftclient_send_cmd(&cs);
	int ret_code = read_reply();
	if (ret_code == 550)
		printf("Failed to make a directory.");
}

/**
 *  Upload a file to server
 */
void ftclient_load(int data_sock, char* arg)
{
	int fd = open(cmd.arg, O_RDONLY);
	char buffer[];
	int rc;
	struct stat st;
	
	if (fp == NULL) 
	{
		perror("Error file name");
	}
	stat(arg, &st);
	int len = st.st_size;
	if (sendfile(data_sock, fd, NULL, len) < 0)
	{
		perror("Sending error");
	}
}

/**
 *  CD directory in server
 */
void ftclient_cd(int data_sock, int sock_control, char* arg)
{
	char buffer[MAXSIZE];
	sprintf(buffer, "%s", cmd->arg);
	
	int rc = send(data_sock, buffer, (int)strlen(buffer), 0);
	if (rc < 0)
	{
		perror("Error sending command to server");
	
	}
}

/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login()
{
	struct command cmd;
	char user[256];
	memset(user, 0, 256);

	// Get username from user
	printf("Name: ");	
	fflush(stdout); 		
	read_input(user, 256);

	// Send USER command to server
	strcpy(cmd.code, "USER");
	strcpy(cmd.arg, user);
	ftclient_send_cmd(&cmd);
	
	// Wait for go-ahead to send password
	int wait;
	recv(sock_control, &wait, sizeof wait, 0);

	// Get password from user
	fflush(stdout);	
	char *pass = getpass("Password: ");	

	// Send PASS command to server
	strcpy(cmd.code, "PASS");
	strcpy(cmd.arg, pass);
	ftclient_send_cmd(&cmd);
	
	// wait for response
	int retcode = read_reply();
	switch (retcode) {
		case 430:
			printf("Invalid username/password.\n");
			exit(0);
		case 230:
			printf("Successful login.\n");
			break;
		default:
			perror("error reading message from server");
			exit(1);		
			break;
	}
}





int main(int argc, char* argv[]) 
{		
	int data_sock, retcode, s;
	char buffer[MAXSIZE];
	struct command cmd;	
	struct addrinfo hints, *res, *rp;

	if (argc != 3) {
		printf("usage: ./ftclient hostname port\n");
		exit(0);
	}

	char *host = argv[1];
	char *port = argv[2];

	// Get matching addresses
	memset(&hints, 0, sizeof(struct addrinfo));
	/*
	struct addrinfo{
		int       ai_flags; (an indication sign used as a parameter in fuction 'getaddrinfo')
		int       ai_family;
		int       ai_socketype;
		int       ai_protocol;
		size_t    ai_addrlen;
		char*     ai_canonname;
		struct sockaddr* ai_addr;
		struct addrinfo* ai_next;
	}
	
	struct sockaddr{
		unsigned short sa_family; (address family , AF_xxx, usually AF_INET)
		char sa_data[14]; (14 B of protocal address)
	}
	*/
	hints.ai_family = AF_UNSPEC;		// uncertain
	hints.ai_socktype = SOCK_STREAM;	// TCP
	
	s = getaddrinfo(host, port, &hints, &res);	// 0 == success
	if (s != 0) {
		printf("getaddrinfo() error %s", gai_strerror(s));
		exit(1);
	}
	
	// Find an address to connect to & connect
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		/* 'socket' function returns >=0 --> success ; -1 --> error */
		sock_control = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (sock_control < 0)
			continue;
		/* connect() connect arg1 to arg2, 0-->success ; -1-->error with reasons in errno*/
		/* addrlen : lenth of struct sockaddr */
		if(connect(sock_control, res->ai_addr, res->ai_addrlen)==0) {
			break;
		} else {
			perror("connecting stream socket");
			exit(1);
		}
		close(sock_control);
	}
	freeaddrinfo(rp);


	// Get connection, welcome messages
	printf("Connected to %s.\n", host);
	print_reply(read_reply()); 
	

	/* Get name and password and send to server */
	ftclient_login();

	while (1) { // loop until user types quit
		
		// Get a command from user
		if ( ftclient_read_command(buffer, sizeof buffer, &cmd) < 0) {
			printf("Invalid command\n");
			continue;	// loop back for another command
		}

		// Send command to server
		if (send(sock_control, buffer, (int)strlen(buffer), 0) < 0 ) {
			close(sock_control);
			exit(1);
		}

		retcode = read_reply();		
		if (retcode == 221) {
			/* If command was quit, just exit */
			print_reply(221);		
			break;
		}
		
		if (retcode == 502) {
			// If invalid command, show error message
			printf("%d Invalid command.\n", retcode);
		} else {			
			// Command is valid (RC = 200), process command
		
			// open data connection
			if ((data_sock = ftclient_open_conn(sock_control)) < 0) {
				perror("Error opening socket for data connection");
				exit(1);
			}			
			
			// execute command
			if (strcmp(cmd.code, "LIST") == 0) {
				ftclient_list(data_sock, sock_control);
			} 
			else if (strcmp(cmd.code, "RETR") == 0) {
				// wait for reply (is file valid)
				if (read_reply() == 550) {
					print_reply(550);		
					close(data_sock);
					continue; 
				}
				ftclient_get(data_sock, sock_control, cmd.arg);
				print_reply(read_reply()); 
			}
			else if (strcmp(cmd.code, "CDDR") ==0 ) {
				if (read_reply() == 550) {
					print_reply(550);
					close(data_sock);
					continue;
				};
				ftclient_cd(data_sock, cmd.arg);
				print_reply(read_reply()); 
			}
			
			else if (strcmp(cmd.code, "MKDR") == 0) {
			        if (read_reply() == 550) {
			                print_reply(550);
			                close(data_sock);
			                continue;
			        }
			        ftclient_mkdr(cmd);
			        print_reply(read_reply());
			        continue;
			}
			else if (strcmp(cmd.code, "LOAD") == 0) {
				if (read_reply() == 550) {
					print_reply(550);
					close(data_sock);
					continue;
				}
				ftclient_load(data_sock, sock_control, cmd.arg);
				print_reply(read_reply());
				continue;
			}
			close(data_sock);
		}

	} // loop back to get more user input

	// Close the socket (control connection)
	close(sock_control);
    return 0;  
}
