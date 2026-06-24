#ifndef SCREEN_H
#define SCREEN_H

#include <iostream>
#include <string>
#include <vector>
#include "ProcessMap.h"
#include "Process.h"
#include "RQ.h"

class Screen {
private:
    // Helper to print the localized process information block
    static void renderProcessSMI(Process* proc) {
        if (proc == nullptr) return;

        std::cout << "\nProcess name: " << proc->getName() << "\n";
        std::cout << "ID: " << proc->getPid() << "\n";
        std::cout << "Logs:\n";

        // Fetch and print out chronological screen print entries
        const auto& logs = proc->getScreenLogBuffer();
        for (const auto& logLine : logs) {
            std::cout << logLine << "\n";
        }

        // Spec & Mockup Match: If finished, print "Finished!" and skip line numbers entirely
        if (proc->isFinished()) {
            std::cout << "\nFinished!\n";
            return;
        }

        // Only prints if the process is actively running or waiting
        std::cout << "\nCurrent instruction line: " << proc->getCurrentInstruction() << "\n";
        std::cout << "Lines of code: " << proc->getTotalInstructions() << "\n";
    }

public:
    Screen() = delete; // Pure static UI component

    // OVERLOAD 1: Standard Re-attach (Equivalent to screen -r)
    static bool enterProcessScreen(const std::string& processName) {
        Process* proc = ProcessMap::getProcessByName(processName);

        // Guard: Process doesn't exist at all in the registry
        if (proc == nullptr) {
            std::cout << "Process " << processName << " not found.\n";
            return false;
        }

        // Guard: Process exists but has already finished (Strict spec match)
        if (proc->isFinished()) {
            std::cout << "Process " << processName << " not found.\n";
            return false;
        }

        //My ass is on Linux
        std::system("clear");

        renderProcessSMI(proc);
        std::string subInput; //For input loop

        // New loop bound completely inside new isolated screen space
        while (true) {
            std::cout << "\nroot:\\> "; //I should probably switch from the example
            std::getline(std::cin, subInput);

            if (subInput == "process-smi") {
                renderProcessSMI(proc);
            } 
            else if (subInput == "exit") {
                break;
            } 
            else if (!subInput.empty()) {
                std::cout << "Command not recognized within process screen context.\n";
            }
        }
        return true;
    }

    // OVERLOAD 2: Spawn & Attach (Equivalent to screen -s)
    static bool enterProcessScreen(const std::string& processName, int minIns, int maxIns) {
        // Check if a process with this name already exists to prevent duplication collisions
        if (ProcessMap::getProcessByName(processName) != nullptr) {
            std::cout << "Error: A process named '" << processName << "' already exists.\n";
            return false;
        }

        // Call the function from ProcessMap to instantiate and register it
        int newPid = ProcessMap::createNewProcess(processName, minIns, maxIns);

        // Put the new process into the global Ready Queue so the CPU starts crunching it
        RQ::addToReady(newPid);

        // Fall back directly to the original function to clear the screen and start the loop
        return enterProcessScreen(processName);
    }
};

#endif