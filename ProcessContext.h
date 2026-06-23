#include <vector>
#include <string>
#include <cstdlib>
#include "Instruction.h"
#include "SymbolTable.h"

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

//DEPRECATED

/*
This class contains the context and code necesarry for executing Instruction Lines

It handles the Symbol Table and Instruction lines of the program
*/
class ProcessContext {
private:
    std::vector<Instruction> flatInstructions; //Vector of Instructions
    SymbolTable sTable; // Private memory bank for THIS process context
    size_t curInstructionLine = 0; //Keeps track of current instruction line

    // Small controlled pool of variable names
    const std::vector<std::string> VAR_POOL = {"x", "y", "z", "i", "j", "k"};

    // Internal helper to get a safe value (literal or variable)
    int32_t resolveValue(const std::string& token) {
        
        // If it exists in the symbol table, it's a variable
        if (sTable.exists(token)) {
            return std::get<unsigned short>(sTable.getValue(token));
        }

        // If it doesn't exist but matches a known variable pool name, implicit declaration!
        for (const auto& var : VAR_POOL) {
            if (token == var) {
                sTable.insert(token, "unsigned short", "0");
                return 0;
            }
        }
        // Otherwise, treat it as a raw literal number
        return std::stoi(token);
    }

    // Unified recursive blueprint generator (Handles all 6 opcodes cleanly)
    InstructionBlueprint generateSingleBlueprint(int currentDepth) {
        InstructionBlueprint bp;
        
        // Pick randomly from all possible instructions. 
        // If we are at depth 3, cap it so it cannot choose OpCode::FOR anymore (6th type).
        // This mean nested for loops are capped at 3
        int maxChoice = (currentDepth >= 3) ? 5 : 6; 
        int choice = rand() % maxChoice; 

        std::string v1 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string v2 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string v3 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string randVal = std::to_string(rand() % 50);

        switch (choice) {
            case 0: 
                bp.op = OpCode::PRINT; 
                bp.args = {v1}; 
                break;
            case 1: 
                bp.op = OpCode::DECLARE; 
                bp.args = {v1, randVal}; 
                break;
            case 2: 
                bp.op = OpCode::ADD; 
                bp.args = {v1, v2, randVal}; 
                break;
            case 3: 
                bp.op = OpCode::SUBTRACT; 
                bp.args = {v1, v2, v3}; 
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
                    // Recurse deeper down the tree and track depth
                    bp.loopBody.push_back(generateSingleBlueprint(currentDepth + 1));
                }
                break;
            }
        }
        return bp;
    }

    // Recursive flattener to transition safe blueprints into operational class Instructions
    void flattenBlueprint(const InstructionBlueprint& bp) {
        if (bp.op != OpCode::FOR) {
            flatInstructions.push_back(Instruction(bp.op, bp.args));
        } else {
            for (int r = 0; r < bp.repeats; r++) {
                for (const auto& inner : bp.loopBody) {
                    flattenBlueprint(inner);
                }
            }
        }
    }

public:
    // Constructor handles the safety constraints: matching min/max without truncations
    ProcessContext(int minIns, int maxIns) {
        bool generationValid = false;

        while (!generationValid) {
            std::vector<InstructionBlueprint> temporaryBatch;
            size_t totalProjectedSize = 0;

            // Generate instructions sequentially at the top level (depth = 1)
            int initialItems = minIns; 
            for (int i = 0; i < initialItems; ++i) {
                InstructionBlueprint bp = generateSingleBlueprint(1);
                totalProjectedSize += bp.calculateFlatSize();
                temporaryBatch.push_back(bp);
            }

            // If the math validates perfectly within specifications, accept and unpack it!
            if (totalProjectedSize >= (size_t)minIns && totalProjectedSize <= (size_t)maxIns) {
                generationValid = true;
                for (const auto& bp : temporaryBatch) {
                    flattenBlueprint(bp);
                }
            }
            // Over-sized or under-sized batches are implicitly dropped here on loop reset
        }
    }

    // Unchanged function signature left intact
    Instruction generateSingleRandomInstruction() {
        int choice = rand() % 4; // PRINT, DECLARE, ADD, SUBTRACT
        std::string v1 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string v2 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string v3 = VAR_POOL[rand() % VAR_POOL.size()];
        std::string randVal = std::to_string(rand() % 50);

        if (choice == 0) return Instruction(OpCode::PRINT, {v1});
        if (choice == 1) return Instruction(OpCode::DECLARE, {v1, randVal});
        if (choice == 2) return Instruction(OpCode::ADD, {v1, v2, randVal});
        return Instruction(OpCode::SUBTRACT, {v1, v2, v3});
    }

    // Processes a single instruction cycle and manages math logic
    void step(unsigned int& processSleepTimer) {
        if (curInstructionLine >= flatInstructions.size()) return;

        const Instruction& current = flatInstructions[curInstructionLine];
        OpCode op = current.getOpCode();
        const auto& args = current.getArgs();

        switch (op) {
            case OpCode::ADD: {
                // Ensure target variable exists
                if (!sTable.exists(args[0])) {
                    sTable.insert(args[0], "unsigned short", "0");
                }
                
                int32_t val2 = resolveValue(args[1]);
                int32_t val3 = resolveValue(args[2]);
                int32_t result = val2 + val3;
                
                // Clamping rule logic
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
                processSleepTimer = std::stoi(args[0]);
                break;
            }
            case OpCode::DECLARE: {
                sTable.insert(args[0], "unsigned short", args[1]);
                break;
            }
            case OpCode::PRINT: {
                // Handle PRINT logic safely (screen buffer outputs can hook here)
                break;
            }
        }
        curInstructionLine++;
    }

    bool isFinished() const { return curInstructionLine >= flatInstructions.size(); }
    size_t getCurLine() const { return curInstructionLine; }
    size_t getTotalLines() const { return flatInstructions.size(); }
};