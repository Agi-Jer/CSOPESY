#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "Screen.h"
#include "Report.h"
#include "Process.h"
#include "BatchGeneration.h"

// Unified header banner showcasing the application header details and ALL configurations
void showBanner(int numCores, const std::string& scheduler, int quantum, int batchFreq, int minIns, int maxIns, unsigned int delayPerExec) {
    std::cout << " ▄████████  ▄████████    ▄██████▄     ▄███████▄    ▄████████    ▄████████   ▄██   ▄   \n";
    std::cout << "███    ███  ███    ███   ███    ███   ███    ███   ███    ███   ███    ███  ███   ██▄ \n";
    std::cout << "███    █▀   ███    █▀    ███    ███   ███    ███   ███    █▀    ███    █▀   ███▄▄▄███ \n";
    std::cout << "███         ███          ███    ███   ███    ███  ▄███▄▄▄       ███         ▀▀▀▀▀▀███ \n";
    std::cout << "███        ▀███████████  ███    ███  ▀█████████▀ ▀▀███▀▀▀     ▀███████████  ▄██   ███ \n";
    std::cout << "███    █▄           ███  ███    ███   ███          ███    █▄           ███  ███   ███ \n";
    std::cout << "███    ███    ▄█    ███  ███    ███   ███          ███    ███    ▄█    ███  ███   ███ \n";
    std::cout << "████████▀   ▄████████▀    ▀██████▀   ▄████▀        ██████████  ▄████████▀    ▀█████▀  \n";
    std::cout << "--------------------------------------------------------------------------------------\n";
    std::cout << "Welcome to CSOPESY Emulator!\n\n";
    std::cout << "Developers: Aglugub, Jerome Andrei C.\n";
    std::cout << "SYSTEM CONFIGURATION:\n";
    std::cout << "  Number of Cores:   " << numCores << "\n";
    std::cout << "  Scheduling Algo:   " << scheduler << "\n";
    std::cout << "  Quantum Slice:     " << quantum << " cycle(s)\n";
    std::cout << "  Batch Generation:  " << batchFreq << " cycle(s)\n";
    std::cout << "  Min Instructions:  " << minIns << "\n";
    std::cout << "  Max Instructions:  " << maxIns << "\n";
    std::cout << "  Execution Delay:   " << delayPerExec << " cycle(s)\n";
    std::cout << "--------------------------------------------------------------------------------------\n\n";
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
                // Strips quotation marks around strings like "rr" or "fcfs"
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
        std::cout << "root:\\> ";
        std::string preInput;
        std::getline(std::cin, preInput);

        if (preInput == "initialize") {
            if (!parseConfigFile(globalNumCores, globalScheduler, globalQuantumCycles, globalBatchFreq, globalMinIns, globalMaxIns, globalDelayPerExec)) {
                std::cout << "Error: 'config.txt' could not be found or opened.\n";
                continue; 
            }

            // Place the Config to the correct classes
            CPU::initialize(globalNumCores, globalScheduler, globalQuantumCycles);
            Process::setGlobalDelayPerExec(globalDelayPerExec);
            BatchGeneration::initialize(globalMinIns, globalMaxIns, globalBatchFreq);
            break;
        } 
        // Allow a clean exit even if they haven't initialized yet
        else if (preInput == "exit") {
            return 0; 
        }
    }

    // BOOTSTRAP SUCCESS TRANSITION 
    std::system("clear"); 
    showBanner(globalNumCores, globalScheduler, globalQuantumCycles, globalBatchFreq, globalMinIns, globalMaxIns, globalDelayPerExec);

    // MAIN RUNTIME SCHEDULER INTERACTIVE CLI COMMAND LOOP
    while (true) {
        if (input == "exit") {
            std::cout << "Terminating worker threads and closing console safely...\n";
            
            // STATELESS CLEANUP: Order matters to prevent hanging blocks
            BatchGeneration::shutdown();
            CPU::shutdown();
            
            break; // Breaks out of the while(true) loop to end main()
        }
        else if (input == "screen -ls") {
            Report::printScreenList();
        }
        else if (input == "scheduler-start") {
            BatchGeneration::start();
            std::cout << "Batch generation activated. Dummy processes are spawning...\n";
        }
        else if (input == "scheduler-stop") {
            BatchGeneration::stop();
            std::cout << "Batch process generation paused.\n";
        }
        else if (input == "report-util") {
            if (Report::generateUtilReport()) {
                std::cout << "Utilization metrics logged to csopesy-log.txt!\n";
            } else {
                std::cout << "Error writing utility log file.\n";
            }
        }
        //Make sure to go last so it doesnt mess with screen -ls
        else if (input.rfind("screen", 0) == 0) {
            
            // Trim or break apart options via parsing spaces
            std::vector<std::string> tokens;
            std::string token;
            std::size_t start = 0, end = 0;
            
            //Tokenize
            while ((end = input.find(' ', start)) != std::string::npos) {
                if (end != start){
                    tokens.push_back(input.substr(start, end - start));
                }
                start = end + 1;
            }
            if (start < input.size()){ //Get Last Substring
                tokens.push_back(input.substr(start));
            } 

            // Check if argument structural bounds are acceptable
            if (tokens.size() == 3) {
                std::string mode = tokens[1];
                std::string name = tokens[2];

                if (mode == "-s") {
                    // Spawn and Attach using global config defaults passed from main
                    Screen::enterProcessScreen(name, globalMinIns, globalMaxIns);
                } 
                else if (mode == "-r") {
                    // Standard re-attach execution handle
                    Screen::enterProcessScreen(name);
                } 
                else {
                    std::cout << "Invalid flag. Use 'screen -s [name]' or 'screen -r [name]'.\n";
                }
            } 
            else {
                std::cout << "Usage error. Format expected: 'screen [-s/-r] [name]'.\n";
            }
        }
        else {
            std::cout << "Command not recognized.\n";
        }
    }

    return 0;
}