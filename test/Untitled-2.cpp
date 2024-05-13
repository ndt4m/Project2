#include <iostream>
#include <cstdlib> // For system() function
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

#ifdef UNICODE
typedef std::wstring TString;
#else
typedef TString TString;
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
typedef std::wistringstream Tistringstream;
#endif


// Struct to store task information
struct TaskInfo {
    TString imageName;
    TString pid;
    TString sessionName;
    TString sessionNum;
    TString memUsage;
    TString status;
    TString userName;
    TString cpuTime;
    TString windowTitle;
};

// Function to parse tasklist output
std::vector<TaskInfo> parseTasklistOutput(const TString& tasklistOutput) {
    std::vector<TaskInfo> tasks;
    Tistringstream iss(tasklistOutput);
    TString line;

    // Skip the header
    std::getline(iss, line);

    while (std::getline(iss, line)) {
        Tistringstream lineStream(line);
        TString token;
        std::vector<TString> tokens;

        // Split the line into tokens
        while (std::getline(lineStream, token, ',')) {
            tokens.push_back(token);
        }

        // Create a TaskInfo object and populate its fields
        TaskInfo task;
        task.imageName = tokens[0];
        task.pid = tokens[1];
        task.sessionName = tokens[2];
        task.sessionNum = tokens[3];
        task.memUsage = tokens[4];
        task.status = tokens[5];
        task.userName = tokens[6];
        task.cpuTime = tokens[7];
        task.windowTitle = tokens[8];

        tasks.push_back(task);
    }

    return tasks;
}

// Function to convert TaskInfo objects to JSON
TString taskInfoToJson(const std::vector<TaskInfo>& tasks) {
    Tostringstream oss;
    oss << TEXT("[") << std::endl;
    for (size_t i = 0; i < tasks.size(); ++i) {
        oss << TEXT("  {") << std::endl;
        oss << TEXT("    \"Image Name\": \"") << tasks[i].imageName << TEXT("\",") << std::endl;
        oss << TEXT("    \"PID\": \"") << tasks[i].pid << TEXT("\",") << std::endl;
        oss << TEXT("    \"Session Name\": \"") << tasks[i].sessionName << TEXT("\",") << std::endl;
        oss << TEXT("    \"Session#\": \"") << tasks[i].sessionNum << TEXT("\",") << std::endl;
        oss << TEXT("    \"Mem Usage\": \"") << tasks[i].memUsage << TEXT("\",") << std::endl;
        oss << TEXT("    \"Status\": \"") << tasks[i].status << TEXT("\",") << std::endl;
        oss << TEXT("    \"User Name\": \"") << tasks[i].userName << TEXT("\",") << std::endl;
        oss << TEXT("    \"CPU Time\": \"") << tasks[i].cpuTime << TEXT("\",") << std::endl;
        oss << TEXT("    \"Window Title\": \"") << tasks[i].windowTitle << TEXT("\"") << std::endl;
        oss << TEXT("  }");
        if (i < tasks.size() - 1) {
            oss << TEXT(",");
        }
        oss << std::endl;
    }
    oss << TEXT("]") << std::endl;

    return oss.str();
}

int main() {
    // Run the tasklist command
    system("tasklist /v /FO csv > tasks.csv");

    // Read the output from the CSV file
    Tifstream file("tasks.csv");
    Tstringstream buffer;
    buffer << file.rdbuf();
    TString tasklistOutput = buffer.str();

    // Parse the tasklist output
    std::vector<TaskInfo> tasks = parseTasklistOutput(tasklistOutput);

    // Convert TaskInfo objects to JSON
    TString json = taskInfoToJson(tasks);

    // Write JSON to a file
    Tofstream jsonFile("tasks.json");
    jsonFile << json;
    jsonFile.close();

    return 0;
}
