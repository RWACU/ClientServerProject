#include "SocketHandler.h"	// SocketHandler interface
#include <iostream>			// cout/cerr interfaces
#include <string>			// String interface
#include <sstream>			// StringStream interface 

// Constant expressions/Defines to be used throughout Server Class //
constexpr unsigned MAX_BUFFER_SIZE = (49152);		// Buffer size of the Incoming/Outgoing messages
#define USERNAME users[inputSocket];				// Redefinition of Users Socket Related Name

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
			std::string& userName = users[currentSocket];
			if (currentSocket == listenSocket)
			{
				// The listenSocket has found a new client wanting to connect
				acceptNewClient();
			}
			else
			{
				// Handle client input buffer to broadcast to all existing users
				char inputbuffer[MAX_BUFFER_SIZE];
				ZeroMemory(inputbuffer, MAX_BUFFER_SIZE);
				int bytesIn = recv(currentSocket, inputbuffer, MAX_BUFFER_SIZE, 0);
				if (bytesIn == SOCKET_ERROR || bytesIn < 0)
				{
					// Client has disconnected, print to server window, and close their socket
					closesocket(currentSocket);
					FD_CLR(currentSocket, &master);
					std::cout << userName << " Has closed their client" << std::endl;
				}
				else
				{
					if (userName.size() == 0)
					{
						// The user entered a username into the buffer as their first input
						// Process buffer to obtain the first word, that will be their username

						// Take only first word of buffer as username
						std::stringstream singleLineName{ inputbuffer };
						singleLineName >> userName;
						userName = inputbuffer;
						
						// Broadcast new connection to all existing clients
						std::ostringstream welcomeNewConnectionMsg;
						welcomeNewConnectionMsg << "<SERVER> " << userName << " has joined the chat!\r\n";
						broadcastMSG(currentSocket, welcomeNewConnectionMsg.str());
						std::cout << currentSocket << " is " << userName << std::endl;
					}
					else
					{
						// Broadcast users message to all existing clients
						std::ostringstream output;
						output << userName << ": " << inputbuffer << "\r\n";
						broadcastMSG(currentSocket, output.str());
					}
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
	users[newClientSocket];

	// Send a welcome message to the connected client
	std::string welcomeMemberMsg = "Welcome to the Server, Please enter a username (No Spaces): ";
	Send(newClientSocket, welcomeMemberMsg);

	return;
}