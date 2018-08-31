#include <iostream>
#include <WS2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER_SIZE (49152)

DWORD WINAPI ListeningThread(LPVOID param)
{
	SOCKET s = (SOCKET)param;
	char Buffer[MAX_BUFFER_SIZE];
	int iResult;

	do
	{
		iResult = recv(s, Buffer, MAX_BUFFER_SIZE, 0);
		if (iResult <= 0) break;
		Buffer[iResult] = '\0';
		std::cout << Buffer << std::endl;
	} while (true);

	return 0;
}

int main()
{
	std::string ipAddress = "PHYSICAL IP HERE";		// IP Address of the server
	int port = 54010;								// LIstening port # on the Server

	//Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);

	if (wsResult != 0)
	{
		std::cerr << "Can't start Winsock! Err #" << wsResult << ", Quitting" << std::endl;
		WSACleanup();
		return -1;
	}

	//Create Socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "Can't start Winsock! Err #" << WSAGetLastError() << ", Quitting" << std::endl;
		WSACleanup();
		return -1;
	}

	// Fill in a Hint Structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);
	
	// connect to Server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		std::cerr << "Can't start Winsock! Err #" << WSAGetLastError() << ", Quitting" << std::endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	//std::cout << "Please Enter a username: " << std::endl;
	//getline(std::cin, userInput);
	send(sock, "", 0, 0);

	char Buffer[MAX_BUFFER_SIZE];
	recv(sock, Buffer, MAX_BUFFER_SIZE, 0);
	std::cout << Buffer << std::endl;
	std::string userInput;
	getline(std::cin, userInput);
	std::cout << std::endl;
	int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);

	// Create Listening Thread to handle incoming messages
	HANDLE hThread;
	DWORD dwThreadID;
	hThread = CreateThread(NULL, 0, &ListeningThread, (void*)sock, 0, &dwThreadID);

	//Do-while loop to send and recieve data
	do
	{
		// Prompt the user for some text and Send the Text
		getline(std::cin, userInput);
		std::cout << std::endl;
		int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
		if (sendResult == SOCKET_ERROR)
		{
			std::cout << "<SERVER> Input failed to send..\n" << std::endl;
		}
	} while (true);

	//cleanup
	closesocket(sock);
	WSACleanup;
	return 0; 
}