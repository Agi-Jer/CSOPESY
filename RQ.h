#ifndef RQ_H
#define RQ_H

#include <queue>
#include <vector> 
#include <shared_mutex> 
#include "Process.h" 

/*
Abstraction for Global RQ

No per-core RQ

Everything is STATIC to act as a psudo global access
*/

class RQ {
private:
    inline static std::queue<int> rQueue;      // Ready Queue (PIDs)
    inline static std::vector<int> runQueue;   // New Running Queue tracking active PIDs on Cores
    inline static std::vector<int> wQueue;     // Waiting Queue tracking blocked/sleeping PIDs
    inline static std::vector<int> fQueue;     // Finished Queue tracking completed PIDs
    inline static std::shared_mutex queueMutex; 

public:
    static void addToReady(int pid) {
        std::unique_lock<std::shared_mutex> lock(queueMutex);
        rQueue.push(pid);
    }

    static void addToFinished(int pid) {
        std::unique_lock<std::shared_mutex> lock(queueMutex);
        fQueue.push_back(pid); 
    }

    static void addToWaiting(int pid) {
        std::unique_lock<std::shared_mutex> lock(queueMutex);
        wQueue.push_back(pid);
    }

    static void addToRunning(int pid) {
        std::unique_lock<std::shared_mutex> lock(queueMutex);
        runQueue.push_back(pid);
    }

    // Outprocess is space for output PID integer
    static bool tryPopReady(int& outPid) {
        std::unique_lock<std::shared_mutex> lock(queueMutex);
        if (rQueue.empty()) return false; 
        outPid = rQueue.front();
        rQueue.pop(); 
        return true; 
    }

    // Return a copy of the running vector so the screen/scheduler can read it safely
    static std::vector<int> getRunningProcesses() {
        std::shared_lock<std::shared_mutex> lock(queueMutex);
        return runQueue;
    }

    // Return a copy of the finished vector so the screen class can read it safely
    static std::vector<int> getFinishedProcesses() {
        std::shared_lock<std::shared_mutex> lock(queueMutex);
        return fQueue;
    }

    // Return a copy of the waiting vector so the screen/scheduler can read it safely
    static std::vector<int> getWaitingProcesses() {
        std::shared_lock<std::shared_mutex> lock(queueMutex);
        return wQueue;
    }

    // Had an issue with screen getting running and finished but having cores run in between
    // Tying the two together to fix this
    static void getSnapshotPids(std::vector<int>& outReady, std::vector<int>& outRunning, std::vector<int>& outWaiting, std::vector<int>& outFinished) {
        std::shared_lock<std::shared_mutex> lock(queueMutex);
        
        // Convert the std::queue rQueue to a vector so it can be read safely by the report engine
        outReady.clear();
        std::queue<int> tempCopy = rQueue;
        while (!tempCopy.empty()) {
            outReady.push_back(tempCopy.front());
            tempCopy.pop();
        }

        outRunning = runQueue;
        outWaiting = wQueue;
        outFinished = fQueue;
    }

    // Removes a PID from the running queue (called when a process yields, sleeps, or finishes)
    static void removeFromRunning(int pid) {
        std::unique_lock<std::shared_mutex> lock(queueMutex);
        for (auto it = runQueue.begin(); it != runQueue.end(); ++it) {
            if (*it == pid) {
                runQueue.erase(it);
                break;
            }
        }
    }

    // Removes a PID from the waiting queue (called when a process wakes up)
    static void removeFromWaiting(int pid) {
        std::unique_lock<std::shared_mutex> lock(queueMutex);
        for (auto it = wQueue.begin(); it != wQueue.end(); ++it) {
            if (*it == pid) {
                wQueue.erase(it);
                break;
            }
        }
    }

    // Returns true if the Ready Queue has zero processes waiting
    static bool isEmptyReady() {
        // Lock guard ensures exclusive access across all concurrent Core threads
        std::shared_lock<std::shared_mutex> lock(queueMutex);
        return rQueue.empty();
    }
};

#endif