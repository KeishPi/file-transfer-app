# file-transfer-app
1) Server multi-threaded: a new thread is created for each client
2) Username/password access to the server: <br \>
	username: client <br \>
	password: pass <br \>
3) Allow client to change directories (cd): see instructions below
4) Transfer other files:
	Not well tested, but worked with an mp4 file. Didn't seem to 
	work with a .pdf or executable, program would hang for some reason.

Put ftserver.c and ftclient.py in separate folders/directories <br \>
To run ftserver.c: <br \>
    Go to the directory containing ftserver.c and the makefile <br \>

	TO COMPILE Enter the following on the command line:
	make
	OR
	gcc -g ftserver.c -o ftserver -lpthread

	TO RUN Enter the following on the command line:
	./ftserver <port#>
	Example: ./ftserver 5888

To run ftclient.py: <br \>
	ftserver must be already running <br \>
        Go to the directory containing ftclient.py <br \>

	Enter one of the following on the command line:
		ftclient.py <server_host> <ctrl_port> -l <data_port>
		Example (-l): ftclient.py localhost 5988 -l 5989
		ftclient.py <server_host> <ctrl_port> -g <filename> <data_port>
		Example (-g): ftclient.py localhost 5988 -g test.txt 5989
		ftclient.py <server_host> <ctrl)port> cd <directory>
		Example (cd): ftclient.py localhost 5988 cd ftDir

Instructions: <br \>
The server is run first and waits for connections from clients. When a client connects the server and client establish a TCP control connection. The client will send a username/password to the server and the server verifies or sends an error message.  If the username/password is valid, the client can then send a command to the server (see above). The server then initiates a TCP data connection and completes the request or reports an error, at which the connection is closed. The server will keep listening to client connections until the server it receives a SIGINT.

Citations: <br \>
    Computer Networking: A Top-Down Approach, 6th ed., Kurose & Ross <br \>
    See ftserver.c and ftclient.py headers for specific websites used <br \>
