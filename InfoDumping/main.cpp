#include "InfoDumping.h"
#include <iostream>
#include <string.h>
#include <stdio.h>
bool DirectoryExists(const TString& dirName_in)
{
    DWORD ftyp = GetFileAttributes(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES) return false;
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY) return true;
    return false;
}

int _tmain(int argc, TCHAR* argv[])
{
    InfoDumping* tool = new InfoDumping();

    TCHAR* port_number;
    TCHAR* server_address;
    TCHAR* output_dir_name;
    if (tool->parseArgs(argc, argv, &port_number, &server_address, &output_dir_name) == -1)
    {
        return -1;
    }


    TCHAR currentDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDir);

    // Append the directory name to the current directory
    TString newDir = currentDir;
    newDir += TEXT("\\");
    newDir += output_dir_name;

    char* cNewDir;
#ifdef UNICODE
    // Convert TCHAR* to char*
    /*printf("Using Unicode Charset\n");*/
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, newDir.c_str(), -1, NULL, 0, NULL, NULL);
    cNewDir = new char[bufferSize];
    WideCharToMultiByte(CP_UTF8, 0, newDir.c_str(), -1, cNewDir, bufferSize, NULL, NULL);
#else
    // No conversion needed if TCHAR is char
    /*printf("Using Multi-Byte Charset\n");*/
    cNewDir = newDir.c_str();
#endif

    // Check if the directory already exists
    if (!DirectoryExists(newDir))
    {
        // Create the directory
        if (CreateDirectory(newDir.c_str(), NULL)) {
            std::cout << "Directory created successfully!\n";
        }
        else {
            std::cerr << "Failed to create directory! Error code: " << GetLastError() << std::endl;
        }
    }

    // Send an initial buffer
    SOCKET ConnectSocket = INVALID_SOCKET;
    ConnectSocket = tool->constructConnection(port_number, server_address);
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
        /*std::cout << "ffdaf\n";*/
        tool->receiveAll(sizeof(msg), msg, ConnectSocket);
        /*printf("msg: %s\n", msg);*/
        if (strcmp(msg, "dump") != 0)
        {
            std::cerr << "[-] Fail to receive dumping signal from server\n";
            memset(msg, 0, sizeof(msg));
            break;
        }
        std::cout << "[+] Start dumping\n";
        tool->dump_ram(newDir);
        tool->dump_process_info(cNewDir);
        tool->dum_net(cNewDir);
        tool->dump_registers(newDir);
        std::cout << "[+] End dumping\n";

        std::cout << "[+] Start sending dump to server\n";
        if (tool->sendDir(cNewDir, ConnectSocket) == 1)
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
#ifdef UNICODE
    delete[] cNewDir;
#endif

    return -1;
}
