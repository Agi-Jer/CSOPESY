#ifndef CORE_H
#define CORE_H

#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex> // Added for thread safety when CPU reads the process
#include <memory>
#include "Process.h"
#include "RQ.h"

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
    std::unique_ptr<Process> currentProcess; //Process its currently handling in core
    bool holdsProcess; //Bool if core is currently handling a process
    unsigned int quantumCycle; //Quantum cycle for Round Robin
    unsigned int trackQCycle; //keeps track of the current quantum cycle for the RR logic

    std::thread workerThread; //Worker thread for FCFS or RR logic
    std::atomic<bool>& cpuRunningRef; //Checks if CPU is still running for when exit function is called
    
   //Counters for keeping track and updating the current clock cycle
   //Makes sures each thread only runs once per cycle
    const std::atomic<unsigned long long>& globalCycleRef; 
    unsigned long long currCycle; 

    // Added to protect currentProcess and holdsProcess from being modified 
    // by this thread while the CPU thread is trying to read them for the UI
    mutable std::mutex coreMutex; 


    // First-Come, First-Served scheduling implementation
    void fcfs() {
        // FCFS Scheduler Action: Grab a process if idle
        if (!holdsProcess) {
            Process poppedProcess; // Temporary container for tryPopReady
            if (RQ::tryPopReady(poppedProcess)) {
                
                // Allocate the raw process onto the heap safely!
                currentProcess = std::make_unique<Process>(poppedProcess);
                
                holdsProcess = true;
                currentProcess->setAssignedCore(coreId); // Use -> instead of .
            }
        }

        // Execution of Instruction
        if (holdsProcess && currentProcess != nullptr) {
            currentProcess->executeInstruction();

            if (currentProcess->isFinished()) { 
                RQ::addToFinished(*currentProcess);
                currentProcess.reset();
                holdsProcess = false; 
                //std::cout << "Core " << coreId << " finished Process " << currentProcess.getPid() << " on Cycle " << currCycle << std::endl;
            }
        }
    }

    // Round Robin scheduling implementation
    void rr() {
        // RR Scheduler Action: Grab a process if idle
        if (!holdsProcess) {
            Process poppedProcess; // Temporary container for tryPopReady
            if (RQ::tryPopReady(poppedProcess)) {
                
                // Allocate the raw process onto the heap safely!
                currentProcess = std::make_unique<Process>(poppedProcess);
                
                holdsProcess = true;
                currentProcess->setAssignedCore(coreId); // Use -> instead of .
                
                // Reset the quantum tracker for this freshly scheduled process
                trackQCycle = 0; 
            }
        }

        // Execution of Instruction
        if (holdsProcess && currentProcess != nullptr) {
            currentProcess->executeInstruction();
            
            // Advance our perpetual quantum clock tracker by 1 cycle
            trackQCycle++;

            // Scenario A: The process naturally finished its workload during this cycle
            if (currentProcess->isFinished()) { 
                RQ::addToFinished(*currentProcess);
                currentProcess.reset();
                holdsProcess = false; 
                trackQCycle = 0; // Reset tracker for the next process allocation
                //std::cout << "Core " << coreId << " finished Process " << currentProcess->getPid() << " on Cycle " << currCycle << std::endl;
            }
            // Scenario B: Process is not finished, but the time slice has expired!
            else if (trackQCycle >= quantumCycle) {
                // Copying data to prevent other Core threads from pulling from RQ 
                // at the same time this thread removes it from memory
                Process preemptedProcess = *currentProcess; 
                currentProcess.reset();                     
                holdsProcess = false;
                trackQCycle = 0; 

                RQ::addToReady(preemptedProcess);           
            }
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

            // Scope block to lock variables during modification
            {
                std::lock_guard<std::mutex> lock(coreMutex);

                // Dynamically route workloads via rapid assembly-friendly integer checks
                switch (type) {
                    case SchedulerType::FCFS:
                        fcfs();
                        break;
                    case SchedulerType::ROUND_ROBIN:
                        rr();
                        break;
                }
                
            } // Mutex automatically unlocks here
        }
    }

public:
    // Make a thread on construction
    Core(int id, std::atomic<bool>& cpuRunFlag, const std::atomic<unsigned long long>& cpuCycle, std::string coreType = "fcfs", unsigned int quantumCycle = 0) 
        : coreId(id), currentProcess(nullptr), holdsProcess(false),
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
    // Check if the core is currently busy
    bool isHoldingProcess() const { 
        std::lock_guard<std::mutex> lock(coreMutex);
        return holdsProcess && (currentProcess != nullptr); 
    }
    // Read the process currently assigned to this core
    const Process& getCurrentProcess() const { 
        std::lock_guard<std::mutex> lock(coreMutex);
        return *currentProcess;
    }

    // Thread-safe method for the CPU to extract an active process copy safely
    // This is meant for reading the process (e.g. Screen -ls)
    bool getActiveProcessCopy(Process& outProcess) const {
        std::lock_guard<std::mutex> lock(coreMutex);
        
        // CRITICAL FIX: Only copy if the pointer is NOT null!
        if (holdsProcess && currentProcess != nullptr) {
            outProcess = *currentProcess; // Dereference the pointer safely
            return true;
        }
        return false;
    }
};

#endif