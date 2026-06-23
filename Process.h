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
    // New process execution states
    enum class ProcessState {
        READY,
        RUNNING,
        WAITING,
        FINISHED
    };

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
    ProcessState state = ProcessState::READY; // Current scheduling lifecycle state

    // Check if String args is number or variable
    // If Variable, return the value in symbol table or o
    // if Doesnt exist in symbol table, declare as 0
    // Otherwise return the token as int
    int32_t resolveValue(const std::string& token) {
        if (sTable.exists(token)) {
            return std::get<unsigned short>(sTable.getValue(token));
        }

        // Check the central pool inside the generator class
        if (GenerateProcessInstructions::isTokenInVariablePool(token)) {
            sTable.insert(token, "unsigned short", "0");
            return 0;
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

    //---------------------------
    //Execution
    //---------------------------

    //Wrapper for all the switch cases
    void executeInstruction(const Instruction& current) {
        OpCode op = current.getOpCode();
        const auto& args = current.getArgs();

        switch (op) {
            case OpCode::ADD:       executeAdd(args);      break;
            case OpCode::SUBTRACT:  executeSubtract(args); break;
            case OpCode::SLEEP:     executeSleep(args);    break;
            case OpCode::DECLARE:   executeDeclare(args);  break;
            case OpCode::PRINT:     executePrint(args);    break;
        }
    }

    
    void executeAdd(const std::vector<std::string>& args) {
        //Check if it exists, if not declate the variable
        if (!sTable.exists(args[0])) {
            sTable.insert(args[0], "unsigned short", "0");
        }

        int32_t val2 = resolveValue(args[1]);
        int32_t val3 = resolveValue(args[2]);
        int32_t result = val2 + val3;
        
        if (result > 65535) result = 65535;
        if (result < 0) result = 0;

        sTable.update(args[0], std::to_string(result));
    }

    void executeSubtract(const std::vector<std::string>& args) {
        //Check if it exists, if not declate the variable
        if (!sTable.exists(args[0])) {
            sTable.insert(args[0], "unsigned short", "0");
        }
        int32_t val2 = resolveValue(args[1]);
        int32_t val3 = resolveValue(args[2]);
        int32_t result = val2 - val3;

        if (result > 65535) result = 65535;
        if (result < 0) result = 0;

        sTable.update(args[0], std::to_string(result));
    }

    void executeSleep(const std::vector<std::string>& args) {
        sleepTimer = std::stoi(args[0]);
        state = ProcessState::WAITING; // Automatically switch to WAITING on SLEEP evaluation
    }

    void executeDeclare(const std::vector<std::string>& args) {
        sTable.insert(args[0], "unsigned short", args[1]);
    }

    void executePrint(const std::vector<std::string>& args) {
        // Hook screen buffer logs here when needed
    }
    
public:
    // Safe default constructor for display slots
    Process() 
        : pid(-1), name("unassigned"), assignedCore(-1), finished(false), 
          currentInstruction(0), totalInstructions(0), state(ProcessState::READY) {
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // Standard constructor managing random generation ranges
    Process(std::string processName, int minIns, int maxIns)
        : pid(globalPidCounter++), name(processName), assignedCore(-1), finished(false),
          currentInstruction(0), state(ProcessState::READY) {
        
        // Call the generation offshoot factory once to build the instruction list
        instructions = GenerateProcessInstructions::createProgram(minIns, maxIns);

        totalInstructions = static_cast<int>(instructions.size());
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // Main function that begin actually running the instructions
    // Call this to execute one line of code
    // Wrapper for execute instruction
    void runCycle() {
        //Check if Process if Asleep or Over
        if (finished || state == ProcessState::FINISHED || sleepTimer > 0 || state == ProcessState::WAITING) return;
        if (currentInstruction >= totalInstructions) return;

        // Route the current active line to the execution handler
        executeInstruction(instructions[currentInstruction]);

        currentInstruction++;
        dateLastInstruction = std::chrono::system_clock::now();

        if (currentInstruction >= static_cast<int>(instructions.size())) {
            finished = true;
            state = ProcessState::FINISHED;
        }
    }

    void decrementSleep() { 
        if (sleepTimer > 0) {
            sleepTimer--; 
            if (sleepTimer == 0 && state == ProcessState::WAITING) {
                state = ProcessState::READY; // Back to ready queue once sleep expires
            }
        }
    }
    
    bool isSleeping() const { return sleepTimer > 0 || state == ProcessState::WAITING; }

    // State Modifiers
    void setReady()    { if (state != ProcessState::FINISHED) state = ProcessState::READY; }
    void setRunning()  { if (state != ProcessState::FINISHED) state = ProcessState::RUNNING; }
    void setWaiting()  { if (state != ProcessState::FINISHED) state = ProcessState::WAITING; }
    void setFinished() { state = ProcessState::FINISHED; finished = true; }

    // Setters & Getters
    void setAssignedCore(int coreId) { this->assignedCore = coreId; }
    std::string getName() const { return name; }
    int getPid() const { return pid; }
    int getCurrentInstruction() const { return currentInstruction; }
    int getTotalInstructions() const { return totalInstructions; }
    
    // Checks both the legacy boolean and the localized enum state safely
    bool isFinished() const { return finished || state == ProcessState::FINISHED; }
    int getAssignedCore() const { return assignedCore; }

    // Converts enum to human-readable text for visual terminal dumps
    std::string getStateString() const {
        switch (state) {
            case ProcessState::READY:    return "READY";
            case ProcessState::RUNNING:  return "RUNNING";
            case ProcessState::WAITING:  return "WAITING";
            case ProcessState::FINISHED: return "FINISHED";
        }
        return "UNKNOWN";
    }

    std::string getCreationTime() const { return formatTimestamp(dateCreated); }
    std::string getLastInstructionTime() const { return formatTimestamp(dateLastInstruction); }
};

#endif