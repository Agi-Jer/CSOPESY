#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>

class Process {
private:
    int pid;                    // Unique process ID
    std::string name;           // Process name
    int assignedCore;           // CPU core currently assigned
    bool finished;              // Is this process finished?

    int currentInstruction;     // Current instruction index
    int totalInstructions;      // Total instructions in the program

    std::vector<Instruction> instructions;
    // The actual instructions to be processed

public:
    // Simulation for processing one instruction line
    void executeInstruction() {
        if (!finished) {

            // Execute current instruction
            run(instructions[currentInstruction]);

            currentInstruction++;

            // Check if process is done
            if (currentInstruction >= totalInstructions) {
                finished = true;
            }
        }
    }
};

#endif