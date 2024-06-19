
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <winsock2.h>
#include <string>
#include <sstream>
#include <fstream>
#include <tchar.h>
#include <stdio.h>

#define File 0
#define Dir 1
#define SIZE (1024*16)

#ifdef UNICODE
typedef std::wstring TString;
#else
typedef std::string TString;
#endif

#ifdef UNICODE
typedef std::wostringstream Tostringstream;
#else
typedef std::ostringstream Tostringstream;
#endif

#ifdef UNICODE
typedef std::wstringstream Tstringstream;
#else
typedef std::stringstream Tstringstream;
#endif

#ifdef UNICODE
typedef std::wifstream Tifstream;
#else
typedef std::ifstream Tifstream;
#endif

#ifdef UNICODE
typedef std::wofstream Tofstream;
#else
typedef std::ofstream Tofstream;
#endif

#ifdef UNICODE
typedef std::wistringstream Tistringstream;
#else
typedef std::istringstream Tistringstream;
#endif

struct NetStatInfo
{
    TString protocol;
    TString localAddress;
    TString foreignAddress;
    TString state;
    TString pid;
};

struct box
{
    short fileType;
    unsigned long long fileSize;
    char data[SIZE];
};


class InfoDumping
{
public:
    virtual int dump_ram(TString dir, TString timestamp);
    virtual int dump_registries(TString dir, TString timestamp);
    virtual void dump_process_info(char* dir, char* timestamp);
    virtual void dum_net(char* dir, char* timestamp);
    virtual SOCKET constructConnection(TString port_number, TString server_address);
    virtual int sendDir(std::string fileName, SOCKET sockfd);
    virtual int receiveAll(size_t size, char* dest, SOCKET sockfd);
    virtual int parseArgs(int argc, TCHAR** argv, TCHAR** port_number, TCHAR** server_address, TCHAR** output_dir_name);
};
