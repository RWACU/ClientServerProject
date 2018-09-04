#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <fstream>
#include <sstream>

// Constant expressions to be used throughout Client //
constexpr char IPADDRESS[] = "###.###.###.###";				// IP Address of the server
constexpr unsigned PORT = 10000;							// Listening port # on the Server
constexpr unsigned MAX_BUFFER_SIZE = (2048);				// Buffer size of the Incoming/Outgoing messages

// Confirm recieve was successful and send verification code
int recieveVerify(SOCKET verificationSocket, int iResult)
{
	if (iResult < 0)
	{
		// The server sent an error code, or has closed, must close program
		closesocket(verificationSocket);
		WSACleanup();
		exit(-1);
	}
	send(verificationSocket, "OK", strlen("OK"), 0);
	return iResult;
}

// Recieve a file being sent by the server
void recievefile (SOCKET listeningSocket)
{
	// Recieve the filename with extension from the server
	char fileName[32] = "";
	recieveVerify(listeningSocket, recv(listeningSocket, fileName, MAX_BUFFER_SIZE, 0));
	std::cout << "Recieving File: " << fileName << std::endl;
	
	// Create (or overwrite) file for input
	std::ofstream outputFile;
	outputFile.open(fileName, std::ios::binary | std::ios::out | std::ios::trunc);
	if (outputFile.is_open())
	{
		// Recieve total size of the file to be able to handle input
		char recievedFileSize[50] = "";
		int recieveResultCode = recieveVerify(listeningSocket, recv(listeningSocket, recievedFileSize, MAX_BUFFER_SIZE, 0));
		recievedFileSize[recieveResultCode] = '\0';
		int size = atoi(recievedFileSize);

		// Copy all data in chunks of 1024 bytes into created file
		std::cout << "Copy started..." << std::endl;
		char buffer[1024];
		for ( ; size > 0; size -= 1024)
		{
			if (size >= 1024)
			{
				recieveVerify(listeningSocket, recv(listeningSocket, buffer, 1024, 0));
				outputFile.write(reinterpret_cast<char *>(buffer), 1024);
			}
			else
			{
				recieveVerify(listeningSocket, recv(listeningSocket, buffer, size, 0));
				outputFile.write(reinterpret_cast<char *>(buffer), size);
			}
		}
		outputFile.close();
		std::cout << "Copy Completed..." << std::endl;
		return;
	}
	else
	{
		// Unable to open file
		std::cerr << "File failed to load from:\n" << fileName << "\nPlease check path and try again\n" << std::endl;
	}
	return;
}

// Thread process for listening to incoming messages from the server
DWORD WINAPI ListeningThread(LPVOID param)
{
	SOCKET listeningSocket = (SOCKET)param;
	char outputBuffer[MAX_BUFFER_SIZE];
	while (true)
	{
		int iResult = recieveVerify(listeningSocket, recv(listeningSocket, outputBuffer, MAX_BUFFER_SIZE, 0));
		outputBuffer[iResult] = '\0';
		if (strcmp(outputBuffer, "~New File~") == 0)
		{
			// Server is attempting to send a file
			recievefile(listeningSocket);
			continue;
		}
		else
		{
			// Server is sending text
			std::cout << outputBuffer << std::endl;
		}
		
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
	std::cout << "Client connecting to Server..." << std::endl;

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

	// Create Listening Thread to handle incoming messages
	HANDLE hThread;
	DWORD dwThreadID;
	hThread = CreateThread(NULL, 0, &ListeningThread, (void*)clientSocket, 0, &dwThreadID);

	// Continuously send input to server
	std::string userInput;
	while (true)
	{
		// Prompt the user for some text and send the text
		getline(std::cin, userInput);
		std::cout << std::endl;
		sendInput(clientSocket, std::move(userInput));
	}

	//cleanup
	closesocket(clientSocket);
	WSACleanup();
	return 0; 
}