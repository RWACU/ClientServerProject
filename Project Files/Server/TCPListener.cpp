#include "TCPListener.h"
#include <string>

#include <iostream>
#include <sstream>

#define MAX_BUFFER_SIZE (49152)

//initialize Winsock
bool TCPListener::Init()
{
	// Initialize Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsInit = WSAStartup(ver, &wsData);
	if (wsInit != 0)
	{
		std::cerr << "Can't Initialize Winsock! Quitting" << std::endl;
		return 0;
	}
	return 1;
}



// Create a Socket
SOCKET TCPListener::createListenSocket()
{
	// Create a Socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		std::cerr << "Can't Create a Socket! Quitting" << std::endl;
		return -1;
	}

	// Bind an IP Address and port to a Socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_port);
	hint.sin_addr.s_addr = INADDR_ANY;
	inet_pton(AF_INET, m_ipAddress.c_str(), &hint.sin_addr);

	int bindOK = bind(listening, (sockaddr*)&hint, sizeof(hint));
	if (bindOK == SOCKET_ERROR)
	{
		std::cerr << "Can't Bind to Socket! Quitting" << std::endl;
		return -1;
	}

	// Tell Windows the Socket is for listening
	int listenOK = listen(listening, SOMAXCONN);
	if (listenOK == SOCKET_ERROR)
	{
		std::cerr << "Can't Bind to Socket! Quitting" << std::endl;
		return -1;
	}

	return listening;
}

// Recieve loop
void TCPListener::run()
{
	char buf[MAX_BUFFER_SIZE];

	// Create a Listening Socket
	listening = createListenSocket();
	if (listening == INVALID_SOCKET)
	{
		//UNKNOWN
	}
	FD_ZERO(&master);
	FD_SET(listening, &master);

	// Wait for Connection
	while (true)
	{
		fd_set copy = master;
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; ++i)
		{
			SOCKET inputSock = copy.fd_array[i];
			if (inputSock == listening)
			{
				acceptNewClient();
			}
			else
			{
				// Handle Client Input
				char buf[MAX_BUFFER_SIZE];
				ZeroMemory(buf, MAX_BUFFER_SIZE);

				int bytesIn = recv(inputSock, buf, MAX_BUFFER_SIZE, 0);
				if (bytesIn == SOCKET_ERROR || bytesIn <= 0)
				{
					std::cerr << users[inputSock] << " Has closed their client" << std::endl;
					// Drop Client
					closesocket(inputSock);
					FD_CLR(inputSock, &master);
				}
				else
				{
					if (users[inputSock] == "")
					{
						users[inputSock] = buf;
						std::string nameMessage;
						
						Send(inputSock, nameMessage);

						//Broadcast we have a new connection
						std::ostringstream os;
						os << "<SERVER> " << users[inputSock] << " has joined the chat!\r\n";
						std::string welcomeNewConnectionMsg{ os.str() };
						broadcastMSG(inputSock, welcomeNewConnectionMsg);
						std::cout << inputSock << " is " << users[inputSock] << std::endl;
					}
					else
					{
						std::ostringstream os;
						os << users[inputSock] << ": " << buf << "\r\n";
						std::string output{ os.str() };
						broadcastMSG(inputSock, output);
						
					}
				}
			}
		}
	}
}

//Broadcast message to all clients
void TCPListener::broadcastMSG(SOCKET inputSock, std::string msg)
{
	// Send message to other clients, and definiately NOT the listening socket
	for (int i = 0; i < master.fd_count; ++i)
	{
		SOCKET outputSock = master.fd_array[i];
		if (outputSock != listening && outputSock != inputSock)
		{
			send(outputSock, msg.c_str(), msg.size() + 1, 0);
			std::cout << msg;
		}
	}
}

void TCPListener::acceptNewClient()
{
	// Accept a new connection
	sockaddr_in newClient;
	int clientSize = sizeof(newClient);

	SOCKET newClientSocket = accept(listening, (sockaddr*)&newClient, &clientSize);
	if (newClientSocket == INVALID_SOCKET)
	{
		std::cerr << "Can't Create a Socket! Quitting" << std::endl;
		//UNKNOWN
		return;
	}

	/*Can remove if desired - will display connections in window*/
	char host[NI_MAXHOST];				//Client's remote name
	char service[NI_MAXSERV];			//Service (i.e. port) the client is connected on

	ZeroMemory(host, NI_MAXHOST);		//same as memset(host, 0, NI_MAXHOST);
	ZeroMemory(service, NI_MAXSERV);

	if (getnameinfo((sockaddr*)&newClient, sizeof(newClient), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
	{
		std::cout << host << " connected on port " << service << std::endl;
	}
	else
	{
		inet_ntop(AF_INET, &newClient.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(newClient.sin_port) << std::endl;
	}
	/*Can remove if desired - will display connections in window*/

	//Add the new connection to the list of connected clients
	FD_SET(newClientSocket, &master);
	users[newClientSocket];

	// send a welcome message to the connected client
	std::string welcomeMemberMsg = "Welcome to the Server, Please enter a username: ";
	Send(newClientSocket, welcomeMemberMsg);
}