#ifndef CORE_H
#define CORE_H

#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "Process.h"
#include "RQ.h"
#include "ProcessMap.h" // Needed to perform the initial PID lookup

/*
The Core class is an abstraction of a cpu core

It handles all processing and decides when to take or add to the RQ (e.g. Round Robin)
*/

// Strongly-typed enumerator for optimized cycle evaluations
enum class SchedulerType {
    FCFS,
    ROUND_ROBIN
};

class Core {
private:
    int coreId; //Core Id starts from 0 counting up
    SchedulerType type; // Fast enum type tracking instead of strings
    
    // Tracking fields matching the new ProcessMap architecture
    int currentPid;                          // Holds the ID of the assigned process (-1 if idle)
    Process* currentProcess;                 // Performance Optimization: Caches pointer locally
    
    bool holdsProcess; //Bool if core is currently handling a process
    unsigned int quantumCycle; //Quantum cycle for Round Robin
    unsigned int trackQCycle; //keeps track of the current quantum cycle for the RR logic

    std::thread workerThread; //Worker thread for FCFS or RR logic
    std::atomic<bool>& cpuRunningRef; //Checks if CPU is still running for when exit function is called
    
   //Counters for keeping track and updating the current clock cycle
   //Makes sures each thread only runs once per cycle
    const std::atomic<unsigned long long>& globalCycleRef; 
    unsigned long long currCycle; 

    // First-Come, First-Served scheduling implementation
    void fcfs() {
        // FCFS Scheduler Action: Grab a process if idle
        if (!holdsProcess) {
            int poppedPid = -1; // Temporary container for tryPopReady
            if (RQ::tryPopReady(poppedPid)) {
                currentPid = poppedPid;
                holdsProcess = true;
                
                // Fetch and cache the direct pointer once during acquisition
                currentProcess = ProcessMap::getProcess(currentPid);
                if (currentProcess != nullptr) {
                    currentProcess->setRunning();
                    currentProcess->setAssignedCore(coreId);
                }
                
                // Mirror onto the global running registry tracking system
                RQ::addToRunning(currentPid);
            }
        }

        // Execution of Instruction using the cached pointer directly
        if (holdsProcess && currentProcess != nullptr) {
            // runCycle handles execution routing, internal clocks, and auto-finishing flags
            currentProcess->runCycle();

            // Step A: Check if the instruction placed the process in a WAITING state (e.g., SLEEP instruction)
            if (currentProcess->isSleeping()) {
                RQ::removeFromRunning(currentPid);
                RQ::addToWaiting(currentPid);
                
                // Reset fields completely to signal core availability
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false;
            }
            // Step B: Check if the process naturally exhausted its program lines
            else if (currentProcess->isFinished()) { 
                RQ::removeFromRunning(currentPid);
                RQ::addToFinished(currentPid);
                
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false; 
            }
        } else if (holdsProcess) {
            // Fallback safety case: If core thinks it has work but pointer is null, reset flags
            currentPid = -1;
            currentProcess = nullptr;
            holdsProcess = false;
        }
    }

    // Round Robin scheduling implementation
    void rr() {
        // RR Scheduler Action: Grab a process if idle
        if (!holdsProcess) {
            int poppedPid = -1; // Temporary container for tryPopReady
            if (RQ::tryPopReady(poppedPid)) {
                currentPid = poppedPid;
                holdsProcess = true;
                
                // Fetch and cache the direct pointer once during acquisition
                currentProcess = ProcessMap::getProcess(currentPid);
                if (currentProcess != nullptr) {
                    currentProcess->setRunning();
                    currentProcess->setAssignedCore(coreId);
                }
                
                // Mirror onto the global running registry tracking system
                RQ::addToRunning(currentPid);
                
                // Reset the quantum tracker for this freshly scheduled process
                trackQCycle = 0; 
            }
        }

        // Execution of Instruction using the cached pointer directly
        if (holdsProcess && currentProcess != nullptr) {
            // runCycle handles execution routing, internal clocks, and auto-finishing flags
            currentProcess->runCycle();
            
            // Advance our perpetual quantum clock tracker by 1 cycle
            trackQCycle++;

            // Scenario A: The process hit a SLEEP instruction and went into a WAITING state
            if (currentProcess->isSleeping()) {
                RQ::removeFromRunning(currentPid);
                RQ::addToWaiting(currentPid);
                
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false;
                trackQCycle = 0;
            }
            // Scenario B: The process naturally finished its workload during this cycle
            else if (currentProcess->isFinished()) { 
                RQ::removeFromRunning(currentPid);
                RQ::addToFinished(currentPid);
                
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false; 
                trackQCycle = 0; 
            }
            // Scenario C: Process is not finished or waiting, but the time slice has expired!
            else if (trackQCycle >= quantumCycle) {
                currentProcess->setReady(); // Set state back to READY before sending back to queue

                RQ::removeFromRunning(currentPid);                     
                RQ::addToReady(currentPid);           
                
                currentPid = -1;                     
                currentProcess = nullptr;
                holdsProcess = false;
                trackQCycle = 0; 
            }
        } else if (holdsProcess) {
            // Fallback safety case: If core thinks it has work but pointer is null, reset flags
            currentPid = -1;
            currentProcess = nullptr;
            holdsProcess = false;
            trackQCycle = 0;
        }
    }

    //Main Loop of the TCore Thread
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

            // Dynamically route workloads via rapid assembly-friendly integer checks
            switch (type) {
                case SchedulerType::FCFS:
                    fcfs();
                    break;
                case SchedulerType::ROUND_ROBIN:
                    rr();
                    break;
            }
        }
    }

public:
    // Make a thread on construction
    Core(int id, std::atomic<bool>& cpuRunFlag, const std::atomic<unsigned long long>& cpuCycle, std::string coreType = "fcfs", unsigned int quantumCycle = 0) 
        : coreId(id), currentPid(-1), currentProcess(nullptr), holdsProcess(false),
          quantumCycle(quantumCycle), trackQCycle(0),
          cpuRunningRef(cpuRunFlag), globalCycleRef(cpuCycle), currCycle(0) {
        
        // Validate and convert the incoming setup string into our optimal enum value
        if (coreType == "rr" || coreType == "RR") {
            this->type = SchedulerType::ROUND_ROBIN;
        } else {
            this->type = SchedulerType::FCFS; // Fallback / Default choice
        }
        
        workerThread = std::thread(&Core::runCoreLoop, this);
    }

    //Deconstructor to make sure no threads are lost
    ~Core() {
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    //Getters
    int getCoreId() const { return coreId; }
    
    // Check if the core is currently busy (Now safely thread-safe without explicit lock)
    bool isHoldingProcess() const { 
        return holdsProcess && (currentProcess != nullptr); 
    }
};

#endif