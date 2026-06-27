#ifndef REPORT_H
#define REPORT_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include "RQ.h"
#include "ProcessMap.h"
#include "Process.h"
#include "CPU.h"

class Report {
private:
    // Internal helper to build the unified ASCII snapshot string block
    static std::string buildSnapshotString() {
        std::vector<int> runningPids;
        std::vector<int> finishedPids;

        // Fetch both snapshot tracks simultaneously under a single unified lock pass
        // (Note: To keep this original method compiling with the new RQ signature, we pass dummy vectors)
        std::vector<int> dummyReady, dummyWaiting;
        RQ::getSnapshotPids(dummyReady, runningPids, dummyWaiting, finishedPids);

        // Dynamically grab the absolute source of truth directly from the CPU
        int totalCores = CPU::getNumCores();

        int coresUsed = static_cast<int>(runningPids.size());
        if (coresUsed > totalCores) coresUsed = totalCores; // Upper bound guard
        int coresAvailable = totalCores - coresUsed;

        // Calculate utilization: (Cores Used / Total Cores) * 100
        int cpuUtilization = (totalCores > 0) ? static_cast<int>((static_cast<double>(coresUsed) / totalCores) * 100) : 0;

        std::string output = "";
        output += "CPU utilization: " + std::to_string(cpuUtilization) + "%\n";
        output += "Cores used: " + std::to_string(coresUsed) + "\n";
        output += "Cores available: " + std::to_string(coresAvailable) + "\n";
        output += "-------------------------------------------------------\n\n";

        // 1. Running Processes Segment
        output += "Running processes:\n";
        for (int pid : runningPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                int coreId = proc->getAssignedCore();
                std::string coreStr = (coreId >= 0 && coreId < totalCores) ? std::to_string(coreId) : "N/A";
                coreStr = "Core: " + coreStr;

                std::string insStr = std::to_string(proc->getCurrentInstruction()) + " / " + std::to_string(proc->getTotalInstructions());

                std::stringstream ss;
                ss << std::left  << std::setw(15) << proc->getName()
                   << std::left  << std::setw(26) << proc->getCreationTime()
                   << std::left  << std::setw(12) << coreStr
                   << std::right << std::setw(10) << insStr << "\n";

                output += ss.str();
            }
        }
        if (runningPids.empty()) output += "(No active running processes)\n";

        output += "\nFinished processes:\n";
        // 2. Finished Processes Segment
        for (int pid : finishedPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                output += proc->getName() + "\t" 
                       + proc->getCreationTime() + "\t"
                       + "Finished\t"
                       + std::to_string(proc->getCurrentInstruction()) + " / " 
                       + std::to_string(proc->getTotalInstructions()) + "\n";
            }
        }
        if (finishedPids.empty()) output += "(No finished processes)\n";
        
        output += "-------------------------------------------------------\n";
        return output;
    }

    // New diagnostic snapshot capturing all queues in order of Ready, Running, Waiting, and Finished
    static std::string buildSnapshotStringComplete() {
        std::vector<int> readyPids;
        std::vector<int> runningPids;
        std::vector<int> waitingPids;
        std::vector<int> finishedPids;

        // Atomically lock down and fetch the full state distribution tracks
        RQ::getSnapshotPids(readyPids, runningPids, waitingPids, finishedPids);

        int totalCores = CPU::getNumCores();
        int coresUsed = static_cast<int>(runningPids.size());
        if (coresUsed > totalCores) coresUsed = totalCores;
        int coresAvailable = totalCores - coresUsed;
        int cpuUtilization = (totalCores > 0) ? static_cast<int>((static_cast<double>(coresUsed) / totalCores) * 100) : 0;

        std::string output = "";
        output += "CPU utilization: " + std::to_string(cpuUtilization) + "%\n";
        output += "Cores used: " + std::to_string(coresUsed) + "\n";
        output += "Cores available: " + std::to_string(coresAvailable) + "\n";
        output += "-------------------------------------------------------\n\n";

        // 1. Ready Processes Segment
        output += "Ready processes:\n";
        for (int pid : readyPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                output += proc->getName() + "\t" 
                       + proc->getCreationTime() + "\t"
                       + "Ready\t"
                       + std::to_string(proc->getCurrentInstruction()) + " / " 
                       + std::to_string(proc->getTotalInstructions()) + "\n";
            }
        }
        if (readyPids.empty()) output += "(No active ready processes)\n";

        // 2. Running Processes Segment
        output += "\nRunning processes:\n";
        for (int pid : runningPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                output += proc->getName() + "\t" 
                       + proc->getCreationTime() + "\t"
                       + "Core: " + std::to_string(proc->getAssignedCore()) + "\t"
                       + std::to_string(proc->getCurrentInstruction()) + " / " 
                       + std::to_string(proc->getTotalInstructions()) + "\n";
            }
        }
        if (runningPids.empty()) output += "(No active running processes)\n";

        // 3. Waiting Processes Segment
        output += "\nWaiting processes:\n";
        for (int pid : waitingPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                output += proc->getName() + "\t" 
                       + proc->getCreationTime() + "\t"
                       + "Waiting\t"
                       + std::to_string(proc->getCurrentInstruction()) + " / " 
                       + std::to_string(proc->getTotalInstructions()) + "\n";
            }
        }
        if (waitingPids.empty()) output += "(No active waiting processes)\n";

        // 4. Finished Processes Segment
        output += "\nFinished processes:\n";
        for (int pid : finishedPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                output += proc->getName() + "\t" 
                       + proc->getCreationTime() + "\t"
                       + "Finished\t"
                       + std::to_string(proc->getCurrentInstruction()) + " / " 
                       + std::to_string(proc->getTotalInstructions()) + "\n";
            }
        }
        if (finishedPids.empty()) output += "(No finished processes)\n";
        
        output += "-------------------------------------------------------\n";
        return output;
    }

public:
    // Pure static implementation
    Report() = delete;

    // Handles the live display for the "screen -ls" command workflow and returns the payload
    static std::string printScreenList() {
        return buildSnapshotString();
    }

    // Handles the file export system for the "report-util" command workflow
    static bool generateUtilReport() {
        std::ofstream logFile("csopesy-log.txt", std::ios::out | std::ios::trunc);
        if (!logFile.is_open()) {
            return false;
        }

        logFile << buildSnapshotString();
        logFile.close();
        return true;
    }
};

#endif