#define WIN32_LEAN_AND_MEAN
#include "..\kernel\executable\winpmem.h"
#include "InfoDumping.h"
#include <iostream>
#include <vector>
#include <ws2tcpip.h>
#include <iomanip>
#include <string.h>
#include <stdio.h>

unsigned int myMin(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
}

int sendAll(size_t size, char* src, SOCKET sockfd)
{
    unsigned int totalBytesSent = 0;
    unsigned int bytesSent = 0;

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

int InfoDumping::receiveAll(size_t size, char* dest, SOCKET sockfd)
{
    unsigned int totalBytesReceived = 0;
    unsigned int bytesReceived = 0;

    while (totalBytesReceived < size)
    {
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

    unsigned int bytesToSend = -1;
    unsigned int bytesReadFromFile = -1;

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

int InfoDumping::sendDir(std::string fileName, SOCKET sockfd)
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
        strncpy(msg, strerror(errno), strlen(strerror(errno)) + 1);
        if (sendAll(sizeof(msg), msg, sockfd) == 1)
        {
            closesocket(sockfd);
            return 1;
        }
        closesocket(sockfd);
        return 1;
    }
    printf("%s\n", fileName.c_str());
    strncpy(msg, "success", strlen("success") + 1);
    if (sendAll(sizeof(msg), msg, sockfd) == 1)
    {
        return 1;
    }


    if (!(fileAttributes & FILE_ATTRIBUTE_NORMAL) && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        //printf("%s\n", fileName.c_str());
        HANDLE fileHandle = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            //printf("%s\n", "loixxx");
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            strncpy(msg, strerror(errno), strlen(strerror(errno)) + 1);
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
            strncpy(msg, strerror(errno), strlen(strerror(errno)) + 1);
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
        std::string fn = strrchr(fileName.c_str(), '\\') + 1;
        strncpy(sender.data, fn.c_str(), fn.size() + 1);
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
        //printf("dir\n");
        sender.fileType = Dir;
        sender.fileSize = 0;

        std::string fn = strrchr(fileName.c_str(), '\\') + 1;
        strncpy(sender.data, fn.c_str(), fn.size() + 1);
        //printf("%sddd\n", fn.c_str());
        memcpy(msg, &sender, sizeof(box));

        if (sendAll(sizeof(msg), msg, sockfd) == 1)
        {
            return 1;
        }

        char curDirPath[MAX_PATH];
        strncpy(curDirPath, fileName.c_str(), fileName.size() + 1);
        if (!SetCurrentDirectoryA(curDirPath))
        {
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            closesocket(sockfd);
            return 1;
        }
        //printf("%susdjf\n", curDirPath);
        char* adding = strcpy(curDirPath + strlen(curDirPath), "\\*");
        //printf("%s\n", curDirPath);
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
            strcpy(adding + 1, findFileData.cFileName);
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

void help(TCHAR* ExeName)
{
    printf("[+] Usage:  %S [option]\n", ExeName);
    printf("[+] Option:\n");
    printf("            -i: Specify the server IP address.\n"
        "            -p: Specify the server port.\n"
        "            -d: Specify the dump directory name.\n");
}

int InfoDumping::parseArgs(int argc, TCHAR** argv, TCHAR** port_number, TCHAR** server_address, TCHAR** output_dir_name)
{
    if (argc != 7)
    {
        help(argv[0]);
        return -1;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == TEXT('-') && argv[i][1] != 0)
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

SOCKET InfoDumping::constructConnection(TString port_number, TString server_address)
{
    char* srv_addr;
    char* port;
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    int iResult;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

#ifdef UNICODE
    // Convert TCHAR* to char*
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, port_number.c_str(), -1, NULL, 0, NULL, NULL);
    port = new char[bufferSize];
    WideCharToMultiByte(CP_UTF8, 0, port_number.c_str(), -1, port, bufferSize, NULL, NULL);

    bufferSize = WideCharToMultiByte(CP_UTF8, 0, server_address.c_str(), -1, NULL, 0, NULL, NULL);
    srv_addr = new char[bufferSize];
    WideCharToMultiByte(CP_UTF8, 0, server_address.c_str(), -1, srv_addr, bufferSize, NULL, NULL);
#else
    // No conversion needed if TCHAR is char
    port = port_number.c_str();
    srv_addr = server_address.c_str();
#endif
    /*printf("srv_addr: %s\n", srv_addr);
    printf("port: %s\n", port);*/
    // Resolve the server address and port
    iResult = getaddrinfo(srv_addr, port, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
#ifdef UNICODE
        delete[] port;
        delete[] srv_addr;
#endif
        WSACleanup();
        return 1;
    }

#ifdef UNICODE
    delete[] port;
    delete[] srv_addr;
#endif

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
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


WinPmem* WinPmemFactory()
{
    SYSTEM_INFO sys_info;

    ZeroMemory(&sys_info, sizeof(sys_info));

    GetNativeSystemInfo(&sys_info);

    switch (sys_info.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        printf("%S",L"WinPmem64\n");
        return new WinPmem64();

    case PROCESSOR_ARCHITECTURE_INTEL:
        printf("% S", L"WinPmem32\n");
        return new WinPmem32();

    default:
        return NULL;
    }
}

int InfoDumping::dump_ram(TString dir, TString timestamp)
{
    TString output_filename = dir;
    output_filename += TEXT("\\ramDump");
    output_filename += timestamp;
    output_filename += TEXT(".raw");

    __int64 status;
    unsigned __int32 mode = PMEM_MODE_PHYSICAL;

    WinPmem* pmem_handle = WinPmemFactory();
    TCHAR* driver_filename = NULL;

    pmem_handle->set_driver_filename(driver_filename);

    status = pmem_handle->create_output_file((TCHAR*)output_filename.c_str());

    if ((status) && (pmem_handle->install_driver() > 0) && (pmem_handle->set_acquisition_mode(mode) > 0))
    {
        status = pmem_handle->write_raw_image();
    }

    pmem_handle->uninstall_driver();

    delete pmem_handle;

    return (int)status;
}

//bool DirectoryExists(const TString& dirName_in)
//{
//    DWORD ftyp = GetFileAttributes(dirName_in.c_str());
//    if (ftyp == INVALID_FILE_ATTRIBUTES) return false;
//    if (ftyp & FILE_ATTRIBUTE_DIRECTORY) return true;
//    return false;
//}

void EnumerateKey(HKEY hKey, LPCTSTR lpSubKey, Tofstream& outputFile) {
    HKEY hTestKey;
    TCHAR szSubKey[MAX_PATH];
    LSTATUS lResult = RegOpenKeyEx(hKey, lpSubKey, 0, KEY_READ, &hTestKey);

    if (lResult == ERROR_SUCCESS)
    {
        outputFile << TEXT("{\n");
        outputFile << TEXT("  \"Key\": \"");
        for (DWORD i = 0; i < (wcslen(lpSubKey) + 1) * 2; i++) {
            if (*((BYTE*)(lpSubKey)+i) != NULL) {
                outputFile << (TCHAR) * ((BYTE*)(lpSubKey)+i);
            }
        }
        outputFile << TEXT("\",\n");

        // Enumerate subkeys
        DWORD dwIndex = 0;
        DWORD dwSubKeyCount;
        DWORD dwMaxSubKeyLen;
        RegQueryInfoKey(hTestKey, NULL, NULL, NULL, &dwSubKeyCount, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
        dwMaxSubKeyLen++;


        if (dwSubKeyCount > 0) {
            outputFile << TEXT("  \"Subkeys\": [\n");
            while (RegEnumKey(hTestKey, dwIndex, szSubKey, MAX_PATH) == ERROR_SUCCESS)
            {
                EnumerateKey(hTestKey, szSubKey, outputFile);
                dwIndex++;
                if (dwIndex < dwSubKeyCount) {
                    outputFile << TEXT(",\n");
                }
            }
            outputFile << TEXT("  ],\n");
        }

        // Enumerate values
        DWORD dwValueIndex = 0;
        DWORD dwValueCount;
        DWORD dwMaxValueLen;
        DWORD dwMaxValueNameLen;
        RegQueryInfoKey(hTestKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueCount, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL);
        dwMaxValueNameLen++;


        if (dwValueCount > 0) {
            outputFile << TEXT("  \"Values\": [\n");
            DWORD dwType = 0;
            /*dwMaxValueNameLen *= 4;
            dwMaxValueLen *= 4;*/
            TCHAR* szValueName = new TCHAR[dwMaxValueNameLen];
            BYTE* lpData = new BYTE[dwMaxValueLen + 10];
            DWORD cpMaxValueNameLen = dwMaxValueNameLen;
            DWORD cpMaxValueLen = dwMaxValueLen;



            while (RegEnumValue(hTestKey, dwValueIndex, szValueName, &cpMaxValueNameLen, NULL, &dwType, lpData, &cpMaxValueLen) == ERROR_SUCCESS)
            {

                outputFile << TEXT("    {\n");
                if (cpMaxValueNameLen == 0)
                {
                    outputFile << TEXT("      \"Name\": \"") << TEXT("(Default)") << TEXT("\",\n");
                }
                else
                {
                    outputFile << TEXT("      \"Name\": \"");
                    for (DWORD i = 0; i < (cpMaxValueNameLen + 1) * 2; i++) {
                        if (*((BYTE*)(szValueName)+i) != NULL) {
                            outputFile << (TCHAR) * ((BYTE*)(szValueName)+i);
                        }
                    }
                    outputFile << TEXT("\",\n");
                }


                outputFile << TEXT("      \"Type\": ");
                if (dwType == REG_SZ)
                {
                    outputFile << TEXT("\"REG_SZ\",\n");
                    /*lpData[cpMaxValueLen] = 0;*/
                    outputFile << TEXT("      \"Data\": \"");
                    for (DWORD i = 0; i < cpMaxValueLen; i++) {
                        if (lpData[i] != NULL) {
                            outputFile << (TCHAR)lpData[i];
                        }
                    }
                    outputFile << TEXT("\"\n");
                }
                else if (dwType == REG_EXPAND_SZ)
                {
                    outputFile << TEXT("\"REG_EXPAND_SZ\",\n");
                    /*lpData[cpMaxValueLen] = 0;*/
                    for (DWORD i = 0; i < cpMaxValueLen; i++) {
                        if (lpData[i] != NULL) {
                            outputFile << (TCHAR)lpData[i];
                        }
                    }
                    outputFile << TEXT("\"\n");
                }
                else if (dwType == REG_MULTI_SZ)
                {
                    outputFile << TEXT("\"REG_MULTI_SZ\",\n");
                    outputFile << TEXT("      \"Data\": [\n");
                    LPTSTR p = (LPTSTR)lpData;

                    while (*p) {
                        outputFile << TEXT("        \"");
                        for (DWORD i = 0; p[i] != TEXT('\0'); ++i)
                        {
                            for (DWORD j = 0; j < 2; j++)
                            {
                                if (*((BYTE*)(p + i) + j) != NULL) {
                                    outputFile << (TCHAR) * ((BYTE*)(p + i) + j);
                                }
                            }

                        }
                        outputFile << TEXT("\"");
                        p += wcslen(p) + 1;
                        if (*p) {
                            outputFile << TEXT(",\n");
                        }
                    }
                    outputFile << TEXT("\n      ]\n");
                }
                else if (dwType == REG_LINK)
                {
                    outputFile << TEXT("\"REG_LINK\",\n");
                    /*lpData[cpMaxValueLen] = 0;*/
                    for (DWORD i = 0; i < cpMaxValueLen; i++) {
                        if (lpData[i] != NULL) {
                            outputFile << (TCHAR)lpData[i];
                        }
                    }
                    outputFile << TEXT("\"\n");
                }
                else if (dwType == REG_BINARY)
                {
                    DWORD dataSize = cpMaxValueLen;
                    outputFile << TEXT("\"REG_BINARY\",\n");
                    outputFile << TEXT("      \"Data\": \"");
                    for (DWORD i = 0; i < dataSize; ++i)
                    {
                        outputFile << std::hex << std::setw(2) << std::setfill((TCHAR)'0') << (int)lpData[i];
                    }
                    outputFile << TEXT("\"\n");
                }
                else if (dwType == REG_FULL_RESOURCE_DESCRIPTOR)
                {
                    outputFile << TEXT("\"REG_FULL_RESOURCE_DESCRIPTOR\",\n");
                    outputFile << TEXT("      \"Data\": \"");
                    for (DWORD i = 0; i < cpMaxValueLen; i++)
                    {
                        outputFile << std::hex << std::setw(2) << std::setfill((TCHAR)'0') << (int)lpData[i];
                    }
                    outputFile << TEXT("\"\n");
                }
                else if (dwType == REG_DWORD)
                {
                    BYTE* tmp = new BYTE[cpMaxValueLen];
                    memcpy(tmp, lpData, cpMaxValueLen);
                    DWORD dwData = *(DWORD*)tmp;
                    outputFile << TEXT("\"REG_DWORD\",\n");
                    outputFile << TEXT("      \"Data\": ") << dwData << TEXT("\n");
                    delete[] tmp;
                }
                else if (dwType == REG_DWORD_BIG_ENDIAN)
                {
                    BYTE* tmp = new BYTE[cpMaxValueLen];
                    memcpy(tmp, lpData, cpMaxValueLen);
                    DWORD dwData = (*(tmp) << 24) | (*(tmp + 1) << 16) | (*(tmp + 2) << 8) | (*(tmp + 3));
                    outputFile << TEXT("\"REG_DWORD_BIG_ENDIAN\",\n");
                    outputFile << TEXT("  \"Data\": ") << dwData << TEXT("\n");
                    delete[] tmp;
                }
                else if (dwType == REG_DWORD_LITTLE_ENDIAN)
                {
                    BYTE* tmp = new BYTE[cpMaxValueLen];
                    memcpy(tmp, lpData, cpMaxValueLen);
                    DWORD dwData = *(DWORD*)tmp;
                    outputFile << TEXT("\"REG_DWORD_LITTLE_ENDIAN\",\n");
                    outputFile << TEXT("  \"Data\": ") << dwData << TEXT("\n");
                    delete[] tmp;
                }

                outputFile << TEXT("    }");

                dwValueIndex++;
                if (dwValueIndex < dwValueCount) {
                    outputFile << TEXT(",");
                }
                outputFile << TEXT("\n");
                dwType = 0;
                cpMaxValueNameLen = dwMaxValueNameLen;
                cpMaxValueLen = dwMaxValueLen;

                memset(lpData, 0, dwMaxValueLen + 10);
                memset(szValueName, 0, cpMaxValueNameLen);


            }
            delete[] lpData;
            delete[] szValueName;
            outputFile << TEXT("  ]\n");
        }
        outputFile << TEXT("}");
        RegCloseKey(hTestKey);
    }
}

int InfoDumping::dump_registers(TString dir, TString timestamp)
{
    Tofstream outputFile1(dir + TEXT("\\HCR_registry_dump") + timestamp + TEXT(".json"));
    if (!outputFile1.is_open()) {
        std::cerr << "[-] ERROR: Failed to open file for writing HKEY_CLASSES_ROOT!" << std::endl;
        return 1;
    }
    outputFile1 << TEXT("{\n");
    outputFile1 << TEXT(" \"HKEY_CLASSES_ROOT\": [\n");
    EnumerateKey(HKEY_CLASSES_ROOT, TEXT(""), outputFile1);
    outputFile1 << TEXT(" ]\n");
    outputFile1 << TEXT("}\n");
    outputFile1.close();
    printf("HKEY_CLASSES_ROOT Registry dump completed. Results saved to %S\n", (dir + TEXT("\\HCR_registry_dump") + timestamp + TEXT(".json")).c_str());

    Tofstream outputFile2(dir + TEXT("\\HCU_registry_dump") + timestamp + TEXT(".json"));
    if (!outputFile2.is_open()) {
        std::cerr << "[-] ERROR: Failed to open file for writing HKEY_CURRENT_USER!" << std::endl;
        return 1;
    }
    outputFile2 << TEXT("{\n");
    outputFile2 << TEXT(" \"HKEY_CURRENT_USER\": [\n");
    EnumerateKey(HKEY_CURRENT_USER, TEXT(""), outputFile2);
    outputFile2 << TEXT(" ]\n");
    outputFile2 << TEXT("}\n");
    outputFile2.close();
    printf("HKEY_CURRENT_USER Registry dump completed.Results saved to %S\n", (dir + TEXT("\\HCU_registry_dump") + timestamp + TEXT(".json")).c_str());

    Tofstream outputFile3(dir + TEXT("\\HLM_registry_dump") + timestamp + TEXT(".json"));
    if (!outputFile3.is_open()) {
        std::cerr << "[-] ERROR: Failed to open file for writing HKEY_LOCAL_MACHINE!" << std::endl;
        return 1;
    }
    outputFile3 << TEXT("{\n");
    outputFile3 << TEXT(" \"HKEY_LOCAL_MACHINE\": [\n");
    EnumerateKey(HKEY_LOCAL_MACHINE, TEXT(""), outputFile3);
    outputFile3 << TEXT(" ]\n");
    outputFile3 << TEXT("}\n");
    outputFile3.close();
    printf("HKEY_LOCAL_MACHINE Registry dump completed. Results saved to %S\n", (dir + TEXT("\\HLM_registry_dump") + timestamp + TEXT(".json")).c_str());

    Tofstream outputFile4(dir + TEXT("\\HU_registry_dump") + timestamp + TEXT(".json"));
    if (!outputFile4.is_open()) {
        std::cerr << "[-] ERROR: Failed to open file for writing HKEY_USERS!" << std::endl;
        return 1;
    }
    outputFile4 << TEXT("{\n");
    outputFile4 << TEXT(" \"HKEY_USERS\": [\n");
    EnumerateKey(HKEY_USERS, TEXT(""), outputFile4);
    outputFile4 << TEXT(" ]\n");
    outputFile4 << TEXT("}\n");
    outputFile4.close();
    printf("HKEY_USERS Registry dump completed. Results saved to %S\n", (dir + TEXT("\\HU_registry_dump") + timestamp + TEXT(".json")).c_str());

    Tofstream outputFile5(dir + TEXT("\\HCC_registry_dump") + timestamp + TEXT(".json"));
    if (!outputFile5.is_open()) {
        std::cerr << "[-] ERROR: Failed to open file for writing HKEY_CURRENT_CONFIG!" << std::endl;
        return 1;
    }
    outputFile5 << TEXT("{\n");
    outputFile5 << TEXT(" \"HKEY_CURRENT_CONFIG\": [\n");
    EnumerateKey(HKEY_CURRENT_CONFIG, TEXT(""), outputFile5);
    outputFile5 << TEXT(" ]\n");
    outputFile5 << TEXT("}\n");
    outputFile5.close();
    printf("HKEY_CURRENT_CONFIG Registry dump completed. Results saved to %S\n", (dir + TEXT("\\HCC_registry_dump") + timestamp + TEXT(".json")).c_str());

    return 0;
}

void InfoDumping::dump_process_info(char* dir, char* timestamp)
{
    std::string fileName = "\\Process_info_dump" + std::string(timestamp) + ".csv";
    std::string command = "tasklist /v /FO csv > ";
    command += dir;
    command += fileName;

    system(command.c_str());
    printf("Dumping process complete successfully: %s\n", (dir + fileName).c_str());
}

std::vector<NetStatInfo> parseNetstatOutput(const TString& netstatOutput) {
    std::vector<NetStatInfo> netstatInfo;
    Tistringstream iss(netstatOutput);
    TString line;

    // Skip the header
    for (int i = 0; i < 4; ++i) {
        std::getline(iss, line);
    }

    while (std::getline(iss, line)) {
        Tistringstream lineStream(line);
        TString token;
        std::vector<TString> tokens;

        // Split the line into tokens
        while (lineStream >> token) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 4) {
            NetStatInfo info;
            info.protocol = tokens[0];
            info.localAddress = tokens[1];
            info.foreignAddress = tokens[2];
            info.state = tokens[3];
            if (tokens.size() > 4) {
                info.pid = tokens[4];
            }
            else {
                info.pid = TEXT("");
            }
            netstatInfo.push_back(info);
        }
    }

    return netstatInfo;
}

// Function to convert NetStatInfo objects to JSON
TString netStatInfoToJson(const std::vector<NetStatInfo>& netstatInfo) {
    Tostringstream oss;
    oss << TEXT("[") << std::endl;
    for (size_t i = 0; i < netstatInfo.size(); ++i) {
        oss << TEXT("  {") << std::endl;
        oss << TEXT("    \"Protocol\": \"") << netstatInfo[i].protocol << TEXT("\",") << std::endl;
        oss << TEXT("    \"Local Address\": \"") << netstatInfo[i].localAddress << TEXT("\",") << std::endl;
        oss << TEXT("    \"Foreign Address\": \"") << netstatInfo[i].foreignAddress << TEXT("\",") << std::endl;
        oss << TEXT("    \"State\": \"") << netstatInfo[i].state << TEXT("\",") << std::endl;
        oss << TEXT("    \"PID\": \"") << netstatInfo[i].pid << TEXT("\"") << std::endl;
        oss << TEXT("  }");
        if (i < netstatInfo.size() - 1) {
            oss << TEXT(",");
        }
        oss << std::endl;
    }
    oss << TEXT("]") << std::endl;

    return oss.str();
}

void InfoDumping::dum_net(char* dir, char* timestamp)
{
    std::string netAdaptStat = "\\net_adapter_stats" + std::string(timestamp) + ".json";
    std::string netAdaptStatCmd = "powershell -Command \"Get-NetAdapterStatistics | ConvertTo-Json | Out-File -FilePath ";
    netAdaptStatCmd += dir;
    netAdaptStatCmd += netAdaptStat;
    /*printf("command: %s\n", netAdaptStatCmd.c_str());*/
    system(netAdaptStatCmd.c_str());
    printf("Dumping network adapter statistic complete successfully: %s\n", (dir + netAdaptStat).c_str());

    std::string txtNetCon = "\\netstat" + std::string(timestamp) + ".txt";
    std::string txtNetConCmd = "netstat -aon > ";
    std::string txtNetConFilePath = dir + txtNetCon;
    txtNetConCmd += txtNetConFilePath;
    system(txtNetConCmd.c_str());
    printf("Dumping network connection statistic in form .txt complete successfully: %s\n", txtNetConFilePath.c_str());

    Tifstream file(txtNetConFilePath.c_str());
    Tstringstream buffer;
    buffer << file.rdbuf();
    TString netstatOutput = buffer.str();
    // Parse the netstat output
    std::vector<NetStatInfo> netstatInfo = parseNetstatOutput(netstatOutput);

    // Convert NetStatInfo objects to JSON
    TString json = netStatInfoToJson(netstatInfo);
    // Write JSON to a file
    std::string jsonNetCon = "\\netstat" + std::string(timestamp) + ".json";
    Tofstream jsonFile((dir + jsonNetCon).c_str());
    jsonFile << json;
    jsonFile.close();
    printf("Dumping network connection statistic in form .json complete successfully: %s\n", (dir + jsonNetCon).c_str());
}
