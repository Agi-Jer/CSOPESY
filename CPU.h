#ifndef CPU_H
#define CPU_H

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>
#include <memory> // Required for std::unique_ptr and std::make_unique
#include "Core.h" 

/*
Represents an abstraction of the CPU itself

handles primarily the Cores and the cpu cycle logic

Wrapper for Cores
*/


class CPU {
private:
    int numCores; //Num of cores in CPU
    // Updated to use unique_ptr so non-copyable/non-movable Cores can reside safely
    std::vector<std::unique_ptr<Core>> cores; //Vector of Core class
    
    std::thread cpuThread; //Clock cycle Thread
    std::atomic<bool> isRunning; //Bool to check if CPU is still running
    
    // Updated to an atomic type so cores can sleep on it cleanly
    std::atomic<unsigned long long> cycleCount; //CPU cycle tracker

    // Iterates through all currently sleeping processes and decrements their timers
    void manageWaitingQueue() {
        // Grab a snapshot copy of the waiting PIDs to iterate safely
        std::vector<int> waitingPids = RQ::getWaitingProcesses();

        for (int pid : waitingPids) {
            //Get process from map
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                // Decrement the local sleep counter inside the process
                proc->decrementSleep();

                // If the process woke up and changed its state back to READY
                if (!proc->isSleeping()) {
                    // Update global queues: remove from waiting list, push to ready loop
                    RQ::removeFromWaiting(pid);
                    RQ::addToReady(pid);
                }
            } else {
                // Safety cleanup: If a process somehow vanished from memory, clear it out
                RQ::removeFromWaiting(pid);
            }
        }
    }

    // background cpu cycle thread
    void runCPULoop() {
        // ~30 cycles per second (approx 33.33ms per cycle)
        //auto cycleDuration = std::chrono::microseconds(33333); // ~30 Hz
        //slower for this example
        auto cycleDuration = std::chrono::milliseconds(250);//~4Hz

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
};

#endif