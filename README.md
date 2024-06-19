# Tool to Dump Information from Compromised Network Nodes
## Introduction
This project is about "Building a tool to dump information from compromised network nodes". In this topic, I will focus on building a tool including server and client to extract the needed information from the nodes which is marked compromised. This tool can be used in the forensic investigations in cyberattacks. By using this tool we can gather as much information from the compromised network nodes as possible, after that the analyzing process will be taken place to produce the IOCs (Indicator of Compromised) for preventing future attacks.
## Repository Structure
The server source code and executable is place in `server_linux` folder

The client source code and executable is place in `InfoDumping` folder

The `kernel` folder is the source code of the open source tool `WinpMemp`
## Deployment

In this project, both the client and server are portable executables. This means that no additional libraries are required. The only necessary files for deployment are the executables themselves.

### Server

The server is designed to run on a Linux operating system machine. To run the server, you need a Linux machine.

#### Building the Server from Source

If you want to build the program from the source, locate the file named `server.cpp` in the `server_linux` folder and execute the following command:

```sh
g++ server.cpp -lpthread -o server
`````
Running the Server
The server requires two arguments: Port (-p, --port): The port that the server listens on. Interval (-i, --interval): The period of time (in seconds) that the dumping signal will be sent after the previous signal. Additionally, the server has a help argument (-h, --help) that briefly describes how to run the server.

```sh
Usage
./server [OPTIONS]
Options
-h, --help: Display the usage of the program.
-p, --port: Specify the port number the server uses to receive connections from clients.
-i, --interval: Specify the interval in seconds that the server sends a dumping signal to clients.
`````
### Client
The client is designed to run on a Windows operating system machine. Therefore, to run the client, you need a Windows machine. Additionally, because the client needs to extract RAM and registries, it needs to be run in Administration mode.

#### Building the Client from Source
If you want to build the client from source code, open the project in the InfoDumping folder in Visual Studio, choose Debug, x64, and build it again. This project was developed using Visual Studio 2022 version 17.6.2.

Running the Client requires three arguments: IP address of the server (-i): The IP address of the server to connect to. Port of the server (-p): The port of the server. Dump directory name (-d): The name of the folder where all the dumping data that the client extracted will be stored. The client will display its usage if no arguments or incorrect arguments are provided.
```sh
Usage
.\InfoDumping.exe [options]
Options
-i: Specify the server IP address.
-p: Specify the server port.
-d: Specify the dump directory name.
`````
