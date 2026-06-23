#ifndef PROCESS_MAP_H
#define PROCESS_MAP_H

#include <map>
#include <string>
#include <iostream>
#include "Process.h"

class ProcessMap {
private:
    // Global static registry holding all active or finished processes
    inline static std::map<int, Process> registry;

public:
    // Disallow instantiation since this functions purely as a global manager
    ProcessMap() = delete;

    // Creates a new process, places it in the map, and returns its assigned PID
    static int createNewProcess(std::string processName, int minIns, int maxIns) {
        // Instantiate the process object
        Process newProc(processName, minIns, maxIns);
        int pid = newProc.getPid();

        // Move/insert it into our map using PID as the unique key
        registry[pid] = newProc;

        return pid;
    }

    // Gets a direct memory pointer to a specific process by its PID
    static Process* getProcess(int pid) {
        auto it = registry.find(pid);
        if (it != registry.end()) {
            return &(it->second); // Return the memory address of the Process object inside the map
        }
        return nullptr; // Return nullptr safely if the PID doesn't exist
    }
};

#endif