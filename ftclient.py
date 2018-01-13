#!/usr/bin/python

#****************************************************************************************#
# Author:		Keisha Arnold
# Filename:		ftclient.py
# Description:	A simple file transfer application utilizing the sockets API.
#				This is the client program, it connects to a server application at a
#				port # specified on the command line.
#				The server application is run first and listens for clients,
#				The client sends a username/password to the server for
#				verification. If verified, the client can send a command to
#				the server. The server will complete the request or 
#				respond with an error.
# Sources :		get socket IP:
#				https://stackoverflow.com/questions/58294/how-do-i-get-the-external-ip-of-a-socket-in-python
#				CS 344 (Intro to Operating Systems) Lectures
#				CS 372 (Into to Computer Networks) Lectures
#****************************************************************************************#

from socket import *
import sys
import fcntl
import string
import fileinput
import threading
import os.path

#****************************************************************************************#
# Function:			validateCL()
# Description:		validates the command line arguments
# Parameters:		command line arguments
# Pre-Conditions:	none
# Post-Conditions:	prints a message if args/usage incorrect, then exits
#****************************************************************************************#
def validateCL():
    if (len(sys.argv) < 5 or len(sys.argv) > 6):
	print("usage: ftclient.py <host> <ctrl port> <-l> <data port>")
	print("       ftclient.py <host> <ctrl port> <-g> <filename> <data port>")
	print("       ftclient.py <host> <ctrl port> <cd> <path>")
	exit(1)
	#valid ports [1024, 49151]
    elif (int(sys.argv[2]) < 1024 or int(sys.argv[2]) > 49151):
        print("Invalid ctrl port. Use port [1024, 49151].")
        exit(1)
    elif (sys.argv[3] == "-l" and (int(sys.argv[4]) < 1024 or int(sys.argv[4]) > 49151)):
        print("Invalid data port. Use port [1024, 49151].")
        exit(1)
    elif (len(sys.argv) == 6 and (int(sys.argv[5]) < 1024 or int(sys.argv[5]) > 49151)):
        print("Invalid data port. Use port [1024, 49151].")
        exit(1)
    elif (sys.argv[3] != "-l" and sys.argv[3] != "-g" and sys.argv[3] != "cd"):
        print("command {0} not recognized".format(sys.argv[3]))
        exit(1)


#****************************************************************************************#
# Function:			ctrlConnectServer()
# Description:		Establishes a TCP control connection on <ctrl port>
# Parameters:		None
# Pre-Conditions:	The server must be running at the specified socket
# Post-Conditions:	Client socket is set up and connected to the server
#****************************************************************************************#
def ctrlConnectServer():
    serverHost = sys.argv[1]
    serverPort = int(sys.argv[2])
    clientSocket = socket(AF_INET, SOCK_STREAM)
    clientSocket.connect((serverHost, serverPort))
    return clientSocket


#****************************************************************************************#
# Function:			verifyUser(ctrlSocket)
# Description:		Prompts the user to input a username and password,
#					sends the username/password to the server for verification
# Parameters:		ctrlSocket
# Pre-Conditions:	The server must be running at the specified socket,
#					the server and client must have an established TCP control connection
# Post-Conditions:	The server replies verifying the user or if username/password
#					was invalid the program exits
#****************************************************************************************#
def verifyUser(ctrlSocket):
	user = raw_input("Username: ")
	password = raw_input("Password: ")
	verify = user+password
    	ctrlSocket.send(verify.encode("utf-8"))
	reply = ctrlSocket.recv(128)
	replyMsg = reply.decode("utf-8")
	print (replyMsg)
	if "fail" in replyMsg:
		ctrlSocket.close()
		sys.exit(0)


#****************************************************************************************#
# Function:			sendCommand(ctrlSocket)
# Description:		Sends the client IP address and the command to the server
# Parameters:		ctrlSocket
# Pre-Conditions:	The server must be running at the specified socket,
#					the server and client must have an established TCP control connection
# Post-Conditions:	Client IP address and command sent to the server
#****************************************************************************************#
def sendCommand(ctrlSocket):
	#get client address
	sckt = socket(AF_INET, SOCK_DGRAM)
	sckt.connect(("8.8.8.8", 80))
	clientIP = sckt.getsockname()[0] #retuns a tuple, get the 1st element[0]
	
	if len(sys.argv) != 6:
		cmd = sys.argv[1] + " " + clientIP + " " + sys.argv[3] + " " + sys.argv[4]
	else:
		cmd = sys.argv[1] + " " + clientIP + " " + sys.argv[3] + " " + sys.argv[4] + " " + sys.argv[5]

	ctrlSocket.send(cmd.encode("utf-8"))


#****************************************************************************************#
# Function:			dataSocketSetup(dataPort)
# Description:		Sets up data socket to receive TCP data connection from the server
# Parameters:		dataPort, specified on command line
# Pre-Conditions:	The server must be running at the specified socket,
#					the server and client must have an established TCP control connection
# Post-Conditions:	Data socket is set up and server connects
#****************************************************************************************#
def dataSocketSetup(dataPort):
    dataSocket = socket(AF_INET, SOCK_STREAM)
    dataSocket.bind(('', dataPort))
    dataSocket.listen(1)
    connection, address = dataSocket.accept()
    return connection


#****************************************************************************************#
# Function:			getDirList(dataSocket)
# Description:		Receives requested directory list from the server
# Parameters:		dataSocket
# Pre-Conditions:	The server must be running at the specified socket,
#					the server and client must have an established TCP control connection,
#					the server and client must have an established TCP data connection
# Post-Conditions:	Client receives directory list and prints it to console
#****************************************************************************************#
def getDirList(dataSocket):
	dirData = dataSocket.recv(10000)
	directory = dirData.split()
	for fileName in directory:
		print(fileName.decode("utf-8"))


#****************************************************************************************#
# Function:			getfile(dataSocket, filename)
# Description:		Receives requested file from the server
# Parameters:		dataSocket, filename
# Pre-Conditions:	The server must be running at the specified socket,
#					the server and client must have an established TCP control connection,
#					the server and client must have an established TCP data connection
# Post-Conditions:	Client receives requested file and writes it to current directory
#****************************************************************************************#
def receiveFile(dataSocket, fileName):
    if ".txt" in fileName:
		#open file in write mode
        file = open(fileName, "w")
        dataRecv = dataSocket.recv(10000)
		#loop while still recv data
        while (dataRecv):
            fileBuffer = dataRecv.decode("utf-8")
            file.write(fileBuffer)
            dataRecv = dataSocket.recv(10000)
            if not dataRecv:
				break
            if len(dataRecv) == 0:
				break
        file.close()
			
    else:
		#open file in write+binary mode
		#b differentiates btwn text & binary files
        file = open(fileName, "wb")
        dataRecv = dataSocket.recv(10000)
        while (dataRecv):
            file.write(dataRecv)
            dataRecv = dataSocket.recv(10000)
            if not dataRecv:
				break
            if len(dataRecv) == 0:
				break
        file.close()


#****************************************************************************************#
if __name__ == "__main__":
	# check usage & args
	validateCL()
		
	# establish TCP control connection
	ctrlSocket = ctrlConnectServer()
	
	# send client username/password to server
	verifyUser(ctrlSocket)
	
	# send command to server
	sendCommand(ctrlSocket)
	
	# command -1: get directory list
	if sys.argv[3] == "-l":
        	dataPort = int(sys.argv[4])
        	dataSocket = dataSocketSetup(dataPort)
        	print("Receiving requested directory list from {0}:{1}".format(sys.argv[1], dataPort))
        	getDirList(dataSocket)
        	dataSocket.close()
	#command -g: get file
    	elif sys.argv[3] == "-g":
        	fileName = sys.argv[4]
        	dataPort = int(sys.argv[5])
        	print("Reqesting file transfer: {0} from {1}:{2}".format(fileName, sys.argv[1], dataPort))
        	fileStat = ctrlSocket.recv(1024)
        	print("Message from {0}:{1}: {2}".format(sys.argv[1], sys.argv[2], fileStat.decode("utf-8")))
        	if "ERROR" not in fileStat.decode("utf-8"):
            		dataSocket = dataSocketSetup(dataPort)
            		if os.path.exists(fileName):
                		choice = raw_input("Duplicate Filename. \nEnter: (R)Rename File, (O)Overwrite Existing, or (C)Cancel\nSelection: ")
				if choice[0] == "R" or choice[0] == "r":
					rename = raw_input("Rename File: ")
					receiveFile(dataSocket, rename)
				elif choice[0] == "O" or choice[0] == "o":
					receiveFile(dataSocket, fileName)
                		elif choice[0] == "C" or choice[0] == "c":
                    			print("File Transfer Cancelled. Goodbye.")
                    			ctrlSocket.close()
                    			dataSocket.close()
                    			sys.exit(0)
            		else:
                		receiveFile(dataSocket, fileName)
            		print("File Transfer Complete.")
            		dataSocket.close()
		#command cd: change directory
    	elif sys.argv[3] == "cd":
        	newDir = sys.argv[4]
        	msg = ctrlSocket.recv(1024)
        	print(msg.decode("utf-8"));

    	ctrlSocket.close()









