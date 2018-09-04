#include "SocketHandler.h"	// SocketHandler interface
#include <iostream>			// cout/cerr interfaces
#include <string>			// String interface
#include <sstream>			// StringStream interface 
#include <fstream>			// ifstream/ofstream interface

// Constant expressions/Defines to be used throughout Server Class //
constexpr unsigned MAX_BUFFER_SIZE = (49152);		// Buffer size of the Incoming/Outgoing messages

// Initialize Winsock returning whether startup has succeeded
bool SocketHandler::initializeWinsock()
{
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsInit = WSAStartup(ver, &wsData);
	if (WSAStartup(ver, &wsData) != 0)
	{
		std::cerr	<< WSAGetLastError()
					<< "\nCan't Initialize Winsock! Closing Server..." << std::endl;
		return FALSE;
	}
	return TRUE;
}

// Create a listener socket to handle new clients
SOCKET SocketHandler::createListenSocket()
{
	// Create a temperary socket to initialize
	// If all steps succeed, only then pass temp into SocketHandlers member variable
	SOCKET tempListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (tempListenSocket == INVALID_SOCKET)
	{
		// Listener socket not created, must close server as it cannot continue without it
		std::cerr	<< WSAGetLastError()
					<< "\nCan't Create a Socket! Quitting" << std::endl;
		WSACleanup();
		exit(-1);
	}

	// Bind an IP Address and port to a Socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;			// Socket belongs to IPv4 family
	hint.sin_port = htons(m_port);		// Bind port number to socket
	hint.sin_addr.s_addr = INADDR_ANY;	// 
	inet_pton(AF_INET, m_ipAddress.c_str(), &hint.sin_addr);
	int bindOK = bind(tempListenSocket, (sockaddr*)&hint, sizeof(hint));
	if (bindOK == SOCKET_ERROR)
	{
		// Listener socket not created, must close server as it cannot continue without it
		std::cerr	<< WSAGetLastError()
					<< "\nCan't Create a Socket! Quitting" << std::endl;
		WSACleanup();
		exit(-1);
	}

	// Tell Windows the Socket is for listening
	int listenOK = listen(tempListenSocket, SOMAXCONN);
	if (listenOK == SOCKET_ERROR)
	{
		std::cerr << "Can't Bind to Socket! Quitting" << std::endl;
		return -1;
	}

	return std::move(tempListenSocket);
}

void SocketHandler::disconnectClient(SOCKET quittingClient)
{
	closesocket(quittingClient);
	FD_CLR(quittingClient, &master);
	std::cout << getClientID(quittingClient) << " has closed their client" << std::endl;
}

int SocketHandler::sendVerifyHandshake(SOCKET verificationSocket, int iResult)
{
	char rec[3];
	if (recv(verificationSocket, rec, 3, 0) < 0)
	{
		// The client sent an error code, or has closed, must close program
		disconnectClient(verificationSocket);
	}
	return iResult;
}

// Send file over connection to client
void SocketHandler::sendFile(std::string &filePath, unsigned client)
{
	// Send new file command to client to prep for file transfer
	std::cout << "Sending File..." << std::endl;
	sendVerifyHandshake(client, send(client, "~New File~", 11, 0));
	
	// Retrieving the file name with extension from the file path, send to client
	std::string fileName = filePath.substr(filePath.find_last_of('\\') + 1);
	std::ifstream inputFile(filePath, std::ios::in | std::ios::ate | std::ios::binary);
	
	if (inputFile.is_open())
	{
		// When file can be opened, send client the filename
		sendVerifyHandshake(client, send(client, fileName.c_str(), fileName.size() +1, 0));
		
		// Retrieve total file size and send to client for processing
		int fileSize = (int)inputFile.tellg();
		inputFile.seekg(std::ios::beg);
		char fileSizeBuffer[1024];
		_itoa_s(fileSize, fileSizeBuffer, 10);
		sendVerifyHandshake(client, send(client, fileSizeBuffer, strlen(fileSizeBuffer), 0));

		// Send data from file in 1024 byte chunks
		unsigned char buffer[1024];
		for ( ; fileSize > 0; fileSize -= 1024)
		{
			if (fileSize >= 1024)
			{
				inputFile.read(reinterpret_cast<char *>(buffer), 1024);
				sendVerifyHandshake(client, send(client, (char*)buffer, sizeof(buffer), 0));
			}
			else
			{
				inputFile.read(reinterpret_cast<char *>(buffer), fileSize);
				sendVerifyHandshake(client, send(client, (char*)buffer, fileSize, 0));
			}
		}
	}
	inputFile.close();
	return;
}

// Create new sockets, and recieve/handle input from clients
void SocketHandler::run()
{
	// Create a Listening Socket to accept input from clients
	listenSocket = createListenSocket();
	if (listenSocket == INVALID_SOCKET)
	{
		// Listener socket not created, must close server as it cannot continue without it
		std::cerr	<< WSAGetLastError()
					<< "Unable to create a listener socket! Closing server." << std::endl;
		WSACleanup();
		exit(-1);
	}

	// Clear any issues with file descriptor container, add newly created listener to container
	FD_ZERO(&master);
	FD_SET(listenSocket, &master);

	// Wait for input from sockets
	while (true)
	{
		// Create a copy of file descriptor to preserve flags of master
		fd_set copy = master;

		// Find the number of sockets with awaiting messages
		unsigned socketWaitingCount = select(0, &copy, nullptr, nullptr, nullptr);

		// Loop through every socket that is waiting
		for (unsigned inputSocket = 0; inputSocket < socketWaitingCount; ++inputSocket)
		{
			// For the currently active socket, take the clients socket and username for processing
			SOCKET currentSocket = copy.fd_array[inputSocket];
			std::string& userName = getClientID(currentSocket);

			if (currentSocket == listenSocket)
			{
				// The listenSocket has found a new client wanting to connect
				acceptNewClient();
			}
			else
			{
				// Handle client input buffer to broadcast to all existing users
				char inputBuffer[MAX_BUFFER_SIZE];
				ZeroMemory(inputBuffer, MAX_BUFFER_SIZE);
				int bytesIn = recv(currentSocket, inputBuffer, MAX_BUFFER_SIZE, 0);
				if (bytesIn == SOCKET_ERROR || bytesIn < 0)
				{
					// Client has disconnected, print to server window, and close their socket
					disconnectClient(currentSocket);
					break;
				}
				if (strcmp(inputBuffer, "file") == 0)
				{
					MessageRecievedHandler(this, currentSocket, inputBuffer);
				}
				else
				{
					
				}
			}
		}
	}
	return;
}

//Broadcast message to all clients
void SocketHandler::broadcastMSG(SOCKET currentSocket, std::string msg)
{
	// Send message to other clients, and definiately NOT the listening socket
	for (unsigned i = 0; i < master.fd_count; ++i)
	{
		SOCKET outputSocket = master.fd_array[i];
		if (outputSocket != listenSocket && outputSocket != currentSocket)
		{
			send(outputSocket, msg.c_str(), msg.size() + 1, 0);
			std::cout << msg;
		}
	}
}

// Accept incoming client, retrieve information, and welcome new client
void SocketHandler::acceptNewClient()
{
	// Accept a new connection
	sockaddr_in newClient;
	int clientSize = sizeof(newClient);
	SOCKET newClientSocket = accept(listenSocket, (sockaddr*)&newClient, &clientSize);
	if (newClientSocket == INVALID_SOCKET)
	{
		// New Client unable to connect, return without creating a new client objects
		std::cerr	<< WSAGetLastError()
					<< "Client unable to connect.\n" << std::endl;
		return;
	}

	// Print new clients remote and service name to server window
	char host[NI_MAXHOST];				// Client's remote name
	char service[NI_MAXSERV];			// Service (i.e. port) the client is connected on
	memset(host, 0, NI_MAXHOST);
	memset(service, 0, NI_MAXSERV);	
	if (getnameinfo((sockaddr*)&newClient, sizeof(newClient), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
	{
		std::cout << host << " connected on port " << service << std::endl;
	}
	else
	{
		inet_ntop(AF_INET, &newClient.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(newClient.sin_port) << std::endl;
	}

	// Add the new connection to the list of connected clients
	FD_SET(newClientSocket, &master);
	clientID[newClientSocket];

	NewClientHandler(this, newClientSocket);

	return;
}