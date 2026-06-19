#ifndef CPU_H
#define CPU_H

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>
#include <memory> // Required for std::unique_ptr and std::make_unique
#include "Core.h" 

class CPU {
private:
    int numCores;
    // Updated to use unique_ptr so non-copyable/non-movable Cores can reside safely
    std::vector<std::unique_ptr<Core>> cores;
    
    std::thread cpuThread;
    std::atomic<bool> isRunning;
    
    // Updated to an atomic type so cores can sleep on it cleanly
    std::atomic<unsigned long long> cycleCount; 

    // background cpu cycle thread
    void runCPULoop() {
        // ~30 cycles per second (approx 33.33ms per cycle)
        auto cycleDuration = std::chrono::microseconds(33333); // ~30 Hz

        while (isRunning) {
            auto startTime = std::chrono::steady_clock::now();

            // --- CPU CYCLE WORK HAPPENS HERE ---
            cycleCount++;
            
            // Instantly unblocks and wakes up any Core threads waiting for this cycle
            cycleCount.notify_all(); 
            // ------------------------------------

            // Calculate how long the work took and sleep for the remaining time
            auto endTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            if (elapsedTime < cycleDuration) {
                std::this_thread::sleep_for(cycleDuration - elapsedTime);
            }
        }
    }

public:
    // Constructor
    CPU(int coresCount) : numCores(coresCount), cycleCount(0), isRunning(false) {
        isRunning = true;
        
        // Pass references to the operational flags and our updated atomic clock count
        // Instantiate the blank Core classes into our vector via make_unique
        for (int i = 0; i < numCores; ++i) {
            cores.push_back(std::make_unique<Core>(i, isRunning, cycleCount, "FCFS")); 
        }
            
        cpuThread = std::thread(&CPU::runCPULoop, this);
    }

    // Destructor: Essential for cleaning up the thread safely when the CPU shuts down
    ~CPU() {
        isRunning = false; // Signals the while loop to terminate safely
        
        // Unblock any threads that are currently trapped in the globalCycleRef.wait() loop
        cycleCount.notify_all(); 
        
        if (cpuThread.joinable()) {
            cpuThread.join(); // Waits for the thread to completely finish up before destroying the CPU object
        }
    }

    // Getter to check the current cycle count statically or dynamically
    unsigned long long getCycleCount() const {
        return cycleCount.load();
    }

    // Aggregates and returns a vector containing copies of only the active running processes across all cores
    std::vector<Process> getRunningProcesses() const {
        std::vector<Process> runningProcesses;

        // Loop through each core pointer to pull active process information thread-safely
        for (const auto& core : cores) {
            Process p("dummy", -1);
            if (core->getActiveProcessCopy(p)) { // Changed '.' to '->' since elements are pointers
                runningProcesses.push_back(p);
            }
        }

        return runningProcesses;
    }
};

#endif