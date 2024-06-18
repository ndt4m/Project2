#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <getopt.h>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <map>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#define File 0
#define Dir 1
#define SIZE (1024*16)

struct box
{
   short fileType;
   unsigned long long fileSize;
   char data[SIZE];
};

struct clientInfo
{
   int sockfd;
   struct sockaddr_in cli_in;
   unsigned int interval;
};

int sendAll(size_t size, char* src, int sockfd, clientInfo* cinf)
{

   int totalBytesSent = 0;
   int bytesSent = 0;
   
   while (totalBytesSent < size)
   {
      
      bytesSent = write(sockfd, src + totalBytesSent, size - totalBytesSent);
      
      if (bytesSent < 0)
      {
         std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
         std::cout << "[+] Disconnected the client from " << inet_ntoa(cinf->cli_in.sin_addr) << ":" << ntohs(cinf->cli_in.sin_port) << "\n";
       
         close(sockfd);
         return 1;
      }
      else if (bytesSent == 0)
      {
         std::cout << "[+] Client from " << inet_ntoa(cinf->cli_in.sin_addr) << ":" << ntohs(cinf->cli_in.sin_port) << " disconnected\n";
         close(sockfd);
         return 1;
      }

      totalBytesSent += bytesSent;
   }

   return 0;
}

int receiveAll(size_t size, char* dest, int sockfd, clientInfo* cinf)
{

   int totalBytesReceived = 0;
   int bytesReceived = 0;

   while (totalBytesReceived < size)
   {
      bytesReceived = read(sockfd, dest + totalBytesReceived, size - totalBytesReceived);

      if (bytesReceived < 0)
      {
         std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
         std::cout << "[+] Disconnected the client from " << inet_ntoa(cinf->cli_in.sin_addr) << ":" << ntohs(cinf->cli_in.sin_port) << "\n";
         close(sockfd);
         return 1;
      }
      else if (bytesReceived == 0)
      {
         std::cout << "[+] Client from " << inet_ntoa(cinf->cli_in.sin_addr) << ":" << ntohs(cinf->cli_in.sin_port) << " disconnected\n";
         close(sockfd);
         return 1;
      }

      totalBytesReceived += bytesReceived;
   }

   return 0;
}

int writeFile(int sockfd, clientInfo* cinf, std::string fileName, unsigned long long fileSize, char* cwd)
{  
   
   unsigned long long &bytesLeftToReceive = fileSize;

   std::string newName = std::string(cwd) + "/" + std::string(fileName);
   printf("%s\n", newName.c_str());
   FILE* f = fopen(newName.c_str(), "wb");
   if (f == NULL)
   {
      std::cerr << "[-] ERROR: Can't create new file\n";
      std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
      fclose(f);

      close(sockfd);
      return 1;
   }

   char receiveBuffer[SIZE];
   bzero(receiveBuffer, sizeof(receiveBuffer));

   int bytesToReceive = -1;
   
   while(bytesLeftToReceive > 0)
   {
      bytesToReceive = std::min<unsigned long long>(bytesLeftToReceive, SIZE);
      
      if (receiveAll(bytesToReceive, receiveBuffer, sockfd, cinf) == 1)
      {
         return 1;
      }

      fwrite(receiveBuffer, 1, bytesToReceive, f);

      bzero(receiveBuffer, SIZE);

      bytesLeftToReceive = bytesLeftToReceive - bytesToReceive;

   }

   fclose(f);
   return 0;
}

int writeDir(int sockfd, clientInfo* cinf, int flag, char* cwd)
{
   char msg[sizeof(box)];
   box receiver;

   bzero(&receiver, sizeof(box));
   bzero(msg, sizeof(msg));
   
   if (receiveAll(sizeof(msg), msg, sockfd, cinf) == 1)
   {
      return 1;
   }

   if (strcmp(msg, "success") != 0)
   {
      return 1;
   }

   if (receiveAll(sizeof(msg), msg, sockfd, cinf) == 1)
   {
      return 1;
   }

   memcpy(&receiver, msg, sizeof(box));
   
   if (receiver.fileType == File)
   {
        printf("file\n");
        if (writeFile(sockfd, cinf, std::string(receiver.data), receiver.fileSize, cwd) == 1)
        {
            return 1;
        }

        bzero(&receiver, sizeof(box));
        bzero(msg, sizeof(msg));

        return 0;
   }
   else // Case the file is Directory
   {

        std::string newDirName;
        if (flag == 1)
        {
            newDirName = std::string(cwd) + "/" + std::string(inet_ntoa(cinf->cli_in.sin_addr)) + "_" + std::to_string(ntohs(cinf->cli_in.sin_port));
            if (mkdir(newDirName.c_str(), 0777) != 0)
            {
               if (errno != EEXIST)
               {
                  printf("[-] dirname: %s\n", newDirName.c_str());
                  std::cerr << "[-] ERROR: Can't create new dir\n";
                  std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
                  exit(1);
               }
      
            }
            printf("dirname: %s\n", newDirName.c_str());
            newDirName +=  "/" + std::string(receiver.data);
        }
        else
        {
            newDirName = std::string(cwd) + "/" + std::string(receiver.data);
        }
        
        if (mkdir(newDirName.c_str(), 0777) != 0)
        {
            printf("dirname: %s\n", newDirName.c_str());
            std::cerr << "[-] ERROR: Can't create new dir\n";
            std::cerr << "[-] ERROR: " << strerror(errno) << "\n";
            exit(1);

        }
        printf("dirname: %s\n", newDirName.c_str());  


        int loopFlag = -1; 

        if (receiveAll(sizeof(int), (char*)&loopFlag, sockfd, cinf) == 1)
        {
            return 1;
        }


        while (loopFlag == 1)
        {
            writeDir(sockfd, cinf, 0, (char*) newDirName.c_str());
            if (receiveAll(sizeof(int), (char*)&loopFlag, sockfd, cinf) == 1)
            {
                return 1;
            }
        }
      return 0;
   }
   
}

void* clientThread(void* arg)
{
   struct clientInfo* cinf = (clientInfo*) arg;
   int sockfd = cinf->sockfd;

   char msg[sizeof(box)];
   bzero(msg, sizeof(msg));
   char cwd[PATH_MAX];
   getcwd(cwd, PATH_MAX);
   while (true)
   {
      strncpy(msg, "dump", strlen("dump")+1);
      if (sendAll(sizeof(msg), msg, sockfd, cinf) == 1)
      {
        printf("[-] ERROR: Can't sent dumping message\n");
        break;
      }
      if (writeDir(sockfd, cinf, 1, cwd) == 1)
      {
        break;
      }
      std::cout << "[+] Successfully receive dumping from " << inet_ntoa(cinf->cli_in.sin_addr) << ":" << ntohs(cinf->cli_in.sin_port) << std::endl;
      bzero(msg, sizeof(msg));
      sleep(cinf->interval);
   }
   delete cinf;
   return nullptr;
}


int main(int argc, char* argv[])
{
    int opt;
    int port_num = -1;
    unsigned int interval = -1;
    struct option long_options[] = {
       {"port", required_argument, NULL, 'p'},
       {"interval", required_argument, NULL, 'i'},
       {"help", no_argument, NULL, 'h'},
       {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hp:i:", long_options, NULL)) != -1)
    {
       switch (opt)
       {
          case 'h':
             std::cout << "[+] Usage: ./fileName [OPTIONS]\n"
                       << "[+] Description: Execute command and Return result to the client.\n"
                       << "[+] Options:\n"
                       << "             -h, --help: Display the usage of the program.\n"
                       << "             -p, --port: Specify the port number the server uses to receive connections from clients.\n"
                       << "             -i, --interval: Specify the interval in second that the server sends a dumping signal to clients.\n";
                       
             exit(0);
             break;

          case 'p':
             port_num = atoi(optarg);
             break;
          case 'i':
             interval = atoi(optarg);
             break;
          default:
             exit(1);
             break;
       }
    }

    if (port_num == -1)
    {
       std::cerr << "Option -p/--port are required\n";
       exit(1);
    }
    if (interval == -1)
    {
       std::cerr << "Option -i/--interval are required\n";
       exit(1);
    }

	// Client socket address structures
	struct sockaddr_in cliAddr;
	socklen_t addr_size = sizeof(cliAddr);

	
	int listeningSockfd = socket(AF_INET, SOCK_STREAM, 0);

	
	if (listeningSockfd < 0) {
		std::cerr << "[-] ERROR: Can't create a socket for listening\n";
      exit(1);
	}

	printf("Server Socket is created.\n");

   // Bind the IP address and port to a socket
	sockaddr_in serv_addr;
   serv_addr.sin_family = AF_INET; // specify the address family is IPv4. 
   serv_addr.sin_port = htons(port_num); //set the port number,
   serv_addr.sin_addr.s_addr = INADDR_ANY; // bind to all available network interfaces on the host.


	if (bind(listeningSockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
   {
      std::cerr << "[-] ERROR: Can't bind the IP address and port of the server to a socket\n";
      exit(1);
   }

	// Mark this socket is for listening
   if (listen(listeningSockfd, 10) < 0) 
   {
      std::cerr <<"[-] ERROR: Can't mark the socket for listening\n";
      exit(1);
   }

   std::cout << "[*] Start server successfully in " << inet_ntoa(serv_addr.sin_addr) << ":" << ntohs(serv_addr.sin_port) << std::endl;
    
	while (true) 
   {
		// Accept clients and
		// store their information in cliAddr
		int clientSocket = accept(listeningSockfd, (struct sockaddr*)&cliAddr, &addr_size);
      struct clientInfo* cinf = new clientInfo;
      
		// Error handling
		if (clientSocket < 0) 
      {
		   std::cerr << "[-] ERROR: Can't connect to the " << inet_ntoa(cliAddr.sin_addr) << ":" << ntohs(cliAddr.sin_port) << "\n";
         exit(1);
		}
		// Displaying information of
		// connected client
      cinf->sockfd = clientSocket;
      
      cinf->interval = interval;
      cinf->cli_in = cliAddr;
		std::cout << "[+] New client connect from " << inet_ntoa(cliAddr.sin_addr) << ":" << ntohs(cliAddr.sin_port) << std::endl;

      pthread_t x;
      int ret = pthread_create(&x, NULL, &clientThread, cinf);
      if (ret != 0)
      {
         std::cerr << "[-] ERROR: Can't create thread for client from " << inet_ntoa(cliAddr.sin_addr) << ":" << ntohs(cliAddr.sin_port) << "\n";
      }
		
	}

	close(listeningSockfd);
	return 0;
}

