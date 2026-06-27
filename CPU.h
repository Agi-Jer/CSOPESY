#ifndef CPU_H
#define CPU_H

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>
#include <memory>
#include "Core.h" 
#include "ProcessMap.h"
#include "RQ.h"

/*
Represents a static abstraction of the CPU itself.
Handles primarily the Cores and the background master clock cycle thread.
*/
class CPU {
private:
    // Pure static state variables (Inline initialized for C++20)
    inline static int numCores = 0;
    inline static std::vector<std::unique_ptr<Core>> cores;
    inline static std::thread cpuThread;
    inline static std::atomic<bool> isRunning{false};
    inline static std::atomic<unsigned long long> cycleCount{0};

    // Iterates through all currently sleeping processes and decrements their timers
    static void manageWaitingQueue() {
        std::vector<int> waitingPids = RQ::getWaitingProcesses();

        for (int pid : waitingPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                proc->decrementSleep();

                if (!proc->isSleeping()) {
                    RQ::removeFromWaiting(pid);
                    RQ::addToReady(pid);
                }
            } else {
                RQ::removeFromWaiting(pid);
            }
        }
    }

    // Background CPU master clock cycle loop thread
    static void runCPULoop() {
        //auto cycleDuration = std::chrono::milliseconds(250); // ~4Hz
        auto cycleDuration = std::chrono::milliseconds(100); // 10Hz
        //auto cycleDuration = std::chrono::microseconds(33333); // 30Hz
        //auto cycleDuration = std::chrono::microseconds(16667); // 60Hz

        while (isRunning) {
            auto startTime = std::chrono::steady_clock::now();

            // --- CPU MASTER CLOCK TICK ---
            cycleCount++;
            
            // Ticks down the timers for any processes currently blocked/sleeping
            manageWaitingQueue();
            
            // Instantly unblocks and wakes up any Core threads waiting for this cycle
            cycleCount.notify_all(); 
            // ------------------------------------

            auto endTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            if (elapsedTime < cycleDuration) {
                std::this_thread::sleep_for(cycleDuration - elapsedTime);
            }
        }
    }

public:
    // Delete constructor since it's a pure static UI/Engine module
    CPU() = delete;

    // Bootstraps the CPU engine (Called once during initialize)
    static void initialize(int coresCount, const std::string& schedulerAlgo, unsigned int quantumCycles = 1) {
        if (isRunning) return; // Prevent duplicate instantiation collisions

        numCores = coresCount;
        cycleCount = 0;
        isRunning = true;
        
        // Populate core clusters passing the inner atomic reference down securely
        for (int i = 0; i < numCores; ++i) {
            cores.push_back(std::make_unique<Core>(i, isRunning, cycleCount, schedulerAlgo, quantumCycles)); 
        }
            
        // Spawn background master hardware clock thread
        cpuThread = std::thread(&CPU::runCPULoop);
    }

    // Shuts down execution blocks and joins threads safely
    static void shutdown() {
        if (!isRunning) return;

        isRunning = false; 
        cycleCount.notify_all(); // Wake up any threads waiting for a clock tick
        
        if (cpuThread.joinable()) {
            cpuThread.join(); 
        }
        cores.clear(); // Free allocated Core memory handles safely
    }

    // Globally accessible atomic snapshot trackers
    static unsigned long long getCycleCount() {
        return cycleCount.load();
    }

    static int getNumCores() {
        return numCores;
    }
};

#endif