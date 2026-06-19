#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "CPU.h"
#include "RQ.h"
#include "Process.h"
#include "Screen.h"

int main() {
    std::cout << "Initializing OS Emulator..." << std::endl;

    // 1. Declare 4 cores for the CPU scheduler as required by specifications
    int numCores = 4;
    CPU myCPU(numCores);

    // 2. Record the following test case: Create 10 processes, each with 100 print commands
    std::cout << "Generating 10 test processes (100 PRINT instructions each)..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        // Passing "PRINT" auto-increments the static PID and names them screen_0, screen_1...
        // It also fills its own vector with 100 PRINT instructions completely behind the scenes.
        Process p("PRINT", 100);
        
        // Push onto the pseudo-global thread-safe RQ
        RQ::addToReady(p);
    }
    std::cout << "Processes loaded into Ready Queue. Cores are executing asynchronously.\n\n";

    // 3. Command CLI Parser Loop
    std::string command;
    while (true) {
        std::cout << "Enter command: ";
        std::getline(std::cin, command);

        // Periodically type the "screen -ls" command every 1 - 2 seconds to inspect snapshots
        if (command == "screen -ls") {
            // Renders layout using ncurses and blocks cleanly until any key is pressed
            Screen::displayScreenLS(myCPU);
        } 
        // Type "exit" to close your emulator cleanly
        else if (command == "exit") {
            std::cout << "Shutting down OS Emulator cleanly..." << std::endl;
            break;
        } 
        else if (!command.empty()) {
            std::cout << "Unknown command. Use 'screen -ls' to view processes or 'exit' to quit.\n";
        }
    }

    // On scope exit, CPU's destructor will terminate its loop and join its worker threads cleanly
    return 0;
}