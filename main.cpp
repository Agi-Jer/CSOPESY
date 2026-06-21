#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <memory> 
#include "CPU.h"
#include "RQ.h"
#include "Process.h"
#include "Screen.h"
#include <unistd.h>
#include <signal.h>

int main() {
    // Clear the terminal completely on boot so it is a total blank canvas
    std::cout << "\033[2J\033[1;1H" << std::flush;

    std::string command;
    std::unique_ptr<CPU> myCPU = nullptr;
    bool isInitialized = false;

    // --- PHASE 1: THE BLANK BOOT SCREEN ---
    // The loop keeps running silently, reading input from a blank slate
    while (!isInitialized) {
        std::getline(std::cin, command);

        if (command == "initialize") {
            // Found it! Clear the screen again and stand up the OS architecture
            std::cout << "\033[2J\033[1;1H" << std::flush;
            std::cout << "Initializing OS Emulator Components..." << std::endl;
            
            // 1. Declare 4 cores for the CPU scheduler
            int numCores = 4;
            myCPU = std::make_unique<CPU>(numCores);

            // 2. Load the test case work items (10 processes, 100 PRINT instructions each)
            std::cout << "Generating 10 test processes (100 PRINT instructions each)..." << std::endl;
            for (int i = 0; i < 10; ++i) {
                Process p("PRINT", 100);
                RQ::addToReady(p);
            }
            
            std::cout << "Processes loaded into Ready Queue. Cores are executing asynchronously.\n\n";
            isInitialized = true;
        }
        else if (command == "exit") {
            return 0;
        }
        // If they type anything else while the OS hasn't been initialized, we remain completely blank
    }

    // --- PHASE 2: NORMAL OPERATING SYSTEM CLI ---
    while (true) {
        std::cout << "Enter command: ";
        std::getline(std::cin, command);

        if (command == "screen -ls") {
            // Opens the ncurses snapshot window overlay
            Screen::displayScreenLS(*myCPU);
        } 
        else if (command == "exit") {
            std::cout << "Shutting down OS Emulator cleanly..." << std::endl;
            
            // 1. Instantly stop and join all CPU and Core threads
            myCPU.reset(); 

            std::cout << "Closing console window in 2 seconds..." << std::endl;
            
            // 2. Pause the execution thread for exactly 2 seconds
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // 3. FEDORA FIX: Send a SIGHUP (Hangup) signal to our parent shell process.
            // This tells bash to terminate immediately, bypassing the 'read' trap entirely.
            #ifdef __linux__
                kill(getppid(), SIGHUP); 
            #else
                std::exit(0);
            #endif
        }
        else if (!command.empty()) {
            std::cout << "Unknown command. Commands: 'screen -ls' or 'exit'.\n";
        }
    }

    return 0;
}