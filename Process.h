#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include "Instruction.h"
#include "SymbolTable.h"
#include "GenerateProcessInstructions.h" // Include factory class

/*
Abstraction for Process

Handles all the necessary load for keeping track of processing time and
running the low level logic for processing the instructions
*/
class Process {
private:
    inline static int globalPidCounter = 0;

    int pid;                    // Unique process ID
    std::string name;           // Process name
    int assignedCore;           // CPU core currently assigned
    bool finished;              // Is this process finished?

    int currentInstruction;     // Current instruction index
    int totalInstructions;      // Total instructions in the program

    std::vector<Instruction> instructions; // The actual instructions to be processed

    // TIME TRACKING FIELDS
    std::chrono::system_clock::time_point dateCreated;
    std::chrono::system_clock::time_point dateLastInstruction;

    // RUNTIME ENGINE FIELDS
    SymbolTable sTable;         // Private memory bank for THIS process context
    unsigned int sleepTimer = 0;// Handles process sleep cycles
    const std::vector<std::string> VAR_POOL = {"x", "y", "z", "i", "j", "k"};

    // Internal helper to get a safe value (literal or variable)
    int32_t resolveValue(const std::string& token) {
        if (sTable.exists(token)) {
            return std::get<unsigned short>(sTable.getValue(token));
        }
        for (const auto& var : VAR_POOL) {
            if (token == var) {
                sTable.insert(token, "unsigned short", "0");
                return 0;
            }
        }
        return std::stoi(token);
    }

    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) const {
        std::time_t currentTime = std::chrono::system_clock::to_time_t(tp);
        std::tm* localTime = std::localtime(&currentTime);
        if (!localTime) return "(00/00/0000 00:00:00AM)";

        char buffer[40];
        std::strftime(buffer, sizeof(buffer), "(%m/%d/%Y %I:%M:%S%p)", localTime);
        return std::string(buffer);
    }

public:
    // Safe default constructor for display slots
    Process() 
        : pid(-1), name("unassigned"), assignedCore(-1), finished(false), 
          currentInstruction(0), totalInstructions(0) {
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // Standard constructor managing random generation ranges
    Process(std::string processName, int minIns, int maxIns)
        : pid(globalPidCounter++), name(processName), assignedCore(-1), finished(false),
          currentInstruction(0) {
        
        // Call the generation offshoot factory once to build the instruction list
        instructions = GenerateProcessInstructions::createProgram(minIns, maxIns);

        totalInstructions = static_cast<int>(instructions.size());
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // Core runner executed inside your processing ticks
    void runCycle() {
        if (finished || sleepTimer > 0) return;
        if (currentInstruction >= static_cast<int>(instructions.size())) return;

        const Instruction& current = instructions[currentInstruction];
        OpCode op = current.getOpCode();
        const auto& args = current.getArgs();

        switch (op) {
            case OpCode::ADD: {
                if (!sTable.exists(args[0])) {
                    sTable.insert(args[0], "unsigned short", "0");
                }
                int32_t val2 = resolveValue(args[1]);
                int32_t val3 = resolveValue(args[2]);
                int32_t result = val2 + val3;
                
                if (result > 65535) result = 65535;
                if (result < 0) result = 0;

                sTable.update(args[0], std::to_string(result));
                break;
            }
            case OpCode::SUBTRACT: {
                if (!sTable.exists(args[0])) {
                    sTable.insert(args[0], "unsigned short", "0");
                }
                int32_t val2 = resolveValue(args[1]);
                int32_t val3 = resolveValue(args[2]);
                int32_t result = val2 - val3;

                if (result > 65535) result = 65535;
                if (result < 0) result = 0;

                sTable.update(args[0], std::to_string(result));
                break;
            }
            case OpCode::SLEEP: {
                sleepTimer = std::stoi(args[0]);
                break;
            }
            case OpCode::DECLARE: {
                sTable.insert(args[0], "unsigned short", args[1]);
                break;
            }
            case OpCode::PRINT: {
                // Hook screen buffer logs here
                break;
            }
        }

        currentInstruction++;
        dateLastInstruction = std::chrono::system_clock::now();

        if (currentInstruction >= static_cast<int>(instructions.size())) {
            finished = true;
        }
    }

    void decrementSleep() { if (sleepTimer > 0) sleepTimer--; }
    bool isSleeping() const { return sleepTimer > 0; }

    // Setters & Getters
    void setAssignedCore(int coreId) { this->assignedCore = coreId; }
    std::string getName() const { return name; }
    int getPid() const { return pid; }
    int getCurrentInstruction() const { return currentInstruction; }
    int getTotalInstructions() const { return totalInstructions; }
    bool isFinished() const { return finished; }
    int getAssignedCore() const { return assignedCore; }

    std::string getCreationTime() const { return formatTimestamp(dateCreated); }
    std::string getLastInstructionTime() const { return formatTimestamp(dateLastInstruction); }
};

#endif