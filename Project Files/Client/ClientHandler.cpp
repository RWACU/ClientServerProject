#include "ClientHandler.h"
#include "CommonSC.h"
#include <iostream>
#include <string>			// String interface
#include <sstream>			// StringStream interface 
#include <fstream>			// ifstream/ofstream interface
#include <limits>
#include <memory>
#include <regex>

// Constant expressions to be used throughout ClientHandler Class //
constexpr unsigned MAX_BUFFER_SIZE = (49152);		// Buffer size of the Incoming/Outgoing messages
constexpr char LOGINDIR[] = "../CSC/login.dat";

// Initialize client/server connection
bool ClientHandler::initializeWinsock()
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
		return false;
	}

	//Create Socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		// Cannot create a new socket. Need to close client
		std::cerr << "Can't start Winsock! Err #" << WSAGetLastError() << ", Quitting" << std::endl;
		return false;
	}

	// Fill in a Hint Structure (Clients information)
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// Connect to Server
	int connResult = connect(serverSocket, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		// Cannot connect to server. Need to close client
		std::cerr << "Can't start Winsock! Err #" << WSAGetLastError() << ", Quitting" << std::endl;
		return false;
	}

	// Print new clients remote and service name to server window
	char host[NI_MAXSERV];
	memset(host, 0, NI_MAXHOST);
	char service[NI_MAXSERV];			// Service (i.e. port) the client is connected on
	memset(service, 0, NI_MAXSERV);
	if (getnameinfo((sockaddr*)&serverSocket, sizeof(serverSocket), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
	{
		std::cout << host << " attempting connection on port " << service << std::endl;
	}
	else
	{
		inet_ntop(AF_INET, &hint.sin_addr, host, NI_MAXHOST);
		std::cout << host << " attempting connection on port " << ntohs(hint.sin_port) << std::endl;
	}

	return true;
}

// Make sure user entered a valid username based on standards
bool ClientHandler::verifyUsername(std::string& username)
{
	// verify username qualifies for username constraints
	if (UsernameSpecHandler == nullptr)
	{
		// Default Username Specification
		if (username.size() > 5 && username.size() < 24)
		{
			return std::regex_match(username, std::regex("[!@#$%&*?[:alnum:]]+"));
		}
		return FALSE;
	}
	else
	{
		return UsernameSpecHandler(username);
	}
}

// Make sure user entered a valid password based on standards 
bool ClientHandler::verifyPassword(std::string& password)
{
	// verify password qualifies for password constraints
	if (PasswordSpecHandler == nullptr)
	{
		// Default password specifications
		return (password.size() > 6 && password.size() < 254);
	}
	else
	{
		return PasswordSpecHandler(password);
	}
	return TRUE;


}

bool ClientHandler::enterUsernamePassword(std::string& username, std::unique_ptr<std::string>& password, bool newUserFlag)
{
	while (TRUE)	// continue loop until user enter valid username and password
	{
		if (newUserFlag)
		{
			std::cout << "Please Enter a new username and password...\n";
		}
		else
		{
			std::cout << "Please login to Server, or leave Username blank to create a new user...\n";
		}
		std::cout << "Username: ";
		// User Enters username and password, 
		std::cin >> username;
		clearCinErrors();

		if (username.size() == 0 && !newUserFlag)
		{
			// user wants to create a new account
			return FALSE;
		}
		else if (!verifyUsername(username))
		{
			clearScreen();
			std::cout << "That username doesn't meet the requirements:\n"
				<< "-Must be between 4 and 12 characters"
				<< "-Must only contain letters, numbers and these special characters: !,@,#,$,%,&,*\n"
				<< std::endl;
			continue;
		}

		std::cout << "Password: ";
		std::cin >> *password;
		clearCinErrors();

		if (!verifyPassword(*password))
		{
			clearScreen();
			std::cout << "That passsword doesn't meet the requirements:\n"
				<< "-Must be between 6 and 128 characters"
				<< "-Must contain a letter, number and at least one special character: !,@,#,$,%,&,*\n"
				<< std::endl;
			continue;
		}
		return TRUE;
	}
}

std::string&& ClientHandler::encryptRawPassword(std::unique_ptr<std::string>& unencryptedPassword, std::string& username)
{
	// encrypt password, pass back encrypted password
	char s[] = "ClientServerProgramConnect";
	return encryptArgon2ID(*unencryptedPassword, username, s);
}

bool ClientHandler::runClient()
{
	return true;
}

bool ClientHandler::encryptSendVerify(std::string& hPassword, std::string& serverRandomNumber)
{
	// encrypt password with servers random number, and devices signature
	char secret[] = "ClientServerProgramConnect";
	std::string clientEncryption{ encryptArgon2ID(hPassword, serverRandomNumber, secret) };
	sendPackets(serverSocket, std::move(clientEncryption));
	char encryptionConfirmation[3];
	recievePackets(serverSocket, encryptionConfirmation);
	// Compare strings send "AC" for true, and other response for failure
	if (encryptionConfirmation == "AC")
	{
		return TRUE;
	}
	return FALSE;
}

bool saveInformation(std::string& username, std::string& password)
{
	// If server validates username and password
	std::ofstream loginSave(LOGINDIR, std::ios::trunc);
	if (loginSave.is_open())
	{
		// Save username and hashed password in file
		loginSave << username << "\n" << password;

		// return and start connection
		return TRUE;
	}
	else
	{
		std::cout << "Cannot save login information... Please make sure this path is available:\n:"
			<< LOGINDIR << std::endl;
		// could not save information FIRGURE OUT WHAT TO DO
		return FALSE;
	}
}

bool ClientHandler::clientConnect()
{
	// Open Save file location
	std::ifstream savedLogin(LOGINDIR);
	std::string username, hPassword;
	if (savedLogin.is_open())
	{
		// If username is not set, ask user to log in
		savedLogin >> username >> hPassword;
		savedLogin.close();
	}
	while (TRUE)
	{
		if (username.size() <= 0 || hPassword.size() <= 0)
		{
			while (TRUE)
			{
				// login information not available in file
					
				std::unique_ptr<std::string> rawPassword;
				if (enterUsernamePassword(username, rawPassword, FALSE))
				{
					// User entered a username and password

					// EncryptPassword(password)
					hPassword = encryptRawPassword(rawPassword, username);
					std::string output{ '#' + username + '/' + hPassword };
					// Send Username to server, store username in clientHandler
					sendPackets(serverSocket, std::move(output));

					// Recieve random number
					std::string serverRandomNumber;
					recievePackets(serverSocket, serverRandomNumber);

					// sendEncrypt(string key, string hashed password)
					if (encryptSendVerify(hPassword, serverRandomNumber))
					{
						return (saveInformation(username, hPassword)) ? TRUE : FALSE;
					}
					else // If server denies username or password, loop from User enters password 
					{
						clearScreen();
						static unsigned tries = 4;
						if (tries != 0)
						{
							std::cout << "Your username or password is not correct. You have "
								<< tries << " tries remaining" << std::endl;
							--tries;
							continue;
						}
						else
						{
							std::cout << "There have been too many attempts made..."
								<< "Hit Enter to end session" << std::endl;
							std::cin.ignore();
							return FALSE;
						}
					}
				}
				else
				{
					// User selects new user
					enterUsernamePassword(username, rawPassword, TRUE);

					// EncryptPassword(password)
					hPassword = encryptRawPassword(rawPassword, username);
					std::string output{ "$" + username + '/' + hPassword };

					// Send Username to server, store username in clientHandler
					sendPackets(serverSocket, std::move(output));
					std::string serverRandomNumber;
					recievePackets(serverSocket, (char*)&serverRandomNumber);

					if (serverRandomNumber < "9000")
					{
						// If server validates client, ask user to validate their email
						if (encryptSendVerify(hPassword, serverRandomNumber))
						{
							return (saveInformation(username, hPassword)) ? TRUE : FALSE;
						}
					}
					else // Else If server fails validation, display failure message, as user to re-enter credentials
					{
						if (serverRandomNumber == "9234")
						{
							clearScreen();
							std::cout << "The username you requested has been taken... Please try again." << std::endl;
							continue;
						}
						else if (serverRandomNumber == "9404")
						{
							clearScreen();
							std::cout << "Your request to the server has been denied..."
								<< "Hit Enter to end session" << std::endl;
							std::cin.ignore();
							return FALSE;
						}
						else
						{
							clearScreen();
							std::cout << "There has been an unhandled error from the server, please try again later..."
								<< "Hit Enter to end session" << std::endl;
							std::cin.ignore();
							return FALSE;
						}
					}
				}
			}
		}
		else
		{
			// If file name and password is set
			std::string output{ '#' + username + '/' + hPassword };

			// Send Username to server, store username in clientHandler
			sendPackets(serverSocket, std::move(output));

			// Recieve random number, or error code
			std::string serverRandomNumber;
			recievePackets(serverSocket, serverRandomNumber);

			// sendEncrypt(string key, string hashed password)
			if (encryptSendVerify(hPassword, serverRandomNumber))
			{
				return (saveInformation(username, hPassword)) ? TRUE : FALSE;
			}
			else // If server denies username or password, loop from User enters password 
			{
				// Delete password and username from clientHandler
				clearScreen();
				std::cout << "Your username or password stored in memory is not correct..."
					<< "Please re-enter your information, or create a new user" << std::endl;
				hPassword = "";
				continue;
			}
		}
	}
}

// Close Client - Server sent an error
void ClientHandler::updateSockets(SOCKET directedSocket)
{
	std::cerr << "Server has severed connection... Closing Program." << std::endl;
	closesocket(directedSocket);
	WSACleanup();
	pause();
	exit(-1);
}