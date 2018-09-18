#include "ServerHandler.h"	// SocketHandler interface
#include <iostream>			// cout/cerr interfaces
#include <string>			// string interface
#include <sstream>			// stringstream interface 
#include <fstream>			// ifstream/ofstream interface
#include <future>			// async interface
#include <ctime>
#include "argon2.h"

// Constant expressions/Defines to be used throughout Server Class //
constexpr unsigned MAX_BUFFER_SIZE = (49152);		// Buffer size of the Incoming/Outgoing messages

// Initialize Winsock returning whether startup has succeeded
bool ServerHandler::initializeWinsock()
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
bool ServerHandler::createListenSocket()
{
	// Create a temperary socket to initialize
	// If all steps succeed, only then pass temp into SocketHandlers member variable
	SOCKET tempListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (tempListenSocket == INVALID_SOCKET)
	{
		// Listener socket not created, must close server as it cannot continue without it
		std::cerr	<< WSAGetLastError()
					<< "\nCan't Create a Socket! Quitting" << std::endl;
		return FALSE;
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
		return FALSE;
	}

	// Tell Windows the Socket is for listening
	int listenOK = listen(tempListenSocket, SOMAXCONN);
	if (listenOK == SOCKET_ERROR)
	{
		std::cerr << "Can't Bind to Socket! Quitting" << std::endl;
		return FALSE;
	}
	listenSocket = tempListenSocket;
	return TRUE;
}

void ServerHandler::disconnectClient(SOCKET quittingClient)
{
	std::cout << getClientID(quittingClient) << " has closed their client" << std::endl;
	closesocket(quittingClient);
	FD_CLR(quittingClient, &master);
}

// Create new sockets, and recieve/handle input from clients
void ServerHandler::run()
{
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
				else
				{
					MessageRecievedHandler(this, currentSocket, inputBuffer);
				}
			}
		}
	}
	return;
}

//Broadcast packets to all clients
void ServerHandler::broadcastMSG(SOCKET currentSocket, std::string msg)
{
	// Send message to other clients, and definiately NOT the listening socket
	for (unsigned i = 0; i < master.fd_count; ++i)
	{
		SOCKET outputSocket = master.fd_array[i];
		if (outputSocket != listenSocket && outputSocket != currentSocket)
		{
			sendPackets(outputSocket, msg);
		}
	}
	return;
}

// Accept incoming client, retrieve information, and handle login process on new async thread
void ServerHandler::acceptNewClient()
{
	// Accept a new connection
	sockaddr_in newClient;
	int			clientSize = sizeof(newClient);
	SOCKET		newClientSocket = accept(listenSocket, (sockaddr*)&newClient, &clientSize);

	if (newClientSocket == INVALID_SOCKET)
	{
		// New Client unable to connect, return without creating a new client objects
		std::cerr	<< WSAGetLastError()
					<< "Client unable to connect.\n" << std::endl;
		closesocket(newClientSocket);
		return;
	}

	// Print new clients remote and service name to server window
	char host[NI_MAXHOST];				// Client's remote name
	char service[NI_MAXSERV];			// Service (i.e. port) the client is connected on
	memset(host, 0, NI_MAXHOST);
	memset(service, 0, NI_MAXSERV);	
	if (getnameinfo((sockaddr*)&newClient, sizeof(newClient), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
	{
		std::cout << host << " attempting connection on port " << service << std::endl;
	}
	else
	{
		inet_ntop(AF_INET, &newClient.sin_addr, host, NI_MAXHOST);
		std::cout << host << " attempting connection on port " << ntohs(newClient.sin_port) << std::endl;
	}

	auto future = std::async(std::launch::async, &ServerHandler::clientLogin, this, newClientSocket, host);
	
	return;
}

// NEED DB TO CONNECT TO TO FINISH THIS
bool ServerHandler::checkUsernameExists(std::string& username)
{
	return true;
}

// NEED DB TO CONNECT TO TO FINISH THIS
bool ServerHandler::addNewUser(std::string& username, std::string& hPassword)
{
	return TRUE;
}

// Create and send a random number to act as salt for Argon2ID
std::string&& ServerHandler::sendRandomNumber(SOCKET directedSocket)
{
	std::srand(std::time(nullptr));
	std::string randomNumberKey;
	const char alphanum[] =
		"0123456789"
		"!@#$%^&*"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	
	for (unsigned i = 0; i < 32; ++i)
	randomNumberKey += alphanum[std::rand() % 71];
	sendPackets(directedSocket, randomNumberKey);

	return std::move(randomNumberKey);
}

bool ServerHandler::encryptRecieveVerify(std::string& hPassword, std::string& randomNumber, bool usersExistance, char* secretData, SOCKET newClient)
{
	//recieve client input
	if (usersExistance) // If logging in, fails if user does not exist. // If creating new user, fails if user already exists
	{
		std::string serverSidePass = "";// GET FROM DB

		// encrypt password with servers random number, and devices signature
		std::string  serverSideEncrypt{ encryptArgon2ID(hPassword, randomNumber, secretData) };

		// Recieve encrypted string from client
		char clientSideEncrypt[33];
		recievePackets(newClient, clientSideEncrypt);

		// compare strings send "AC" for true, send "NC" for failure
		if (serverSideEncrypt == (std::string)clientSideEncrypt)
		{
			return TRUE;
		}
	}
	return FALSE;
}

bool ServerHandler::clientLogin(SOCKET newUser, char secretData[NI_MAXHOST])
{
	while (TRUE)
	{
		// Retrieve clients user input (username/password)
		char userInput[279];
		recievePackets(newUser, userInput);
		std::stringstream inputStream{ userInput };

		// Find user code (first character from stream)
		char userCode[1];
		inputStream.write(userCode, 1);

		// Find username and password from stream
		std::string username, hPassword;
		std::getline(inputStream, username, '/');
		std::getline(inputStream, hPassword);

		if (*userCode == '#') // User Login
		{
			// If username exists
			bool doesUserExist = checkUsernameExists(username);

			// Generate random number to act as salt
			std::string randomNumber{ sendRandomNumber(newUser) };

			// Encrypt users hashed password with random number
			if (encryptRecieveVerify(hPassword, randomNumber, doesUserExist, secretData, newUser))
			{
				// Add the new connection to the list of connected clients
				FD_SET(newUser, &master);
				clientID[newUser];
				return TRUE;
			}
			else
			{
				// Close client Connection
				closesocket(newUser);
				return FALSE;
			}
		}
		else if (*userCode == '$') // New User
		{
			if (!checkUsernameExists(username))
			{
				// Generate random number
				std::string randomNumber{ sendRandomNumber(newUser) };

				// Encrypt users hashed password with random number
				bool isNewUser = TRUE;
				if (encryptRecieveVerify(hPassword, randomNumber, isNewUser, secretData, newUser))
				{
					// ADD USERNAME AND HASHED PASSWORD TO DB
					addNewUser(username, hPassword);
					// Add the new connection to the list of connected clients
					FD_SET(newUser, &master);
					clientID[newUser];
					return FALSE;
				}
				continue;
			}
			else
			{
				// Send error code "9234" to indicate username already taken
				sendPackets(newUser, "9234", 5);
				continue;
			}
		}
		else
		{
			// Did not send proper user code, drop client
			closesocket(newUser);
			return FALSE;
		}
	}
}