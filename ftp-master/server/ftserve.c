#include "ftserve.h"

int pidArray[100]={0};
int Vu=0;

/**
 * Autokill Added
 * Last Edit: Oct.23
*/
int main(int argc, char *argv[])
{
	int sock_listen, sock_control, port, pid;

	if (argc != 2) {
		printf("usage: ./ftserve port\n");
		exit(0);
	}

	port = atoi(argv[1]);

	// create socket
	if ((sock_listen = socket_create(port)) < 0 ) {
		perror("Error creating socket");
		exit(1);
	}

	while(1) {	// wait for client request

		// create new socket for control connection
		if ((sock_control = socket_accept(sock_listen))	< 0 )
			break;

		// create child process to do actual file transfer
		if ((pid = fork()) < 0) {
			perror("Error forking child process");
		} else if (pid == 0) {
			close(sock_listen);
			int temp=(int)getpid();
			pidArray[Vu]=temp;
			Vu++;
			ftserve_process(sock_control);
			close(sock_control);
			kill((int)getpid(),-9);
			exit(0);
		}

		close(sock_control);
	}

	close(sock_listen);

	return 0;
}

/**
 * Upload Function
 * Target filename = dirname.
 * Return 226 to USR if SUCCESS. 550 on FAIL.
 * Last Edit: Oct.23
*/
void ftserve_upld(int sock_control, int sock_data, char* dirname)
{
    char data[MAXSIZE];
    int size;
    FILE* fd = fopen(dirname, "w");

    while ((size = recv(data_sock, data, MAXSIZE, 0)) > 0) {
        fwrite(data, 1, size, fd);
    }

    if (size < 0) {
        lt=get_localtime()
        printf ( "%d/%d/%d %d:%d:%d ",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
        printf ( "Error receiving from USRPID=%d" , getpid());
    }

    fclose(fd);
    return 0;
}

/**
 * Change work directory.
 * Return 226 to USR if SUCCESS. 550 on FAIL.
 * Local return 0 on SUCCESS, 1 on FAIL.
 * Last Edit: Oct.23
*/
int ftserve_cddr(int sock_control, int sock_data, char* dirname)
{
    int status=-1;

    if(dirname==".")
        return 0;
    if(dirname=="..")
    {
        status=chdir("..");
        if(status=0)
        {
            send_response(sock_control, 226);
            ftserve_list(sock_data,sock_control);
            return 0;
        }
        else
            send_response(sock_control, 550);
            return 1;
    }

    status=chdir("/"+dirname);
    if(status=0)
    {
        send_response(sock_control, 226);
        ftserve_list(sock_data,sock_control);
        return 0;
    }
    else
        send_response(sock_control, 550);
        return 1;
}


/**
 * Refresh server status without running process and root.
 * Return 226 to USR if SUCCESS. 550 on FAIL.
 * Last Edit: Oct.23
*/
void hdsk(int sock_control, int sock_data)
{
    for(int i=0;i<Vu-1;i++)
    {
        if(pidArray[i]!=0)
            kill(pidArray[i],-9);
    }
    printf("\n Server refreshed by user-pid %d",getpid());
    send_response(sock_control, 226);
}


/**
 * Function-Make directories
 * Return -1 on error, 0 on success
 * Return 226 to USR if SUCCESS. 550 on FAIL.
 * Last Edit: Oct.16
*/
int ftserve_mkdr(int sock_control, int sock_data, char* dirname)
{
	pid_t status;

	string temp = dirname;
	status = system('mkdir'+temp);
	if (-1 == status)
    {
        send_response(sock_control, 550);
    }
    else if(0 == WIFEXITED(status))
    {
        send_response(sock_control, 226);
    }
    else
    {
        send_response(sock_control, 550);
        return -1;
    }
	ftserve_list(sock_data,sock_control);

	return 0;
}


/**
 * Function-Delete directories
 * Return -1 on error, 0 on success
 * Return 226 to USR if SUCCESS. 550 on FAIL.
 * Last Edit: Oct.16
*/
int ftserve_dldr(int sock_control, int sock_data, char* dirname)
{
    pid_t status;

	string temp = dirname;
	status = system('rm -r '+temp);
	if (-1 == status)
    {
        send_response(sock_control, 550);
        return -1;
    }
    else if(0 == WIFEXITED(status))
    {
        send_response(sock_control, 226);
    }
    else
    {
        send_response(sock_control, 550);
        return -1;
    }
	ftserve_list(sock_data,sock_control);

	return 0;
}


/**
 * Send file specified in filename over data connection, sending
 * control message over control connection
 * Handles case of null or invalid filename
 */
void ftserve_retr(int sock_control, int sock_data, char* filename)
{
	FILE* fd = NULL;
	char data[MAXSIZE];
	size_t num_read;

	fd = fopen(filename, "r");

	if (!fd) {
		// send error code (550 Requested action not taken)
		send_response(sock_control, 550);

	} else {
		// send okay (150 File status okay)
		send_response(sock_control, 150);

		do {
			num_read = fread(data, 1, MAXSIZE, fd);

			if (num_read < 0) {
                lt=get_localtime()
                printf ( "%d/%d/%d %d:%d:%d ",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
				printf("error in fread()\n");
			}

			// send block
			if (send(sock_data, data, num_read, 0) < 0)
            {
                lt=get_localtime()
                printf ( "%d/%d/%d %d:%d:%d ",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
                printf("error sending file\n");
            }


		} while (num_read > 0);

		// send message: 226: closing conn, file transfer successful
		send_response(sock_control, 226);

		fclose(fd);
	}
}





/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
int ftserve_list(int sock_data, int sock_control)
{
	char data[MAXSIZE];
	size_t num_read;
	FILE* fd;

	int rs = system("ls -l | tail -n+2 > tmp.txt");
	if ( rs < 0) {
		exit(1);
	}

	fd = fopen("tmp.txt", "r");
	if (!fd) {
		exit(1);
	}

	/* Seek to the beginning of the file */
	fseek(fd, SEEK_SET, 0);

	send_response(sock_control, 1); //starting

	memset(data, 0, MAXSIZE);
	while ((num_read = fread(data, 1, MAXSIZE, fd)) > 0) {
		if (send(sock_data, data, num_read, 0) < 0) {
			perror("err");
		}
		memset(data, 0, MAXSIZE);
	}

	fclose(fd);

	send_response(sock_control, 226);	// send 226

	return 0;
}






/**
 * Open data connection to client
 * Returns: socket for data connection
 * or -1 on error
 */
int ftserve_start_data_conn(int sock_control)
{
	char buf[1024];
	int wait, sock_data;

	// Wait for go-ahead on control conn
	if (recv(sock_control, &wait, sizeof wait, 0) < 0 ) {
		perror("Error while waiting");
		return -1;
	}

	// Get client address
	struct sockaddr_in client_addr;
	socklen_t len = sizeof client_addr;
	getpeername(sock_control, (struct sockaddr*)&client_addr, &len);
	inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf));

	// Initiate data connection with client
	if ((sock_data = socket_connect(CLIENT_PORT_ID, buf)) < 0)
		return -1;

	return sock_data;
}





/**
 * Authenticate a user's credentials
 * Return 1 if authenticated, 0 if not
 */
int ftserve_check_user(char*user, char*pass)
{
	char username[MAXSIZE];
	char password[MAXSIZE];
	char *pch;
	char buf[MAXSIZE];
	char *line = NULL;
	size_t num_read;
	size_t len = 0;
	FILE* fd;
	int auth = 0;

	fd = fopen(".auth", "r");
	if (fd == NULL) {
		perror("file not found");
		exit(1);
	}

	while ((num_read = getline(&line, &len, fd)) != -1) {
		memset(buf, 0, MAXSIZE);
		strcpy(buf, line);

		pch = strtok (buf," ");
		strcpy(username, pch);

		if (pch != NULL) {
			pch = strtok (NULL, " ");
			strcpy(password, pch);
		}

		// remove end of line and whitespace
		trimstr(password, (int)strlen(password));

		if ((strcmp(user,username)==0) && (strcmp(pass,password)==0)) {
			auth = 1;
			break;
		}
	}
	free(line);
	fclose(fd);
	return auth;
}





/**
 * Log in connected client
 */
int ftserve_login(int sock_control)
{
	char buf[MAXSIZE];
	char user[MAXSIZE];
	char pass[MAXSIZE];
	memset(user, 0, MAXSIZE);
	memset(pass, 0, MAXSIZE);
	memset(buf, 0, MAXSIZE);

	// Wait to recieve username
	if ( (recv_data(sock_control, buf, sizeof(buf)) ) == -1) {
		perror("recv error\n");
		exit(1);
	}

	int i = 5;
	int n = 0;
	while (buf[i] != 0)
		user[n++] = buf[i++];

	// tell client we're ready for password
	send_response(sock_control, 331);

	// Wait to recieve password
	memset(buf, 0, MAXSIZE);
	if ( (recv_data(sock_control, buf, sizeof(buf)) ) == -1) {
		perror("recv error\n");
		exit(1);
	}

	i = 5;
	n = 0;
	while (buf[i] != 0) {
		pass[n++] = buf[i++];
	}

	return (ftserve_check_user(user, pass));
}





/**
 * Wait for command from client and
 * send response
 * Returns response code
 */
int ftserve_recv_cmd(int sock_control, char*cmd, char*arg)
{
	int rc = 200;
	char buffer[MAXSIZE];

	memset(buffer, 0, MAXSIZE);
	memset(cmd, 0, 5);
	memset(arg, 0, MAXSIZE);

	// Wait to recieve command
	if ((recv_data(sock_control, buffer, sizeof(buffer)) ) == -1) {
		perror("recv error\n");
		return -1;
	}

	strncpy(cmd, buffer, 4);
	char *tmp = buffer + 5;
	strcpy(arg, tmp);

	if (strcmp(cmd, "QUIT")==0) {
		rc = 221;
	} else if((strcmp(cmd, "USER")==0) || (strcmp(cmd, "PASS")==0) ||
			(strcmp(cmd, "LIST")==0) || (strcmp(cmd, "RETR")==0) || 			(strcmp(cmd, "MKDR")==0) || (strcmp(cmd, "DLDR")==0)) {
		rc = 200;
	} else { //invalid command
		rc = 502;
	}

	send_response(sock_control, rc);
	return rc;
}






/**
 * Child process handles connection to client
 */
void ftserve_process(int sock_control)
{
	int sock_data;
	char cmd[5];
	char arg[MAXSIZE];

	// Send welcome message
	send_response(sock_control, 220);

	// Authenticate user
	if (ftserve_login(sock_control) == 1) {
		send_response(sock_control, 230);
	} else {
		send_response(sock_control, 430);
		exit(0);
	}

	while (1) {
		// Wait for command
		int rc = ftserve_recv_cmd(sock_control, cmd, arg);

		if ((rc < 0) || (rc == 221)) {
			break;
		}

		if (rc == 200 ) {
			// Open data connection with client
			if ((sock_data = ftserve_start_data_conn(sock_control)) < 0) {
				close(sock_control);
				exit(1);
			}

			// Execute command
			if (strcmp(cmd, "LIST")==0)
			{ // Do list
				ftserve_list(sock_data, sock_control);
			} else if (strcmp(cmd, "RETR")==0)
			{ // Do get <filename>
				ftserve_retr(sock_control, sock_data, arg);
			}else if (strcmp(cmd, "MKDR")==0)
			{
				ftserve_mkdr(sock_control, sock_data, arg);
			}else if (strcmp(cmd, "DLDR")==0)
			{
				ftserve_dldr(sock_control, sock_data, arg);
			}else if (strcmp(cmd, "HDSK")==0)
			{
				ftserve_hdsk(sock_control, sock_data, arg);
			}else if (strcmp(cmd, "CDDR")==0)
			{
				ftserve_cddr(sock_control, sock_data, arg);
			}else if (strcmp(cmd, "UPLD")==0)
			{
				ftserve_upld(sock_control, sock_data, arg);
			}

			// Close data connection
			close(sock_data);
		}
	}
}


