#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#define File 0
#define Dir 1
#define SIZE 4096

struct box
{
   short fileType;
   unsigned long long fileSize;
   char data[SIZE];
};

unsigned int myMin(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
}

int sendAll(size_t size, char* src, SOCKET sockfd)
{
   int totalBytesSent = 0;
   int bytesSent = 0;

   while (totalBytesSent < size)
   {
      bytesSent = send(sockfd, src + totalBytesSent, size - totalBytesSent, 0);

      if (bytesSent < 0)
      {
         std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
         closesocket(sockfd);
         std::cout << "[+] Disconnected the server\n";
         return 1;
      }
      else if (bytesSent == 0)
      {
         closesocket(sockfd);
         std::cout << "[+] Server disconnected\n";
         return 1;
      }

      totalBytesSent += bytesSent;
   }

   return 0;
}

int receiveAll(size_t size, char* dest, SOCKET sockfd)
{
   int totalBytesReceived = 0;
   int bytesReceived = 0;
   
   while (totalBytesReceived < size)
   {
        printf("ffffff\n");
        bytesReceived = recv(sockfd, dest + totalBytesReceived, size - totalBytesReceived, 0);

        if (bytesReceived < 0)
        {
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            closesocket(sockfd);
            std::cout << "[+] Disconnected the server\n";
            return 1;
        }
        else if (bytesReceived == 0)
        {
            closesocket(sockfd);
            std::cout << "[+] Server disconnected\n";
            return 1;
        }

        totalBytesReceived += bytesReceived;
   }

   return 0;
}

void sendFile(std::string fileName, SOCKET sockfd)
{
   FILE* f = fopen(fileName.c_str(), "rb");
   if (f == NULL)
   {
      std::cerr << "[-] ERROR: Can't open filename " << fileName << "\n";
      std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
      closesocket(sockfd);
      fclose(f);
      exit(1);
   }

   char sendBuffer[SIZE];
   memset(sendBuffer, 0, sizeof(sendBuffer));

   int bytesToSend = -1;
   int bytesReadFromFile = -1;
   
   while ((bytesReadFromFile = fread(sendBuffer, 1, SIZE, f)) > 0)
   {
      bytesToSend = myMin(bytesReadFromFile, SIZE);

      if (sendAll(bytesToSend, sendBuffer, sockfd) == 1)
      {
         return;
      }
      
      memset(sendBuffer, 0, SIZE);
      
   }

   fclose(f);

   //std::cout << "[+] Upload file " << fileName << " to server success\n";
}

int sendDir(std::string fileName, SOCKET sockfd)
{  
   char msg[sizeof(box)];
   box sender;

   memset(&sender, 0, sizeof(box));
   memset(msg, 0, sizeof(msg));

   // Get the current working directory
   char begin_cwd[MAX_PATH];
   GetCurrentDirectoryA(MAX_PATH, begin_cwd);
   // Get the information of the file including file type and file size
    DWORD fileAttributes = GetFileAttributesA(fileName.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) 
    {
        std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
        strncpy(msg, strerror(errno), strlen(strerror(errno))+1);
        if (sendAll(sizeof(msg), msg, sockfd) == 1)
        {
            closesocket(sockfd);
            return 1;
        }
        closesocket(sockfd);
        return 1;
    }
    printf("%s\n", fileName.c_str());
    strncpy(msg, "success", strlen("success")+1);
    if (sendAll(sizeof(msg), msg, sockfd) == 1)
    {
        return 1;
    }
    
    
    if (!(fileAttributes & FILE_ATTRIBUTE_NORMAL) && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        printf("%s\n", fileName.c_str());
        HANDLE fileHandle = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) 
        {
            printf("%s\n", "loixxx");
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            strncpy(msg, strerror(errno), strlen(strerror(errno))+1);
            if (sendAll(sizeof(msg), msg, sockfd) == 1)
            {
                CloseHandle(fileHandle);
                closesocket(sockfd);
                return 1;
            }
            closesocket(sockfd);
            CloseHandle(fileHandle);
            return 1;
        }
        
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(fileHandle, &fileSize)) 
        {
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            strncpy(msg, strerror(errno), strlen(strerror(errno))+1);
            if (sendAll(sizeof(msg), msg, sockfd) == 1)
            {
                closesocket(sockfd);
                CloseHandle(fileHandle);
                return 1;
            }
            closesocket(sockfd);
            CloseHandle(fileHandle);
            return 1;
        }
    
        sender.fileType = File;
        sender.fileSize = fileSize.QuadPart;
        std::string fn = strrchr(fileName.c_str(), '\\')+1;
        strncpy(sender.data, fn.c_str(), fn.size()+1);
        memcpy(msg, &sender, sizeof(box));

        if (sendAll(sizeof(msg), msg, sockfd) == 1)
        {
            return 1;
        }

        sendFile(fileName, sockfd);

        memset(msg, 0, sizeof(msg));
        memset(&sender, 0, sizeof(box));
        CloseHandle(fileHandle);
        return 0;
    }
    else // Case: the file is Directory
    {
        printf("dir\n");
        sender.fileType = Dir;
        sender.fileSize = 0;
        
        std::string fn = strrchr(fileName.c_str(), '\\')+1;
        strncpy(sender.data, fn.c_str(), fn.size()+1);
        printf("%sddd\n", fn.c_str());
        memcpy(msg, &sender, sizeof(box));

        if (sendAll(sizeof(msg), msg, sockfd) == 1)
        {
            return 1;
        }

        char curDirPath[MAX_PATH];
        strncpy(curDirPath, fileName.c_str(), fileName.size()+1); 
        if (!SetCurrentDirectoryA(curDirPath))
        {
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            closesocket(sockfd);
            return 1;
        }
        printf("%susdjf\n", curDirPath);
        char* adding = strcpy(curDirPath+strlen(curDirPath), "\\*");
        printf("%s\n", curDirPath);
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA(curDirPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) 
        {
            std::cerr << "[-] ERROR: fdsafd" << strerror(errno) << "\n";
            closesocket(sockfd);
            return 1;
        }

        char* filePath = curDirPath;
        int loopFlag = -1;
        do
        {   
            if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            {
                continue;
            }
            loopFlag = 1;
            strcpy(adding+1, findFileData.cFileName);
            if (sendAll(sizeof(int), (char*)&loopFlag, sockfd) == 1)
            {
               return 1;
            }
            if (sendDir(filePath, sockfd) == 1)
            {
               return 1;
            }

        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
        loopFlag = 0;
        if (sendAll(sizeof(int), (char*)&loopFlag, sockfd) == 1)
        {
            return 1;
        }
        if (!SetCurrentDirectoryA(begin_cwd))
        {
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            closesocket(sockfd);
    
            return 1;
        }
        return 0;
    }

}

void help(char* ExeName)
{
    printf("[+] Usage:  %s [option]\n", ExeName);
    printf("[+] Option:\n");
    printf("            -i: Specify the server IP address.\n"
           "            -p: Specify the server port.\n"
           "            -d: Specify the dump directory name.\n");
}

int parseArgs(int argc, char **argv, char** port_number, char** server_address, char** output_dir_name)
{
    if (argc != 7) 
    {
        help(argv[0]);
        return -1;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' && argv[i][1] != 0)
        {
            switch (argv[i][1])
            {

                case 'i':
                {
                    i++;
                    *server_address = argv[i];
                    break;
                }
                case 'p':
                {
                    i++;
                    *port_number = argv[i];
                    break;
                }
                case 'd':
                {
                    i++;
                    *output_dir_name = argv[i];
                    break;
                }
                default:
                {
                    help(argv[0]);
                    return -1;
                }

            }  // Switch.

        }
        else break;   //First option without - means end of options.
    }
    return 1;
}

SOCKET constructConnection(char* port_number, char* server_address)
{
    
    printf("%s\n", server_address);
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) 
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(server_address, port_number, &hints, &result);
    if ( iResult != 0 ) 
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
    {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) 
        {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) 
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    return ConnectSocket;
}

bool DirectoryExists(const std::string& dirName_in) 
{
    DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES) return false;
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY) return true;
    return false;
}

int main(int argc, char **argv) 
{
    char* port_number;
    char* server_address;
    char* output_dir_name;
    if (parseArgs(argc, argv, &port_number, &server_address, &output_dir_name) == -1)
    {
        return -1;
    }

    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);

    // Append the directory name to the current directory
    std::string newDir = currentDir;
    newDir += "\\";
    newDir += output_dir_name;

    // Check if the directory already exists
    if (!DirectoryExists(newDir)) 
    {
        // Create the directory
        if (CreateDirectoryA(newDir.c_str(), NULL)) {
            std::cout << "Directory created successfully!\n";
        } else {
            std::cerr << "Failed to create directory! Error code: " << GetLastError() << std::endl;
        }
    }

    // Send an initial buffer
    SOCKET ConnectSocket = INVALID_SOCKET;
    ConnectSocket = constructConnection(port_number, server_address);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("[-] Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }


    char msg[sizeof(box)];
    memset(msg, 0, sizeof(msg));
    while (true)
    {   
        std::cout << "[+] Waiting for dumping signal \n"; 
        std::cout << "ffdaf\n";
        receiveAll(sizeof(msg), msg, ConnectSocket);
        printf("msg: %s\n", msg);
        if (strcmp(msg, "dump") != 0)
        {
            std::cerr << "[-] Fail to receive dumping signal from server\n";
            memset(msg, 0, sizeof(msg));
            break;
        }
        std::cout << "[+] Start sending dump to server\n";
        if (sendDir(newDir, ConnectSocket) == 1)
        {
            std::cout << "[-] Error on sending dump to server\n";
            memset(msg, 0, sizeof(msg));
            break;
        }
        std::cout << "[+] Dump has been successfully sent to server\n";
        memset(msg, 0, sizeof(msg));
    }
    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}