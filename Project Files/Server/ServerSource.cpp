#include <iostream>
#include <string>
#include <ws2tcpip.h>	//Socket Library for windows

#pragma comment (lib, "ws2_32.lib")

int main()
{
	// Initialize Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		std::cerr << "Can't Initialize Winsock! Quitting" << std::endl;
		return -1;
	}

	// Create a Socke
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		std::cerr << "Can't Create a Socket! Quitting" << std::endl;
		return -1;
	}

	// Bind an IP Address and port to a Socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // could also use inet_pton
	
	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Tell Windows the Socket is for listening
	listen(listening, SOMAXCONN);

	// Wait for a Connection
	sockaddr_in client;
	int clientSize = sizeof(client);

	SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
	if (clientSocket == INVALID_SOCKET)
	{
		std::cerr << "Can't Create a Socket! Quitting" << std::endl;
		closesocket(listening);
		WSACleanup();
		return -1;
	}

	char host[NI_MAXHOST];		//Client's remote name
	char service[NI_MAXSERV];	//Service (i.e. port) the client is connected on

	ZeroMemory(host, NI_MAXHOST);		//same as memset(host, 0, NI_MAXHOST);
	ZeroMemory(service, NI_MAXSERV);

	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
	{
		std::cout << host << " connected on port " << service << std::endl;
	}
	else
	{
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
	} 

	// Close Listening Socket
	closesocket(listening);

	// Handle Client Input (while loop)
	char buf[4096];

	while (true)
	{
		ZeroMemory(buf, 4096);

		// Wait for client to send data
		int bytesRecieved = recv(clientSocket, buf, 4096, 0);
		if (bytesRecieved == SOCKET_ERROR)
		{
			std::cerr << "Error in recv(). Quitting" << std::endl;
			closesocket(clientSocket);
			WSACleanup();
			return -1;
		}

		if (bytesRecieved == 0)
		{
			std::cout << "Client disconnected" << std::endl;
			break;
		}

		std::cout << "<" << host << ">" << std::string(buf, 0, bytesRecieved) << std::endl;

		//Echo message back to client
		send(clientSocket, buf, bytesRecieved + 1, 0);
	}
	
	// Close the Socket
	closesocket(clientSocket);

	// Cleanup Winsock
	WSACleanup();
	return 0;
}