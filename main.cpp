#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "Screen.h"
#include "Report.h"
#include "Process.h"
#include "BatchGeneration.h"

// Unified header banner showcasing the application header details and ALL configurations as a string
std::string getBannerString(int numCores, const std::string& scheduler, int quantum, int batchFreq, int minIns, int maxIns, unsigned int delayPerExec) {
    std::stringstream ss;
    ss << " ▄████████  ▄████████    ▄██████▄     ▄███████▄    ▄████████    ▄████████   ▄██   ▄   \n";
    ss << "███    ███  ███    ███   ███    ███   ███    ███   ███    ███   ███    ███  ███   ██▄ \n";
    ss << "███    █▀   ███    █▀    ███    ███   ███    ███   ███    █▀    ███    █▀   ███▄▄▄███ \n";
    ss << "███         ███          ███    ███   ███    ███  ▄███▄▄▄       ███         ▀▀▀▀▀▀███ \n";
    ss << "███        ▀███████████  ███    ███  ▀█████████▀ ▀▀███▀▀▀     ▀███████████  ▄██   ███ \n";
    ss << "███    █▄           ███  ███    ███   ███          ███    █▄           ███  ███   ███ \n";
    ss << "███    ███    ▄█    ███  ███    ███   ███          ███    ███    ▄█    ███  ███   ███ \n";
    ss << "████████▀   ▄████████▀    ▀██████▀   ▄████▀        ██████████  ▄████████▀    ▀█████▀  \n";
    ss << "--------------------------------------------------------------------------------------\n";
    ss << "Welcome to CSOPESY Emulator!\n\n";
    ss << "Developers: Aglugub, Jerome Andrei C.\n";
    ss << "SYSTEM CONFIGURATION:\n";
    ss << "  Number of Cores:   " << numCores << "\n";
    ss << "  Scheduling Algo:   " << scheduler << "\n";
    ss << "  Quantum Slice:     " << quantum << " cycle(s)\n";
    ss << "  Batch Generation:  " << batchFreq << " cycle(s)\n";
    ss << "  Min Instructions:  " << minIns << "\n";
    ss << "  Max Instructions:  " << maxIns << "\n";
    ss << "  Execution Delay:   " << delayPerExec << " cycle(s)\n";
    ss << "--------------------------------------------------------------------------------------\n\n";
    return ss.str();
}

// Prints to console and captures it in the history buffer simultaneously
void printAndRecord(const std::string& text, std::vector<std::string>& historyBuffer) {
    std::cout << text;
    historyBuffer.push_back(text);
}

// Restores the visual state of the main console by vomiting out the entire history buffer
void restoreConsoleState(const std::vector<std::string>& historyBuffer) {
    std::system("clear"); 
    for (const auto& logEntry : historyBuffer) {
        std::cout << logEntry;
    }
}

// Function to safely load ALL 7 spec properties from your workspace config file
bool parseConfigFile(int& numCores, std::string& scheduler, int& quantum, int& batchFreq, int& minIns, int& maxIns, unsigned int& delayPerExec) {
    std::ifstream configFile("config.txt");
    if (!configFile.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        std::istringstream iss(line);
        std::string key;
        
        if (iss >> key) {
            if (key == "num-cpu") {
                iss >> numCores;
            } else if (key == "scheduler") {
                iss >> scheduler;
                if (scheduler.front() == '"' && scheduler.back() == '"') {
                    scheduler = scheduler.substr(1, scheduler.length() - 2);
                }
            } else if (key == "quantum-cycles") {
                iss >> quantum;
            } else if (key == "batch-process-freq") {
                iss >> batchFreq;
            } else if (key == "min-ins") {
                iss >> minIns;
            } else if (key == "max-ins") {
                iss >> maxIns;
            } else if (key == "delay-per-exec") {
                iss >> delayPerExec;
            }
        }
    }
    configFile.close();
    return true;
}

int main() {
    // Complete system parameter registry
    int globalNumCores = 1;
    std::string globalScheduler = "fcfs";
    int globalQuantumCycles = 1;
    int globalBatchFreq = 1;
    int globalMinIns = 1;
    int globalMaxIns = 10;
    unsigned int globalDelayPerExec = 0;

    // PRE-INITIALIZATION STAGE
    while (true) {
        std::string preInput;
        std::getline(std::cin, preInput);

        if (preInput == "initialize") {
            if (!parseConfigFile(globalNumCores, globalScheduler, globalQuantumCycles, globalBatchFreq, globalMinIns, globalMaxIns, globalDelayPerExec)) {
                std::cout << "Error: 'config.txt' could not be found or opened.\n";
                continue; 
            }

            CPU::initialize(globalNumCores, globalScheduler, globalQuantumCycles);
            Process::setGlobalDelayPerExec(globalDelayPerExec);
            BatchGeneration::initialize(globalMinIns, globalMaxIns, globalBatchFreq);
            break;
        } 
        else if (preInput == "exit") {
            return 0; 
        }
    }

    // Initialize state buffer
    std::vector<std::string> consoleHistory;

    // BOOTSTRAP SUCCESS TRANSITION 
    std::system("clear"); 
    
    // Generate the initial banner screen state and seed it directly into history
    std::string initialBanner = getBannerString(globalNumCores, globalScheduler, globalQuantumCycles, globalBatchFreq, globalMinIns, globalMaxIns, globalDelayPerExec);
    printAndRecord(initialBanner, consoleHistory);

    // MAIN RUNTIME SCHEDULER INTERACTIVE CLI COMMAND LOOP
    while (true) {
        std::cout << "root:\\> ";
        std::string input;
        std::getline(std::cin, input);

        // Store prompt and user typed value into history log sequence
        consoleHistory.push_back("root:\\> " + input + "\n");

        if (input == "exit") {
            std::cout << "Terminating worker threads and closing console safely...\n";
            
            BatchGeneration::shutdown();
            CPU::shutdown();
            
            break; 
        }
        else if (input == "screen -ls") {
            // Note: If printScreenList directly uses std::cout internally, you may want to capture 
            // its response loop within its own reporting class using a string buffer or printAndRecord.
            std::string listOutput = Report::printScreenList();
            
            // Vomit it to the console and record it into history all at once!
            printAndRecord(listOutput, consoleHistory);
        }
        else if (input == "scheduler-start") {
            BatchGeneration::start();
            printAndRecord("Batch generation activated. Dummy processes are spawning...\n", consoleHistory);
        }
        else if (input == "scheduler-stop") {
            BatchGeneration::stop();
            printAndRecord("Batch process generation paused.\n", consoleHistory);
        }
        else if (input == "report-util") {
            if (Report::generateUtilReport()) {
                printAndRecord("Utilization metrics logged to csopesy-log.txt!\n", consoleHistory);
            } else {
                printAndRecord("Error writing utility log file.\n", consoleHistory);
            }
        }
        else if (input.rfind("screen", 0) == 0) {
            
            std::vector<std::string> tokens;
            std::size_t start = 0, end = 0;
            
            while ((end = input.find(' ', start)) != std::string::npos) {
                if (end != start){
                    tokens.push_back(input.substr(start, end - start));
                }
                start = end + 1;
            }
            if (start < input.size()){ 
                tokens.push_back(input.substr(start));
            } 

            if (tokens.size() == 3) {
                std::string mode = tokens[1];
                std::string name = tokens[2];

                if (mode == "-s") {
                    if (Screen::enterProcessScreen(name, globalMinIns, globalMaxIns)) {
                        // User ran exit from process view. Reprint full ledger history.
                        restoreConsoleState(consoleHistory);
                    }
                } 
                else if (mode == "-r") {
                    if (Screen::enterProcessScreen(name)) {
                        // User ran exit from process view. Reprint full ledger history.
                        restoreConsoleState(consoleHistory);
                    }
                }
                else {
                    printAndRecord("Invalid flag. Use 'screen -s [name]' or 'screen -r [name]'.\n", consoleHistory);
                }
            } 
            else {
                printAndRecord("Usage error. Format expected: 'screen [-s/-r] [name]'.\n", consoleHistory);
            }
        }
        else {
            printAndRecord("Command not recognized.\n", consoleHistory);
        }
    }

    return 0;
}