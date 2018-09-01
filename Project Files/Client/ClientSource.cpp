#include <iostream>
#include <WS2tcpip.h>
#include <string>

// Constant expressions to be used throughout Client //
//constexpr char IPADDRESS[] = "###.###.###.###";			// IP Address of the server
constexpr unsigned PORT = 54010;							// Listening port # on the Server
constexpr unsigned MAX_BUFFER_SIZE = (49152);				// Buffer size of the Incoming/Outgoing messages

// Thread process for listening to incoming messages from the server
DWORD WINAPI ListeningThread(LPVOID param)
{
	SOCKET listeningSocket = (SOCKET)param;
	char outputBuffer[MAX_BUFFER_SIZE];
	int iResult;

	while (true)
	{
		iResult = recv(listeningSocket, outputBuffer, MAX_BUFFER_SIZE, 0);
		if (iResult < 0)
		{
			// The server sent an error code, or has closed, must close program
			closesocket(listeningSocket);
			WSACleanup();
			exit(-1);
		}
		outputBuffer[iResult] = '\0';
		std::cout << outputBuffer << std::endl;
	}
	return 0;
}

// Send input to the server
void sendInput(int clientSocket, std::string&& input)
{
	int sendResult = send(clientSocket, input.c_str(), input.size() + 1, 0);
	if (sendResult == SOCKET_ERROR)
	{
		// Failed to send input to server. Continue process, but alert user
		std::cout << "<SERVER> Input failed to send..\n" << std::endl;
	}
}

int main()
{
	//Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		// Cannot start winsock. Need to close client
		std::cerr << "Can't start Winsock! Err #" << wsResult << ", Quitting" << std::endl;
		WSACleanup();
		return -1;
	}

	//Create Socket
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		// Cannot create a new socket. Need to close client
		std::cerr << "Can't start Winsock! Err #" << WSAGetLastError() << ", Quitting" << std::endl;
		WSACleanup();
		return -1;
	}

	// Fill in a Hint Structure (Clients information)
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT);
	inet_pton(AF_INET, IPADDRESS, &hint.sin_addr);
	
	// Connect to Server
	int connResult = connect(clientSocket, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		// Cannot connect to server. Need to close client
		std::cerr << "Can't start Winsock! Err #" << WSAGetLastError() << ", Quitting" << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return -1;
	}

	// User must sent username as first input
	char outputBuffer[MAX_BUFFER_SIZE];
	recv(clientSocket, outputBuffer, MAX_BUFFER_SIZE, 0);
	std::cout << outputBuffer << std::endl;
	std::string userInput;
	getline(std::cin, userInput);
	std::cout << std::endl;
	send(clientSocket, userInput.c_str(), userInput.size() + 1, 0);

	// Create Listening Thread to handle incoming messages
	HANDLE hThread;
	DWORD dwThreadID;
	hThread = CreateThread(NULL, 0, &ListeningThread, (void*)clientSocket, 0, &dwThreadID);

	// Continuously send input to server
	while (true)
	{
		// Prompt the user for some text and send the text
		getline(std::cin, userInput);
		std::cout << std::endl;
		sendInput(clientSocket, std::move(userInput));
	}

	//cleanup
	closesocket(clientSocket);
	WSACleanup;
	return 0; 
}