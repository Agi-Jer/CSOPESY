#ifndef PROCESS_MAP_H
#define PROCESS_MAP_H

#include <map>
#include <string>
#include <iostream>
#include <mutex> 
#include <memory> // Needed for std::shared_ptr
#include "Process.h"

/*
Unordered map of all processes

all processes begin and stay here

other classes simply traded in pids and reference this static class instead
*/

class ProcessMap {
private:
    // Global static registry holding all active or finished processes (Switched to shared_ptr for heap stability)
    inline static std::map<int, std::shared_ptr<Process>> registry;

    // Static mutex to synchronize access across all 128 simulated CPU cores
    inline static std::mutex mapMutex;

public:
    // Disallow instantiation since this functions purely as a global manager
    ProcessMap() = delete;

    // Creates a new process, places it in the map, and returns its assigned PID
    static int createNewProcess(std::string processName, int minIns, int maxIns) {
        // Automatically locks the mutex for the duration of this function scope
        std::lock_guard<std::mutex> lock(mapMutex);

        // Instantiate the process object cleanly on the heap
        auto newProc = std::make_shared<Process>(processName, minIns, maxIns);
        int pid = newProc->getPid();

        // Store the shared pointer securely using PID as the unique key
        registry[pid] = newProc;

        return pid;
    }

    // Overloaded method that creates a new process with an auto-generated name
    static int createNewProcess(int minIns, int maxIns) {
        // Automatically locks the mutex for the duration of this function scope
        std::lock_guard<std::mutex> lock(mapMutex);

        // Instantiate the process object using the min/max constructor overload on the heap
        auto newProc = std::make_shared<Process>(minIns, maxIns);
        int pid = newProc->getPid();

        // Store the shared pointer securely using PID as the unique key
        registry[pid] = newProc;

        return pid;
    }

    // Gets a stable memory pointer to a specific process by its PID
    static Process* getProcess(int pid) {
        // Automatically locks the mutex to safely search the map structure
        std::lock_guard<std::mutex> lock(mapMutex);

        auto it = registry.find(pid);
        if (it != registry.end()) {
            return it->second.get(); // Return the raw underlying stable address safely
        }
        return nullptr; // Return nullptr safely if the PID doesn't exist
    }

    // Gets a direct pointer to a specific process by its string Name
    static Process* getProcessByName(const std::string& processName) {
        std::lock_guard<std::mutex> lock(mapMutex);

        // Loop through the map to find the matching name property
        for (auto& [pid, proc] : registry) {
            if (proc->getName() == processName) {
                return proc.get(); // Return the exact stable memory position
            }
        }
        return nullptr; // Return nullptr if no process matches that name string
    }
};

#endif