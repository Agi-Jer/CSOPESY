#ifndef BATCH_GENERATION_H
#define BATCH_GENERATION_H

#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include "RQ.h"
#include "ProcessMap.h"
#include "CPU.h" // Include our static CPU to look up clock updates

/*
Class that is responsible for batch generating processes
Runs a background thread synchronized with the static CPU hardware clock.
*/
class BatchGeneration {
private:
    inline static int minIns = 0;
    inline static int maxIns = 0;
    inline static unsigned int batchProcessFreq = 1;
    inline static std::atomic<bool> isRunning{false};
    inline static std::atomic<bool> shouldTerminate{false}; // Clear flat shutdown flag

    inline static std::thread workerThread;

    // The background execution loop
    static void runBatchLoop() {
        unsigned long long lastCheckedCycle = 0;
        unsigned int cycleAccumulator = 0;

        while (!shouldTerminate.load()) {
            unsigned long long currentCycle = CPU::getCycleCount();

            // --- ZERO-POINTER ATOMIC WAIT SYSTEM ---
            // If the CPU hasn't ticked forward yet, we wait gracefully
            if (currentCycle == lastCheckedCycle) {
                // We don't have an object reference anymore—we just sleep a tiny fraction 
                // of our CPU cycle duration (250ms / 5 = 50ms) to check the state.
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
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

                // Generate the process utilizing our constructor/factory sequence
                int spawnedPid = ProcessMap::createNewProcess(minIns, maxIns);
                RQ::addToReady(spawnedPid);
            }
        }
    }

public:
    // Purely static configuration manager class
    BatchGeneration() = delete;

    // Initializes the generator loop configuration settings
    static void initialize(int min, int max, unsigned int frequency) {
        minIns = min;
        maxIns = max;
        batchProcessFreq = frequency;
        isRunning = false;       // Start in a paused state until "scheduler-start" is run
        shouldTerminate = false;

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
        shouldTerminate = true; // Breaks the loop cleanly
        
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