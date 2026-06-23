#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "Screen.h"
#include "Report.h"
#include "Process.h"

// Unified header banner showcasing the application header details and ALL configurations
void showBanner(int numCores, const std::string& scheduler, int quantum, int batchFreq, int minIns, int maxIns, unsigned int delayPerExec) {
    std::cout << " _,.----.     ,-,--.    _,.---._        _ __        ,----.    ,-,--.                  \n";
    std::cout << ".' .' -   \\ ,-.'-  _\\ ,-.' , -  `.    .-`.' ,`.  ,-.--` , \\ ,-.'-  _\\ ,--.-.  .-,--.  \n";
    std::cout << "/==/  ,  ,-'/==/_ ,_.'/==/_,  ,  - \\ /==/, -   \\|==|-  _.-`/==/_ ,_.'/==/- / /=/_ /  \n";
    std::cout << "|==|-   |  .\\==\\  \\  |==|   .=.     |==| _ .=. ||==|   `.-.\\==\\  \\   \\==\\, \\/=/. /   \n";
    std::cout << "|==|_   `-' \\\\==\\ -\\ |==|_ : ;=:  - |==| , '=',/==/_ ,    / \\==\\ -\\   \\==\\  \\/ -/    \n";
    std::cout << "|==|   _  , |_\\==\\ ,\\|==| , '='     |==|-  '..'|==|    .-'  _\\==\\ ,\\   |==|  ,_/     \n";
    std::cout << "\\==\\.       /==/\\/ _ |\\==\\ -    ,_ /|==|,  |   |==|_  ,`-._/==/\\/ _ |  \\==\\-, /      \n";
    std::cout << " `-.`.___.-'\\==\\ - , / '.='. -   .' /==/ - |   /==/ ,     /\\==\\ - , /  /==/._/       \n";
    std::cout << "             `--`---'    `--`--''    `--`---'   `--`-----``  `--`---'   `--`-`        \n";
    std::cout << "--------------------------------------------------------------------------------------\n";
    std::cout << "Welcome to CSOPESY Emulator!\n\n";
    std::cout << "Developers: Aglugub, Jerome Andrei C.\n";
    std::cout << "SYSTEM CONFIGURATION:\n";
    std::cout << "  [num-cpu]            Number of Cores:   " << numCores << "\n";
    std::cout << "  [scheduler]          Scheduling Algo:   " << scheduler << "\n";
    std::cout << "  [quantum-cycles]     Quantum Time Slice: " << quantum << " cycle(s)\n";
    std::cout << "  [batch-process-freq] Batch Generation:  " << batchFreq << " cycle(s)\n";
    std::cout << "  [min-ins]            Min Instructions:  " << minIns << "\n";
    std::cout << "  [max-ins]            Max Instructions:  " << maxIns << "\n";
    std::cout << "  [delay-per-exec]     Execution Delay:   " << delayPerExec << " cycle(s)\n";
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

    // PRE-INITIALIZATION STAGE (Completely blank terminal guard block)
    while (true) {
        std::cout << "root:\\> ";
        std::string preInput;
        std::getline(std::cin, preInput);

        if (preInput == "initialize") {
            if (!parseConfigFile(globalNumCores, globalScheduler, globalQuantumCycles, globalBatchFreq, globalMinIns, globalMaxIns, globalDelayPerExec)) {
                std::cout << "Error: 'config.txt' could not be found or opened.\n";
                continue; 
            }

            // Provision constraints down to underlying runtime engines
            Report::setTotalCores(globalNumCores);
            Process::setGlobalDelayPerExec(globalDelayPerExec);
            break;
        } 
        // Allow a clean exit even if they haven't initialized yet
        else if (preInput == "exit") {
            return 0; 
        }
        else if (!preInput.empty()) {
            std::cout << "Error: You must run the 'initialize' command first.\n";
        }
    }

    // BOOTSTRAP SUCCESS TRANSITION 
    std::system("clear"); 
    showBanner(globalNumCores, globalScheduler, globalQuantumCycles, globalBatchFreq, globalMinIns, globalMaxIns, globalDelayPerExec);

    // MAIN RUNTIME SCHEDULER INTERACTIVE CLI COMMAND LOOP
    while (true) {
        std::cout << "root:\\> ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }
        else if (input == "screen -ls") {
            Report::printScreenList();
        }
        else if (input == "report-util") {
            if (Report::generateUtilReport()) {
                std::cout << "Report generated at csopesy-log.txt!\n";
            } else {
                std::cout << "Error creating report file.\n";
            }
        }
        // Sub-screen handling triggers can be placed right here!
    }

    return 0;
}