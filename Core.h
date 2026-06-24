#ifndef CORE_H
#define CORE_H

#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "Process.h"
#include "RQ.h"
#include "ProcessMap.h"

// Strongly-typed enumerator for optimized cycle evaluations
enum class SchedulerType {
    FCFS,
    ROUND_ROBIN
};

class Core {
private:
    int coreId;                         // Core Id starts from 0 counting up
    SchedulerType type;                 // Fast enum type tracking instead of strings
    
    int currentPid;                     // Holds the ID of the assigned process (-1 if idle)
    Process* currentProcess;            // Performance Optimization: Caches pointer locally
    
    bool holdsProcess;                  // Bool if core is currently handling a process
    unsigned int quantumCycle;          // Quantum cycle for Round Robin
    unsigned int trackQCycle;           // Keeps track of the current quantum cycle for the RR logic

    std::thread workerThread;           // Worker thread for FCFS or RR logic
    std::atomic<bool>& cpuRunningRef;   // Checks if CPU is still running for when exit function is called
    
    // Reference to the central CPU clock to break circular dependency
    const std::atomic<unsigned long long>& globalCycleRef; 
    unsigned long long currCycle; 

    // First-Come, First-Served scheduling implementation
    void fcfs() {
        if (!holdsProcess) {
            int poppedPid = -1;
            if (RQ::tryPopReady(poppedPid)) {
                currentPid = poppedPid;
                holdsProcess = true;
                
                currentProcess = ProcessMap::getProcess(currentPid);
                if (currentProcess != nullptr) {
                    currentProcess->setRunning();
                    currentProcess->setAssignedCore(coreId);
                }
                
                RQ::addToRunning(currentPid);
            }
        }

        if (holdsProcess && currentProcess != nullptr) {
            currentProcess->runCycle();

            if (currentProcess->isSleeping()) {
                RQ::removeFromRunning(currentPid);
                RQ::addToWaiting(currentPid);
                
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false;
            }
            else if (currentProcess->isFinished()) { 
                RQ::removeFromRunning(currentPid);
                RQ::addToFinished(currentPid);
                
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false; 
            }
        } else if (holdsProcess) {
            currentPid = -1;
            currentProcess = nullptr;
            holdsProcess = false;
        }
    }

    // Round Robin scheduling implementation
    void rr() {
        if (!holdsProcess) {
            int poppedPid = -1;
            if (RQ::tryPopReady(poppedPid)) {
                currentPid = poppedPid;
                holdsProcess = true;
                
                currentProcess = ProcessMap::getProcess(currentPid);
                if (currentProcess != nullptr) {
                    currentProcess->setRunning();
                    currentProcess->setAssignedCore(coreId);
                }
                
                RQ::addToRunning(currentPid);
                trackQCycle = 0; 
            }
        }

        if (holdsProcess && currentProcess != nullptr) {
            currentProcess->runCycle();
            trackQCycle++;

            if (currentProcess->isSleeping()) {
                RQ::removeFromRunning(currentPid);
                RQ::addToWaiting(currentPid);
                
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false;
                trackQCycle = 0;
            }
            else if (currentProcess->isFinished()) { 
                RQ::removeFromRunning(currentPid);
                RQ::addToFinished(currentPid);
                
                currentPid = -1;
                currentProcess = nullptr;
                holdsProcess = false; 
                trackQCycle = 0; 
            }
            else if (trackQCycle >= quantumCycle) {
                currentProcess->setReady();

                RQ::removeFromRunning(currentPid);                     
                RQ::addToReady(currentPid);           
                
                currentPid = -1;                     
                currentProcess = nullptr;
                holdsProcess = false;
                trackQCycle = 0; 
            }
        } else if (holdsProcess) {
            currentPid = -1;
            currentProcess = nullptr;
            holdsProcess = false;
            trackQCycle = 0;
        }
    }

    // Main Loop of the Core Thread
    void runCoreLoop() {
        while (cpuRunningRef) {
            unsigned long long actualSystemCycle = globalCycleRef.load();
            
            // --- DECOUPLED CLOCK LOOP ---
            if (currCycle == actualSystemCycle) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (!cpuRunningRef) {
                break;
            }

            // Sync local core timeline tracker up to master clock standard
            currCycle = actualSystemCycle;

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
    // Core explicitly tracks the global clock via direct reference passing
    Core(int id, std::atomic<bool>& cpuRunFlag, const std::atomic<unsigned long long>& cpuCycle, std::string coreType = "fcfs", unsigned int quantumCycle = 1) 
        : coreId(id), currentPid(-1), currentProcess(nullptr), holdsProcess(false),
          quantumCycle(quantumCycle), trackQCycle(0),
          cpuRunningRef(cpuRunFlag), globalCycleRef(cpuCycle), currCycle(0) {
        
        if (coreType == "rr" || coreType == "RR") {
            this->type = SchedulerType::ROUND_ROBIN;
        } else {
            this->type = SchedulerType::FCFS;
        }
        
        workerThread = std::thread(&Core::runCoreLoop, this);
    }

    ~Core() {
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    int getCoreId() const { return coreId; }
    
    bool isHoldingProcess() const { 
        return holdsProcess && (currentProcess != nullptr); 
    }
};

#endif