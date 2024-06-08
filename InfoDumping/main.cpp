#include "InfoDumping.h"
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <ctime>
#include <iomanip>

bool DirectoryExists(const TString& dirName_in)
{
    DWORD ftyp = GetFileAttributes(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES) return false;
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY) return true;
    return false;
}

TString get_current_timestamp()
{
    // Get current time
    std::time_t now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);  // safer version of localtime

    // Create a timestamp string
    Tostringstream timestamp;
    timestamp << std::put_time(&tm, TEXT("_%Y%m%d_%H%M%S"));

    return timestamp.str();
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

        TString crtTimestamp = get_current_timestamp();
        TString logDir = newDir + TEXT("\\log") + crtTimestamp;
        char* c_crtTimestamp;
        char* c_logDir;
#ifdef UNICODE
        // Convert TCHAR* to char*
        /*printf("Using Unicode Charset\n");*/
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, logDir.c_str(), -1, NULL, 0, NULL, NULL);
        c_logDir = new char[bufferSize];
        WideCharToMultiByte(CP_UTF8, 0, logDir.c_str(), -1, c_logDir, bufferSize, NULL, NULL);

        bufferSize = WideCharToMultiByte(CP_UTF8, 0, crtTimestamp.c_str(), -1, NULL, 0, NULL, NULL);
        c_crtTimestamp = new char[bufferSize];
        WideCharToMultiByte(CP_UTF8, 0, crtTimestamp.c_str(), -1, c_crtTimestamp, bufferSize, NULL, NULL);
#else
        // No conversion needed if TCHAR is char
        /*printf("Using Multi-Byte Charset\n");*/
        cNewDir = logDir.c_str();
        c_crtTimestamp = crtTimestamp.c_str()
#endif

        if (!DirectoryExists(logDir))
        {
            // Create the directory
            if (CreateDirectory(logDir.c_str(), NULL)) {
                std::cout << "Directory " << c_logDir << " created successfully!\n";
            }
            else {
                std::cerr << "Failed to create directory! Error code: " << GetLastError() << std::endl;
            }
        }

        std::cout << "[+] Start dumping\n";
        //tool->dump_ram(logDir, crtTimestamp);
        tool->dump_process_info(c_logDir, c_crtTimestamp);
        tool->dum_net(c_logDir, c_crtTimestamp);
        //tool->dump_registers(logDir, crtTimestamp);
        std::cout << "[+] End dumping\n";

        std::cout << "[+] Start sending dump to server\n";
        if (tool->sendDir(c_logDir, ConnectSocket) == 1)
        {
            std::cout << "[-] Error on sending dump to server\n";
            memset(msg, 0, sizeof(msg));
            break;
        }
        std::cout << "[+] Dump has been successfully sent to server\n";
        memset(msg, 0, sizeof(msg));
#ifdef UNICODE
        delete[] c_logDir;
        delete[] c_crtTimestamp;
#endif
    }
    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return -1;
}
