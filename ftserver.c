/*******************************************************************************************
 * Author:		Keisha Arnold
 * Filename: 	ftserver.c
 * Description: A simple file transfer application utilizing the sockets API.
 *              This is the server program, it waits for connections from clients
 *              and when a client connects it establishes a TCP control connection.
 *              When the client sends a command to the server, the server initiates a
 *              TCP data connection and completes the request or reports and error at
 *				which point the connection is closed.  The server keeps listening for 
 *				client connections until a SIGINT is received.
 * Citations:   gethostname: http://www.retran.com/beej/getnameinfoman.html
 *				multi-threaded: 
 *	https://stackoverflow.com/questions/6990888/c-how-to-create-thread-using-pthread-create-function
 *	https://stackoverflow.com/questions/13505340/passing-a-file-descriptor-to-a-thread-and-using-it-in-the-functionhomework
 *	http://man7.org/linux/man-pages/man3/pthread_create.3.html
 ********************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEBUG 0
#define BUF_LEN	2048


/*******************************************************************************************
 * Function: 		void error(const char *msg)
 * Description:		Error function used for reporting issues
 * Parameters:		char *msg, message printed to stderr
 * Pre-Conditions: 	msg must be initialized
 * Post-Conditions: 	msg prints to stderr and program exits with status 0
 ********************************************************************************************/
void error(const char *msg) {
	perror(msg);
}

/*********************************************************************************************
 * Function: 		int serverSocketInit(int portNum)
 * Description:		Sets up the server socket sockaddr_in struct
 * Parameters:      int portNum, specified on command line: argv[1]
 * Pre-Conditions: 	Valid port number given on command line
 * Post-Conditions: Returns an initialized, binded sockaddr_in struct
 **********************************************************************************************/
int serverSocketInit(int portNum) {
	struct sockaddr_in serverAddress;
	int listenSocketFD;
	
	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNum); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process
	
	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if(listenSocketFD < 0) {
		perror("ERROR opening socket");
		exit(1);
	}
	
	// Connect socket to port
	if(bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}
	
	return listenSocketFD;
}

/***********************************************************************************************
 * Function: 		int recvMessage(int socketFD, char* buff)
 * Description:		Function to receive messages from the client through the control socket.
 * Parameters:		int socketFD, char* buff
 * Pre-Conditions: 	The server must be running at the specified socket,
 *					the server and client are connected with a dedicated control socket.
 * Post-Conditions: The server receives a message from the server, or the server closes
 *					the connection.
 ************************************************************************************************/
char* recvMessage(int socketFD, char* buff) {
	int charsRecv;
	
	// Get return message from server
	memset(buff, '\0', BUF_LEN);
	if(DEBUG) {
		printf("sizeof(buff): %lu\n", sizeof(buff));
	}
	
	// Read data from the socket, leaving \0 at end
	charsRecv = recv(socketFD, buff, (BUF_LEN - 1), 0);
	if(DEBUG) {
		printf("%s\n", buff);
	}
	
	if(charsRecv == 0) {
		printf("No data received, closing the connection...\n");
		return NULL;
	}
	if(charsRecv < 0) {
		printf("ERROR receiving data.\n");
		return NULL;
	}
	
	return buff;
}

/***********************************************************************************************
 * Function: 		int sendMessage(int socketFD, char* userHandle, , char* serverHandle)
 * Description:		Function to send messages to the server through the connected socket
 *			        until the client closes the connection with "\quit".
 * Parameters:		int socketFD, char* userHandle, , char* serverHandle
 * Pre-Conditions: 	The server must be running at the specified socket,
 *                  userHandle initialized with user specified handle,
 *			        serverHandle initialized with user specified handle,
 *			        the server and client are connected with a dedicated socket.
 * Post-Conditions: The client sends a message to the server, or closes the connection.
 ************************************************************************************************/
int sendMessage(int socketFD, char* buff) {
	int charsWritten;
	
	// send message to client
	charsWritten = send(socketFD, buff, strlen(buff), 0);
	
	if(charsWritten < 0) {
		error("ERROR writing to socket");
	}
	
	if (charsWritten < strlen(buff)) {
		send(socketFD, &buff[charsWritten], strlen(buff) - charsWritten, 0);
	}
    
	return charsWritten;
}

/*******************************************************************************************
 * Function:        int openDataSocket(int dataPort, char* host)
 * Description:		Function to set up the server's data transfer socket
 * Parameters:		The client's hostname and data transfer port
 * Pre-Conditions: 	The server and client are ready to exchange data through the socket
 * Post-Conditions: The server's data transfer socket is initialized and connected to the client
 * Returns:         The data socket, connected to the client process.
 ********************************************************************************************/
int initTCPDataConnection(int dataPort, char* host) {
	struct sockaddr_in serverAddress;
	struct hostent *serverHost;
	int dataSocketFD;
	
	//set up the server address struct
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));  // Clear out the address struct
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(dataPort);  // Store the port number
	serverHost = gethostbyname(host);  // Convert the machine name into a special form of address
	// Copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHost->h_addr, serverHost->h_length);
	
	// Set up the socket
	sleep(1); //wait for client
	dataSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (dataSocketFD < 0) {
		error("ERROR opening socket");
	}
	
	// Connect socket to address
	if (connect(dataSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) != 0) {
		error("ERROR on connecting data socket");
	}
	
	return dataSocketFD;
}

/*******************************************************************************************
 * Function:        void listCmd(int dataPort, char* host)
 * Description:		Function to send current directory contents to the client
 * Parameters:		The client's hostname and data transfer port
 * Pre-Conditions: 	The client is ready to receive the directory through the data socket
 * Post-Conditions: The server sent the requested directory contents to the client
 *                  and closes out the the data socket (created within this function)
 ********************************************************************************************/
void listCmd(int dataPort, char* host) {
	int dataSocket;
	int dataWritten;
	DIR* directory;
	struct dirent *dirList;
	char sendBuffer[BUF_LEN];
	memset(sendBuffer, '\0', sizeof(sendBuffer));
	
	fflush(stdout);
	printf("Sending directory list to %s:%d\n", host, dataPort);
	fflush(stdout);
	
	dataSocket = initTCPDataConnection(dataPort, host);
	directory = opendir("."); //from current directory
	
	if(directory != NULL) {
		 //build dir list
		while((dirList = readdir(directory))) {
			strcat(sendBuffer, dirList->d_name);
			strcat(sendBuffer, " ");
		}
		//send dir list
		sendMessage(dataSocket, sendBuffer);
	}
	else error("ERROR: unable to open directory.");
	
	closedir(directory);
	close(dataSocket);
}

/*******************************************************************************************
 * Function:        void sendFile(int dataPort, char* fileName, char* host)
 * Description:		Function to send the requested file to the client
 * Parameters:		The client's hostname, data transfer port, and file name of wanted file
 * Pre-Conditions: 	The client is ready to receive the file through the data socket
 * Post-Conditions: The server sent the requested file to the client and closes out the
 *                  the data socket (created within this function)
 ********************************************************************************************/
void sendFile(int dataPort, char* fileName, char* host) {
	int fileFD;
	int dataSocket;
	int fileSize;
	int pos;
	int dataWritten;
	int dataRead;
	int dataSent = 0;
	char sendBuffer[BUF_LEN];
	
	fflush(stdout);
	printf("Sending \"%s\" to %s:%d\n", fileName, host, dataPort);
	fflush(stdout);
	
	//set up data connection
	dataSocket = initTCPDataConnection(dataPort, host);
	memset(sendBuffer, '\0', sizeof(sendBuffer));
	
	//open the file and copy it to the buffer
	fileFD = open(fileName, O_RDONLY);
	if (fileFD < 1) { //error
		printf("Error opening %s\n", fileName);
		strncpy(sendBuffer, "ERROR: File not found/could not be opened\n", sizeof(sendBuffer));
		sendMessage(dataSocket, sendBuffer);
		return;
	}
	//get file size, then reset file pointer
	fileSize = lseek(fileFD, 0, SEEK_END);
	if(DEBUG) {
		printf("file size: %d bytes\n", fileSize);
	}
	pos = lseek(fileFD, 0, SEEK_SET);
	
	//small files: up to 1MB
	if (fileSize < 1000000) {
		char fileBuffer[fileSize + 1];  //one extra for null terminator
		memset(fileBuffer, '\0', sizeof(fileBuffer));
		dataRead = read(fileFD, fileBuffer, sizeof(fileBuffer)-1);
		if(DEBUG) {
			printf("file bytes read: %d bytes\n", dataRead);
		}
		//send the file
		dataSent = 0;
		if (fileSize < BUF_LEN) {
			sendMessage(dataSocket, fileBuffer);
		}
		else {  //fileSize is bigger than send buffer
			while (dataSent <= fileSize){
				if ((fileSize - dataSent) < BUF_LEN) {
					memset(sendBuffer, '\0', sizeof(sendBuffer));
					//memcpy for non text files
					memcpy(sendBuffer, &fileBuffer[dataSent], fileSize - dataSent);
					dataWritten = sendMessage(dataSocket, sendBuffer);
					dataSent += dataWritten;
					if(DEBUG){
						printf("bytes sent(now): %d\nbytes sent(total): %d\n", dataWritten, dataSent);
					}
					break;
				}
				//break up the fileBuffer
				memset(sendBuffer, '\0', sizeof(sendBuffer));
				memcpy(sendBuffer, &fileBuffer[dataSent], BUF_LEN-1);
				
				//send
				dataWritten = sendMessage(dataSocket, sendBuffer);
				dataSent += dataWritten;
				if(DEBUG){
					printf("bytes sent(now): %d\nbytes sent(total): %d\n", dataWritten, dataSent);
				}
			}
		}
		close(fileFD);
	}
	//big files: 100's MB
	else {
		close(fileFD);
		size_t fileSize;
		ssize_t partSize;
		char* filePart = NULL;
		
		//open file
		FILE* file = fopen(fileName, "r");
		
		//send file in pieces
		while ((partSize = getline(&filePart, &fileSize, file)) != -1) {
			dataSent = send(dataSocket, filePart, partSize, 0);
			if (dataSent < 0) {
				error("ERROR writing to socket");
				break;
			}
		}
		free(filePart);
		fclose(file);
	}
	close(dataSocket);
}


/*******************************************************************************************
 * Function:        int verifyUser(char* clientLogin)
 * Description:		Function to verify the connecting client
 * Parameters:		A concatenated string containing the username & password from client
 * Pre-Conditions: 	char* clientLogin initialized with username & password from client
 * Post-Conditions: Returns 1 if there isa  match, otherwise returns 0
 ********************************************************************************************/
int verifyUser(char* clientLogin) {
	printf("Verifying user... ");
	
	if (strcmp(clientLogin, "clientpass") != 0) {
		printf("\n... username/password failed.\n");
		return 0;
	}
	else {
		printf("\n... username/password verified.\n");
		return 1;
	}
}

/*********************************************************************************************
 * Function: 		void* ftp_work(void* arg)
 * Description:		in main, pthread_create() uses this to start execution.  It is the method
 *					where all the file transfer work is done between server and client
 * Parameters:		pointer to the data that ftp_work will use, passed in via the
 *					fourth parameter in pthread_create()
 * Pre-Conditions: 	The server must be running at the specified socket socket,
 *                  the struct sockaddr_in must be properly initialized
 * Post-Conditions: The client and server successfully connect or it throws an error and exits
 **********************************************************************************************/
void* ftp_work(void* socketFD) {
	int serverSocket;
	int controlSocket;
	int dataPort;
	int verify_cd;
	char buffer[BUF_LEN];
	char commandLine[BUF_LEN];
	char newDir[256];
	char fileName[256];
	char client[64];
	char clientIP[20];
	char cwd[1024];
	char* token = NULL;
	struct sockaddr_in serverAddress, clientAddress;
	struct stat findFile;
	memset(buffer, '\0', sizeof(buffer));
	memset(commandLine, '\0', sizeof(commandLine));
	memset(newDir, '\0', sizeof(newDir));
	memset(fileName, '\0', sizeof(fileName));
	memset(client, '\0', sizeof(client));
	memset(clientIP, '\0', sizeof(clientIP));
	
	//cast the socket file descriptor
	controlSocket = *(int*)socketFD;
	
	//receive client login info
	recvMessage(controlSocket, buffer);
	
	//verify client
	if (verifyUser(buffer)) {
		memset(buffer, '\0', sizeof(buffer));
		strncpy(buffer, "User verified!", sizeof(buffer));
		sendMessage(controlSocket, buffer);
	}
	else {
		memset(buffer, '\0', sizeof(buffer));
		strncpy(buffer, "Verification failed: username/password incorrect", sizeof(buffer));
		sendMessage(controlSocket, buffer);
		return NULL;
	}
	
	//receive command from the client
	memset(buffer, '\0', sizeof(buffer));
	recvMessage(controlSocket, buffer);
	
	//copy buffer into commandLine to parse for commands
	strcpy(commandLine, buffer);
	if(DEBUG) {
		printf("commandLine: %s\n", commandLine);
	}
	
	//first strtok call parses first part of msg (<server name>)
	token = strtok(commandLine, " \n\0");
	strcpy(client, token);
	if(DEBUG) {
		printf("client: %s\n", client);
	}
	
	//2nd strtok call parses second part of msg (<client IP address>)
	token = strtok(NULL, " \n\0");
	strcpy(clientIP, token);
	printf("Servicing client %s\n", clientIP);
	
	//3rd strtok call parses third part of msg (<command>, can be -l -g)
	token = strtok(NULL, " \n\0");
	if(DEBUG) {
		printf("Command: %s\n", token);
	}
	
	//command -l (list)
	if(strcmp(token, "-l") == 0) {
		//4th strtok call parses fourth part of msg (<data port>)
		token = strtok(NULL, " \n\0");
		dataPort = atoi(token);	// Get the port number, convert to an integer from a string
		printf("List directory requested on port %d\n", dataPort);
		listCmd(dataPort, clientIP);
	}
	
	//command -g (get)
	else if(strcmp(token, "-g") == 0) {
		//4th strtok call parses fourth part of msg (<filename>)
		token = strtok(NULL, " \n\0");
		strcpy(fileName, token);
		//5th strtok call parses fifth part of msg (<data port>)
		token = strtok(NULL, " \n\0");
		dataPort = atoi(token);	 // Get the port number, convert to an integer from a string
		printf("File %s requested on port %d\n", fileName, dataPort);
		
		if(stat(fileName, &findFile) == 0) {
			memset(buffer, '\0', sizeof(buffer));
			sprintf(buffer, "Transferring file: %s...", fileName);
			write(controlSocket, buffer, strlen(buffer));
			sendFile(dataPort, fileName, clientIP);
		}
		else {
			printf("ERROR: file stat error. Sending error message to %s:%d\n", clientIP, dataPort);
			memset(buffer, '\0', sizeof(buffer));
			sprintf(buffer, "ERROR: file not found, unable to open %s", fileName);
			sendMessage(controlSocket, buffer);
		}
	}
	
	//command cd (change directory)
	else if (strncmp(token, "cd", 2) == 0) {
		//4th strtok call parses fourth part of msg (<directory>)
		token = strtok(NULL, " \n\0");
		strcpy(newDir, token);
		printf("Server directory change to \"%s\" requested\n", newDir);
		verify_cd = chdir(newDir);
		
		if(verify_cd == 0) {
			printf("Directory successfully changed.\n");
			memset(cwd, '\0', sizeof(cwd));
			if(getcwd(cwd, sizeof(cwd)) != NULL) {
				printf("Current Working Dir: %s\n", cwd);
			}
			else {
				perror("getcwd() error\n");
			}
			memset(buffer, '\0', sizeof(buffer));
			sprintf(buffer, "Server Current Directory: %s", cwd);
			write(controlSocket, buffer, strlen(buffer));
		}
		else {
			error("ERROR changing directories");
			memset(buffer, '\0', sizeof(buffer));
			sprintf(buffer, "ERROR: directory change failure");
			write(controlSocket, buffer, strlen(buffer));
		}
		
	}
	return NULL;
}


/*******************************************************************************************
 * Function: 		int main(int argc, char *argv[])
 * Description:		main function of the client gets CL arguments, sets up server address,
 *			        connects to the socket, gets the user's handle, completes the TCP
 *			        handshake, send messages and receives messages from the server, closes
 *			        the connection when done chatting.
 * Parameters:		argv[0] is the program filename
 *                  argv[1] is the server's hostName (localhost)
 *                  argv[2] is the server's port #
 * Pre-Conditions: 	The server must be running at the specified socket
 * Post-Conditions: The two hosts exchange messages, when the client or server quits the
 *			        socket is closed and the program terminates.
 ********************************************************************************************/
int main(int argc, char *argv[]) {
	int portNumber;			//server port #
	int listenSocket;		//server listening socket
	int establishedConnect;	//client socket
	
	// for getnameinfo()
	char host[1024];
	char service[20];
	
	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	socklen_t sizeOfClientInfo;	//size of client address
	pthread_t newThread;	//multi-threaded

	// Check usage & args
	if (argc != 2) {
		fprintf(stderr,"USAGE: %s <port number>\n", argv[0]);
		exit(1);
	}

	// Get the port number, convert to an integer from a string
	portNumber = atoi(argv[1]);
	
	// Set up server address struct, connect socket to port
	listenSocket = serverSocketInit(portNumber);
	
	// Flip the socket on- can now receive up to 5 connections
	listen(listenSocket, 5);
	printf("FTServer listening on port %d...\n", portNumber);
	
	// Get the size of the address for the client that will connect
	sizeOfClientInfo = sizeof(clientAddress);

	while(1) {
		// Accept connections from clients
		establishedConnect = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		if(establishedConnect < 0) {
			error("ERROR on accept");
		}
		
		// Get client's hostname
		getnameinfo(&clientAddress, sizeof(clientAddress), host, sizeof(host), service, sizeof(service), 0);
		printf("Connection established with %s\n", host);
		
		// New thread for each client
		// https://stackoverflow.com/questions/6990888/c-how-to-create-thread-using-pthread-create-function
		// 1) pointer to pthread_t structure which will be filled with info
		// 2) attributes for thread (NULL- default)
		// 3) start_routine (a function pointer)- where the new thread starts execution
		// 4) argument passed to start_routine
		if(pthread_create(&newThread, NULL, ftp_work, (void*)&establishedConnect) < 0) {
			error("ERROR creating thread");
		}
	}
	
	// Close the socket
	close(listenSocket);
	printf("Connection closed. Goodbye!\n");
 
	return 0;
}
