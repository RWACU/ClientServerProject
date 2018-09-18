#include "CommonSC.h"

#include "argon2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <ws2tcpip.h>	// Winsock Functions
#include <fstream>

#undef max
#define _OPEN_SYS_ITOA_EXT

// Pause operations on program to show messages to user
void CSC::pause()
{
	std::cout << "\nPress enter to continue!";
	std::cin.sync();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

//Clear all flags and buffer from std::cin
void CSC::clearCinErrors()
{
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Empty Window output, and start back at the top
void CSC::clearScreen()
{
	HANDLE hStdOut{ GetStdHandle(STD_OUTPUT_HANDLE) };
	if (hStdOut == INVALID_HANDLE_VALUE) return;

	// Get the number of cells in the current buffer
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return;
	DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire buffer with spaces
	DWORD	count;
	COORD homeCoords{ 0, 0 };
	if (!FillConsoleOutputCharacter(hStdOut, (TCHAR) ' ', cellCount, homeCoords, &count)) return;

	// Fill the entire buffer with the current colors and attributes
	if (!FillConsoleOutputAttribute(hStdOut, csbi.wAttributes, cellCount, homeCoords, &count)) return;

	// Move the cursor home
	SetConsoleCursorPosition(hStdOut, homeCoords);
}

// Use Argon2ID to encrypt password/salt combonation
std::string&& CSC::encryptArgon2ID(std::string& password, std::string& randomNumber, char* secretData, unsigned t_cost, unsigned m_cost, unsigned parallelism)
{
	unsigned char hashedString[32];
	std::string salt = randomNumber + secretData;
	int rc = argon2_hash(t_cost, 1 << m_cost, parallelism, password.c_str(), (password.size()+1), salt.c_str(), (salt.size()+1), hashedString, 32, NULL, NULL, Argon2_id, ARGON2_VERSION_10);
	if (rc != ARGON2_OK)
	{
		std::cout << "Error: " << argon2_error_message(rc) << std::endl;
		exit(1);
	}
	return std::move(std::string(reinterpret_cast<char*>(hashedString)));
}

// Send packet size then the complete packet set until all is send to directed socket
bool CSC::sendPackets(SOCKET& directedSocket, const char* outputBuffer, uint64_t outputBufferSize, bool packetFlag)
{
	if (packetFlag)
		if (!sendPackets(directedSocket, reinterpret_cast<const char*>(outputBufferSize), 8, FALSE))
			return FALSE;

	uint64_t total		{ 0 };				// how many bytes we've sent
	uint64_t bytesleft	{ outputBufferSize };	// how many we have left to send
	uint64_t sendCode, recvCode;

	while (total < outputBufferSize) {
		sendCode = send(directedSocket, outputBuffer + total, (int)bytesleft, 0);

		if (sendCode == INVALID_SOCKET)
		{
			// Close socket
			// client list update
			// The directedSocket sent an error code, or has stopped
			std::cout << WSAGetLastError() << std::endl;
			return FALSE;
		}
		char rec[3];
		recvCode = recv(directedSocket, rec, 3, 0);
		if (recvCode == INVALID_SOCKET)
		{
			// Close socket
			// client list update
			// The directedSocket sent an error code, or has stopped
			std::cout << WSAGetLastError() << std::endl;
			return FALSE;
		}
		else if (rec == "NC")
		{
			// Client to Client Denied
			return FALSE;
		}
		total += sendCode;
		bytesleft -= sendCode;
	}
	return TRUE;
}

// Send string input to the server through "const char*" sendPackets() 
bool CSC::sendPackets(SOCKET& directedSocket, const std::string& output)
{
	const char* buffer = output.c_str();
	uint64_t sizeSent{ output.size() + 1 };
	return sendPackets(directedSocket, buffer, sizeSent) ? TRUE : FALSE;
}

// Recieve packet size then the complete packet set until all is recieved by the directed socket
bool CSC::recievePackets(SOCKET& directedSocket, char* inputBuffer, bool packetFlag)
{
	uint64_t inputBufferSize;
	if (packetFlag)
	{
		char* incomingBufferSize{};
		if (!recievePackets(directedSocket, incomingBufferSize, FALSE))
			return FALSE;
		inputBufferSize = reinterpret_cast<uint64_t>(incomingBufferSize);
	}
	else
	{
		inputBufferSize = 8;
	}

	uint64_t total = 0;
	uint64_t bytesleft = 8;
	uint64_t recvCode, sendCode;

	while (total < inputBufferSize) {
		recvCode = recv(directedSocket, inputBuffer, (int)bytesleft, 0);
		if (recvCode == INVALID_SOCKET)
		{
			send(directedSocket, "NC", 3, 0);

			// Close socket
			closesocket(directedSocket);
			// client list update
			
			// The directedSocket sent an error code, or has stopped
			std::cout << WSAGetLastError() << std::endl;
			return FALSE;
		}
		sendCode = send(directedSocket, "OK", (int)bytesleft, 0);
		if (sendCode == INVALID_SOCKET)
		{
			// Close socket
			// client list update
			// The directedSocket sent an error code, or has stopped
			std::cout << WSAGetLastError() << std::endl;
			return FALSE;
		}
	}
	return TRUE;
}

// Recieve incoming packets as a string through "char*" recievePackets()
bool CSC::recievePackets(SOCKET& directedSocket, std::string& inputBuffer)
{
	char* buffer{};
	if (recievePackets(directedSocket, buffer))
	{
		inputBuffer = buffer;
		return TRUE;
	}
	return FALSE;
}

// Send file over connection to client
void CSC::sendFile(std::string &filePath, SOCKET directedSocket)
{
	// Send new file command to client to prep for file transfer
	sendPackets(directedSocket, "~FILE~");

	// Retrieving the file name with extension from the file path, send to client
	std::string		fileName = filePath.substr(filePath.find_last_of('\\') + 1);
	std::ifstream	inputFile(filePath, std::ios::in | std::ios::ate | std::ios::binary);

	if (inputFile.is_open())
	{
		// When file can be opened, send client the filename
		sendPackets(directedSocket, fileName);

		// Retrieve total file size and send to client for processing
		uint64_t fileSize = (uint64_t)inputFile.tellg();
		inputFile.seekg(std::ios::beg);
		char fileSizeBuffer[1024];
		_ui64toa_s(fileSize, fileSizeBuffer, 1024, 10);
		sendPackets(directedSocket, fileSizeBuffer);

		// Send data from file in 1024 byte chunks
		unsigned char buffer[1024];
		for (; fileSize > 0; fileSize -= 1024)
		{
			if (fileSize >= 1024)
			{
				inputFile.read(reinterpret_cast<char *>(buffer), 1024);
				sendPackets(directedSocket, (char*)buffer, 1024);
			}
			else
			{
				inputFile.read(reinterpret_cast<char *>(buffer), fileSize);
				sendPackets(directedSocket, (char*)buffer, fileSize);
			}
		}
	}
	inputFile.close();
	return;
}

// Recieve a file being sent by the server
bool CSC::recieveFile(SOCKET& directedSocket)
{
	// Recieve the filename with extension from the server
	char fileName[32] = "";
	recievePackets(directedSocket, (char*)fileName);

	// Create (or overwrite) file for input
	std::ofstream outputFile;
	outputFile.open(fileName, std::ios::binary | std::ios::out | std::ios::trunc);

	if (outputFile.is_open())
	{
		// Recieve total size of the file to be able to handle input
		uint64_t recievedFileSize{};
		recievePackets(directedSocket, (char*)recievedFileSize);

		// Copy all data in chunks of 1024 bytes into created file
		char buffer[1024];
		for (; recievedFileSize > 0; recievedFileSize -= 1024)
		{
			recievePackets(directedSocket, buffer);
			outputFile << buffer;
		}
		outputFile.close();
		return TRUE;
	}
	else
	{
		// Unable to open file
		std::cerr << "File failed to load from:\n" << fileName << "\nPlease check path and try again\n" << std::endl;
		return FALSE;
	}
}