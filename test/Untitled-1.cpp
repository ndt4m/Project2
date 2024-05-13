
#include <iostream>
#include <fstream>
#include <tchar.h>
#include <iomanip>
#include <windows.h>

void EnumerateKey(HKEY hKey, LPCTSTR lpSubKey, std::ofstream& outputFile) {
    HKEY hTestKey;
    TCHAR szSubKey[MAX_PATH];

    if (RegOpenKeyEx(hKey, lpSubKey, 0, KEY_READ, &hTestKey) == ERROR_SUCCESS) {
        outputFile << "{\n";
        outputFile << "  \"Key\": \"" << lpSubKey << "\",\n";

        // Enumerate subkeys
        DWORD dwIndex = 0;
        DWORD dwSubKeyCount;
        DWORD dwMaxSubKeyLen;
        RegQueryInfoKey(hTestKey, NULL, NULL, NULL, &dwSubKeyCount, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
        dwMaxSubKeyLen++;
        // if (lstrcmp(lpSubKey, stopSubkey) == 0) 
        // {
        //     printf("dwSubKeyCount: %d\n", dwSubKeyCount);
        // }
        if (dwSubKeyCount > 0) {
            outputFile << "  \"Subkeys\": [\n";
            while (RegEnumKey(hTestKey, dwIndex, szSubKey, MAX_PATH) == ERROR_SUCCESS) {
                EnumerateKey(hTestKey, szSubKey, outputFile);
                dwIndex++;
                if (dwIndex < dwSubKeyCount) {
                    outputFile << ",\n";
                }
            }
            outputFile << "  ],\n";
        }

        // Enumerate values
        DWORD dwValueIndex = 0;
        DWORD dwValueCount;
        DWORD dwMaxValueLen;
        DWORD dwMaxValueNameLen;
        RegQueryInfoKey(hTestKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueCount, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL);
        dwMaxValueNameLen++;
        
        
        if (dwValueCount > 0) {
            outputFile << "  \"Values\": [\n";
            DWORD dwSize = 0;
            DWORD dwType = 0;
            dwMaxValueNameLen *= 4;
            dwMaxValueLen *= 4;
            TCHAR* szValueName = new TCHAR[dwMaxValueNameLen];
            BYTE* lpData = new BYTE[dwMaxValueLen];
            DWORD cpMaxValueNameLen = dwMaxValueNameLen;
            DWORD cpMaxValueLen = dwMaxValueLen;
            // if (lstrcmp(lpSubKey, stopSubkey) == 0) 
            // {
                printf("valueCount: %d\n", dwValueCount);
                printf("dwMaxValueLen: %d\n", dwMaxValueLen);
                printf("dwMaxValueNameLen: %d\n", dwMaxValueNameLen);
                printf("dwSize: %d\n", dwSize);
                printf("%s\n", lpSubKey);
                exit(0);
            // }
            while (
                    (RegEnumValue(hTestKey, dwValueIndex, szValueName, &dwMaxValueNameLen, NULL, &dwType, lpData, &dwSize) == ERROR_MORE_DATA
                    && RegEnumValue(hTestKey, dwValueIndex, szValueName, &dwMaxValueNameLen, NULL, &dwType, lpData, &(dwSize*=4)) == ERROR_SUCCESS)) 
            {
                // if (lstrcmp(lpSubKey, stopSubkey) == 0) 
                // {
                //     printf("dwValueIndex: %d\n", dwValueIndex);
                //     printf("szValueName: %s\n", szValueName);
                //     printf("dwMaxValueNameLen: %d\n", dwMaxValueNameLen);
                //     printf("dwMaxValueLen: %d\n", dwMaxValueLen);
                //     printf("dwSize: %d\n", dwSize);
                // }
                outputFile << "    {\n";
                if (dwMaxValueNameLen == 0)
                {
                    outputFile << "      \"Name\": \"" << "(Default)" << "\",\n";
                }
                else
                {
                    outputFile << "      \"Name\": \"" << szValueName << "\",\n";
                }
                outputFile << "      \"Type\": ";
                if (dwType == REG_SZ) 
                {
                    outputFile << "\"REG_SZ\",\n";
                    lpData[dwSize] = 0;
                    outputFile << "      \"Data\": \"" << lpData << "\"\n";
                }
                else if (dwType == REG_EXPAND_SZ)
                {
                    outputFile << "\"REG_EXPAND_SZ\",\n";
                    lpData[dwSize] = 0;
                    outputFile << "      \"Data\": \"" << lpData << "\"\n";
                } 
                else if (dwType == REG_MULTI_SZ)
                {
                    outputFile << "\"REG_MULTI_SZ\",\n";
                    lpData[dwSize] = 0;
                    outputFile << "      \"Data\": \"" << lpData << "\"\n";
                }
                else if (dwType == REG_LINK)
                {
                    outputFile << "\"REG_LINK\",\n";
                    lpData[dwSize] = 0;
                    outputFile << "      \"Data\": \"" << lpData << "\"\n";
                }
                else if (dwType == REG_BINARY) 
                {
                    DWORD dataSize = dwSize;
                    outputFile << "\"REG_BINARY\",\n";
                    outputFile << "      \"Data\": \"0x";
                    for (DWORD i = 0; i < dataSize; ++i) 
                    {
                        outputFile << std::hex << std::setw(2) << std::setfill('0') << (int)lpData[i];
                    }
                    outputFile << "\"\n";
                }
                else if (dwType == REG_FULL_RESOURCE_DESCRIPTOR) 
                {
                    outputFile << "\"REG_FULL_RESOURCE_DESCRIPTOR\",\n";
                    outputFile << "      \"Data\": \"";
                    for (DWORD i = 0; i < dwSize; i++) 
                    {
                        outputFile << std::hex << std::setw(2) << std::setfill('0') << (int)lpData[i];
                    }
                    outputFile << "\"\n";
                }
                else if (dwType == REG_DWORD) 
                {
                    BYTE* tmp = new BYTE[dwSize];
                    memcpy(tmp, lpData, dwSize);
                    DWORD dwData = *(DWORD*)tmp;
                    outputFile << "\"REG_DWORD\",\n";
                    outputFile << "      \"Data\": " << dwData << "\n";
                    delete[] tmp;
                }
                else if (dwType == REG_DWORD_BIG_ENDIAN) 
                {
                    BYTE* tmp = new BYTE[dwSize];
                    memcpy(tmp, lpData, dwSize);
                    DWORD dwData = (*(tmp) << 24) | (*(tmp + 1) << 16) | (*(tmp + 2) << 8) | (*(tmp + 3));
                    outputFile << "\"REG_DWORD_BIG_ENDIAN\",\n";
                    outputFile << "  \"Data\": " << dwData << "\n";
                    delete[] tmp;
                }
                else if (dwType == REG_DWORD_LITTLE_ENDIAN) 
                {
                    BYTE* tmp = new BYTE[dwSize];
                    memcpy(tmp, lpData, dwSize);
                    DWORD dwData = *(DWORD*)tmp;
                    outputFile << "\"REG_DWORD_LITTLE_ENDIAN\",\n";
                    outputFile << "  \"Data\": " << dwData << "\n";
                    delete[] tmp;
                }

                outputFile << "    }";
                dwValueIndex++;
                if (dwValueIndex < dwValueCount) {
                    outputFile << ",";
                }
                outputFile << "\n";
                dwSize = 0;
                dwType = 0;
                dwMaxValueLen = cpMaxValueLen;
                dwMaxValueNameLen = cpMaxValueNameLen;
                // if (lstrcmp(lpSubKey, stopSubkey) == 0) 
                // {
                //     printf("dwValueIndex: %d\n", dwValueIndex);
                //     printf("szValueName: %s\n", szValueName);
                //     printf("dwMaxValueNameLen: %d\n", dwMaxValueNameLen);
                //     printf("dwMaxValueLen: %d\n", dwMaxValueLen);
                //     printf("dwSize: %d\n", dwSize);
                // }
                memset(lpData, 0, dwMaxValueLen);
                memset(szValueName, 0, cpMaxValueNameLen);
                // if (lstrcmp(lpSubKey, stopSubkey) == 0) 
                // {
                //     printf("dwValueIndex: %d\n", dwValueIndex);
                //     printf("szValueName: %s\n", szValueName);
                //     printf("dwMaxValueNameLen: %d\n", dwMaxValueNameLen);
                //     printf("dwMaxValueLen: %d\n", dwMaxValueLen);
                //     printf("dwSize: %d\n", dwSize);
                // }

            }
            delete[] lpData;
            outputFile << "  ]\n";
        }
        outputFile << "}";
        RegCloseKey(hTestKey);
    }
}

int main() {
    std::ofstream outputFile("registry_dump.json");
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open file for writing!" << std::endl;
        return 1;
    }
try
    {
        outputFile << "{\n";
        // outputFile << " \"HKEY_LOCAL_MACHINE\": [\n";
        // EnumerateKey(HKEY_LOCAL_MACHINE, TEXT(""), outputFile);
        // outputFile << " ],\n";
        outputFile << " \"HKEY_LOCAL_MACHINE\": [\n";
        EnumerateKey(HKEY_LOCAL_MACHINE, TEXT(""), outputFile);
        outputFile << " ]\n";
        outputFile << "}\n";
        outputFile.close();
        std::cout << "Registry dump completed. Results saved to registry_dump.json" << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        outputFile.close();
        return 1;
    }
    return 0;
    return 0;
}
