#ifndef BATCH_GENERATION_H
#define BATCH_GENERATION_H

#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include "RQ.h"
#include "ProcessMap.h"

/*
Class that is responsible for batch generating processes

Runs a background thread 
*/

class BatchGeneration {
private:
    inline static int minIns = 0;
    inline static int maxIns = 0;
    inline static unsigned int batchProcessFreq = 1;
    inline static std::atomic<bool> isRunning{false};

    inline static std::thread workerThread;
    
    // Pointer to hold the shared CPU master clock reference dynamically
    inline static const std::atomic<unsigned long long>* globalCycleRef = nullptr;

    // The background execution loop
    static void runBatchLoop() {
        unsigned long long lastCheckedCycle = 0;
        unsigned int cycleAccumulator = 0;

        while (true) {
            // Check if the thread should shut down completely
            if (globalCycleRef == nullptr) break;

            // --- ATOMIC WAIT SYSTEM ---
            // Wakes up on EVERY clock pulse or when notified
            unsigned long long currentCycle = globalCycleRef->load();
            while (currentCycle == lastCheckedCycle) {
                globalCycleRef->wait(lastCheckedCycle);
                currentCycle = globalCycleRef->load();
            }
            lastCheckedCycle = currentCycle;

            // If the generation module is manually paused/stopped, skip logic
            if (!isRunning.load()) {
                cycleAccumulator = 0; // Reset progress toward the next batch frequency target
                continue;
            }

            // A valid CPU cycle passed while running!
            cycleAccumulator++;

            // Check if we hit the cycle frequency threshold to spawn a process
            if (cycleAccumulator >= batchProcessFreq) {
                cycleAccumulator = 0; // Reset threshold tracker

                // Generate the process utilizing our overloaded constructor/factory sequence
                int spawnedPid = ProcessMap::createNewProcess(minIns, maxIns);
                RQ::addToReady(spawnedPid);
            }
        }
    }

public:
    // Purely static configuration manager class
    BatchGeneration() = delete;

    // Initializes the generator loop and binds it to the hardware CPU master clock
    static void initialize(const std::atomic<unsigned long long>& cpuMasterClock, int min, int max, unsigned int frequency) {
        minIns = min;
        maxIns = max;
        batchProcessFreq = frequency;
        globalCycleRef = &cpuMasterClock;
        isRunning = false; // Start in a paused state until explicitly kicked off

        // Launch the single perpetual manager thread
        workerThread = std::thread(&BatchGeneration::runBatchLoop);
    }

    // Command triggers to control generation state from UI inputs
    static void start() {
        isRunning = true;
    }

    static void stop() {
        isRunning = false;
    }

    // Dynamic property updates
    static void updateConfiguration(int min, int max, unsigned int frequency) {
        minIns = min;
        maxIns = max;
        batchProcessFreq = frequency;
    }

    // Safe teardown routine during execution exit
    static void shutdown() {
        globalCycleRef = nullptr; // Breaks the while loop
        
        // Safety wake call just in case it's caught waiting inside the cycle lock loop
        if (globalCycleRef != nullptr) {
            // Note: In main.cpp, ensure you notify after removing references
        }
        
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    // Getters for UI telemetry screens
    static bool getIsRunning() { return isRunning.load(); }
    static int getMinIns() { return minIns; }
    static int getMaxIns() { return maxIns; }
    static unsigned int getBatchFreq() { return batchProcessFreq; }
};

#endif