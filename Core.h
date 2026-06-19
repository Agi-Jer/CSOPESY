#ifndef CORE_H
#define CORE_H

#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex> // Added for thread safety when CPU reads the process
#include "Process.h"
#include "RQ.h"

class Core {
private:
    int coreId;
    std::string type;
    Process currentProcess;
    bool holdsProcess;

    std::thread workerThread;
    std::atomic<bool>& cpuRunningRef; 
    
    // Updated to reference an atomic unsigned long long
    const std::atomic<unsigned long long>& globalCycleRef; 
    unsigned long long currCycle; 

    // Added to protect currentProcess and holdsProcess from being modified 
    // by this thread while the CPU thread is trying to read them for the UI
    mutable std::mutex coreMutex; 

    void runCoreLoop() {
        while (cpuRunningRef) {
            
            // Modern Atomic Wait Logic:
            // If the core has caught up to the CPU cycle, it blocks cleanly (0% CPU)
            // until the CPU increments cycleCount and triggers a notification.
            while (currCycle == globalCycleRef.load() && cpuRunningRef) {
                globalCycleRef.wait(currCycle); 
            }

            // Quick sanity check if waking up because the entire simulation is shutting down
            if (!cpuRunningRef) {
                break;
            }

            // Catch up to the freshly updated CPU cycle
            currCycle = globalCycleRef.load();

            // Scope block to lock variables during modification
            {
                std::lock_guard<std::mutex> lock(coreMutex);

                // FCFS Scheduler Action: Grab a process if idle
                if (!holdsProcess) {
                    if (RQ::tryPopReady(currentProcess)) {
                        holdsProcess = true;
                        currentProcess.setAssignedCore(coreId);
                        //std::cout << "Core " << coreId << " grabbed Process " << currentProcess.getPid() << " on Cycle " << currCycle << std::endl;
                    }
                }

                // Execution of Instruction
                if (holdsProcess) {
                    currentProcess.executeInstruction();

                    if (currentProcess.isFinished()) { 
                        RQ::addToFinished(currentProcess);
                        holdsProcess = false; 
                        //std::cout << "Core " << coreId << " finished Process " << currentProcess.getPid() << " on Cycle " << currCycle << std::endl;
                    }
                }
            } // Mutex automatically unlocks here
        }
    }

public:
    // Make a thread on construction
    Core(int id, std::atomic<bool>& cpuRunFlag, const std::atomic<unsigned long long>& cpuCycle, std::string coreType = "FCFS") 
        : coreId(id), type(coreType), holdsProcess(false), currentProcess("dummy", -1),
          cpuRunningRef(cpuRunFlag), globalCycleRef(cpuCycle), currCycle(0) {
        
        workerThread = std::thread(&Core::runCoreLoop, this);
    }

    ~Core() {
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    //getters
    int getCoreId() const { return coreId; }
    // Check if the core is currently busy
    bool isHoldingProcess() const { 
        std::lock_guard<std::mutex> lock(coreMutex);
        return holdsProcess; 
    }
    // Read the process currently assigned to this core
    const Process& getCurrentProcess() const { 
        std::lock_guard<std::mutex> lock(coreMutex);
        return currentProcess;
    }

    // Thread-safe method for the CPU to extract an active process copy safely
    bool getActiveProcessCopy(Process& outProcess) const {
        std::lock_guard<std::mutex> lock(coreMutex);
        if (holdsProcess) {
            outProcess = currentProcess;
            return true;
        }
        return false;
    }
};

#endif