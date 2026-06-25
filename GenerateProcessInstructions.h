#ifndef GENERATE_PROCESS_INSTRUCTIONS_H
#define GENERATE_PROCESS_INSTRUCTIONS_H

#include <vector>
#include <string>
#include <cstdlib>
#include "Instruction.h"

/*
Class to offshoot the complicated logic for randomly generating instructions.
*/

// Temporary scratchpad used only during generation to safely calculate nested sizing
struct InstructionBlueprint {
    OpCode op;
    std::vector<std::string> args;
    int repeats = 1;
    std::vector<InstructionBlueprint> loopBody; // A "set/array" of internal blueprints

    // Recursive helper to look into the future and find the exact flat line count
    size_t calculateFlatSize() const {
        if (op != OpCode::FOR) {
            return 1; // Basic instructions take up exactly 1 line
        }
        size_t bodySize = 0;
        for (const auto& inner : loopBody) {
            bodySize += inner.calculateFlatSize();
        }
        return bodySize * repeats;
    }
};

class GenerateProcessInstructions {
private:
    inline static const std::vector<std::string> VAR_POOL = {"x", "y", "z", "i", "j", "k"};

    // Helper to randomly select between a variable (30% chance) or a number literal (70% chance)
    static std::string selectArgRandomly(const std::string& varToken, const std::string& numToken) {
        return (rand() % 100 < 30) ? varToken : numToken;
    }

    // Unified recursive blueprint generator (Handles all 6 opcodes cleanly)
    // Caches processName to dynamically build the required specification PRINT string literal
    static InstructionBlueprint generateSingleBlueprint(int currentDepth, const std::string& processName, int remainingBuffer) {
        InstructionBlueprint bp;
        
        // Pick randomly from all possible instructions. 
        // If we are at depth 3, cap it so it cannot choose OpCode::FOR anymore (6th type).
        int maxChoice = (currentDepth >= 3 || remainingBuffer <= 200) ? 13 : 15; 
        int choice = rand() % maxChoice; 

        // MNew Weighted rng because my processes were sleeping too much
        if (choice >= 0 && choice < 3)        choice = 0; // PRINT (Weights 0, 1, 2)
        else if (choice >= 3 && choice < 6)   choice = 1; // DECLARE (Weights 3, 4, 5)
        else if (choice >= 6 && choice < 9)   choice = 2; // ADD (Weights 6, 7, 8)
        else if (choice >= 9 && choice < 12)  choice = 3; // SUBTRACT (Weights 9, 10, 11)
        else if (choice == 12)                choice = 4; // SLEEP (Weight 12)
        else if (choice == 13 || choice == 14) choice = 5; // FOR (Weights 13, 14)

        std::string v1 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string v2 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string v3 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string randVal = std::to_string(rand() % 50);
        
        switch (choice) {
            case 0: 
                bp.op = OpCode::PRINT; 
                // Spec: Unless specified, the “msg” in the PRINT function should always be “Hello world from <process_name>!”
                bp.args = {"Hello world from " + processName + "!"}; 
                break;
            case 1: 
                bp.op = OpCode::DECLARE; 
                bp.args = {v1, randVal}; 
                break;
            case 2: 
                bp.op = OpCode::ADD; 
                // Target is always a variable, operands are randomized 30/70
                bp.args = {v1, selectArgRandomly(v2, randVal), selectArgRandomly(v3, randVal)}; 
                break;
            case 3: 
                bp.op = OpCode::SUBTRACT; 
                // Target is always a variable, operands are randomized 30/70
                bp.args = {v1, selectArgRandomly(v2, randVal), selectArgRandomly(v3, randVal)}; 
                break;
            case 4: 
                bp.op = OpCode::SLEEP; 
                bp.args = {std::to_string(1 + (rand() % 10))}; 
                break;
            case 5: {
                bp.op = OpCode::FOR;
                bp.repeats = 2 + (rand() % 4); // Loops 2 to 5 times
                
                // Generates a multi-instruction "set/array" loop body
                int bodySize = 1 + (rand() % 3); 
                for (int i = 0; i < bodySize; ++i) {
                    bp.loopBody.push_back(generateSingleBlueprint(currentDepth + 1, processName, remainingBuffer));
                }
                break;
            }
        }
        return bp;
    }

    // Recursive flattener to transition safe blueprints into operational class Instructions
    static void flattenBlueprint(const InstructionBlueprint& bp, std::vector<Instruction>& flatInstructions) {
        if (bp.op != OpCode::FOR) {
            flatInstructions.push_back(Instruction(bp.op, bp.args));
        } else {
            for (int r = 0; r < bp.repeats; r++) {
                for (const auto& inner : bp.loopBody) {
                    flattenBlueprint(inner, flatInstructions);
                }
            }
        }
    }

    static InstructionBlueprint generateStep(int currentDepth, const std::string& processName, int remainingBuffer) {
        if (remainingBuffer >= 600) {
            InstructionBlueprint masterLoop;
            masterLoop.op = OpCode::FOR;
            masterLoop.repeats = 100;

            InstructionBlueprint addX; addX.op = OpCode::ADD; addX.args = {"x", "x", "1"};
            InstructionBlueprint printX; printX.op = OpCode::PRINT; printX.args = {"Value from: ", "x"};
            
            InstructionBlueprint addY; addY.op = OpCode::ADD; addY.args = {"y", "y", "1"};
            InstructionBlueprint printY; printY.op = OpCode::PRINT; printY.args = {"Value from: ", "y"};
            
            InstructionBlueprint addZ; addZ.op = OpCode::ADD; addZ.args = {"z", "z", "1"};
            InstructionBlueprint printZ; printZ.op = OpCode::PRINT; printZ.args = {"Value from: ", "z"};

            masterLoop.loopBody = {addX, printX, addY, printY, addZ, printZ};
            return masterLoop;
        } else {
            // If we don't have space for a full 600-line loop block but still need to fill 
            // up to minIns, return flat 1-line filler instructions to top it off perfectly.
            InstructionBlueprint filler;
            filler.op = OpCode::PRINT;
            filler.args = {"Hello world from " + processName + "!"};
            return filler;
        }
    }

public:
    // Main factory method called once by Process constructor
    static std::vector<Instruction> createProgram(const std::string& processName, int minIns, int maxIns) {
        bool generationValid = false;
        std::vector<Instruction> finalProgram;

        while (!generationValid) {
            std::vector<InstructionBlueprint> temporaryBatch;
            size_t totalProjectedSize = 0;

            // Generate instructions sequentially at the top level (depth = 1)
            while (totalProjectedSize < (size_t)minIns) {
                int remainingBuffer = maxIns - static_cast<int>(totalProjectedSize);
                //InstructionBlueprint bp = generateSingleBlueprint(0, processName, remainingBuffer);
                InstructionBlueprint bp = generateStep(0, processName, remainingBuffer);
                
                size_t bpSize = bp.calculateFlatSize();
                if (totalProjectedSize + bpSize > (size_t)maxIns) {
                    break; // Safety valve: exit early if the next block would blow past maxIns
                }

                totalProjectedSize += bpSize;
                temporaryBatch.push_back(bp);
            }

            // Verify constraints before finalizing
            if (totalProjectedSize >= (size_t)minIns && totalProjectedSize <= (size_t)maxIns) {
                generationValid = true;
                finalProgram.clear();
                for (const auto& bp : temporaryBatch) {
                    flattenBlueprint(bp, finalProgram);
                }
            }
        }
        return finalProgram;
    }

    // Public static helper to check if a token belongs to the valid variable pool
    static bool isTokenInVariablePool(const std::string& token) {
        for (const auto& var : VAR_POOL) {
            if (token == var) {
                return true;
            }
        }
        return false;
    }
};

#endif