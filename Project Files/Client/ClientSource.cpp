#include <iostream>
#include <WS2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main()
{
	std::string ipAddress = "127.0.0.1";		// IP Address of the server
	int port = 54000;							// LIstening port # on the Server

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

	//Do-while loop to send and recieve data
	char buf[4096];
	std::string userInput;

	do
	{
		// Prompt the user for some text
		std::cout << "> ";
		getline(std::cin, userInput);

		if (userInput.size() > 0)	// Make sure user has typed in something
		{
			// Send the Text
			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
			if (sendResult != SOCKET_ERROR)
			{

				//Wait for Response
				ZeroMemory(buf, 4096);
				int bytesRecieved = recv(sock, buf, 4096, 0);
				if (bytesRecieved > 0)
				{
					//Echo response to console
					std::cout << "SERVER> " << std::string(buf, 0, bytesRecieved) << std::endl;
				}
			}			
		}
	} while (userInput.size() > 0);

	//cleanup
	closesocket(sock);
	WSACleanup;
	return 0; 
}