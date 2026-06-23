#ifndef PROCESS_MAP_H
#define PROCESS_MAP_H

#include <map>
#include <string>
#include <iostream>
#include <mutex> // Included for multi-threaded safety
#include "Process.h"

class ProcessMap {
private:
    // Global static registry holding all active or finished processes
    inline static std::map<int, Process> registry;

    // Static mutex to synchronize access across all 128 simulated CPU cores
    inline static std::mutex mapMutex;

public:
    // Disallow instantiation since this functions purely as a global manager
    ProcessMap() = delete;

    // Creates a new process, places it in the map, and returns its assigned PID
    static int createNewProcess(std::string processName, int minIns, int maxIns) {
        // Automatically locks the mutex for the duration of this function scope
        std::lock_guard<std::mutex> lock(mapMutex);

        // Instantiate the process object
        Process newProc(processName, minIns, maxIns);
        int pid = newProc.getPid();

        // Move/insert it into our map using PID as the unique key
        registry[pid] = newProc;

        return pid;
    }

    // Overloaded method that creates a new process with an auto-generated name
    static int createNewProcess(int minIns, int maxIns) {
        // Automatically locks the mutex for the duration of this function scope
        std::lock_guard<std::mutex> lock(mapMutex);

        // Instantiate the process object using the min/max constructor overload
        Process newProc(minIns, maxIns);
        int pid = newProc.getPid();

        // Move/insert it into our map using PID as the unique key
        registry[pid] = newProc;

        return pid;
    }

    // Gets a direct memory pointer to a specific process by its PID
    static Process* getProcess(int pid) {
        // Automatically locks the mutex to safely search the map structure
        std::lock_guard<std::mutex> lock(mapMutex);

        auto it = registry.find(pid);
        if (it != registry.end()) {
            return &(it->second); // Return the memory address of the Process object inside the map
        }
        return nullptr; // Return nullptr safely if the PID doesn't exist
    }

    // Gets a direct pointer to a specific process by its string Name
    static Process* getProcessByName(const std::string& processName) {
        std::lock_guard<std::mutex> lock(mapMutex);

        // Loop through the map to find the matching name property
        for (auto& [pid, proc] : registry) {
            if (proc.getName() == processName) {
                return &proc; // Return the exact memory position
            }
        }
        return nullptr; // Return nullptr if no process matches that name string
    }
};

#endif