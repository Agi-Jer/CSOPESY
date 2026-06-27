#ifndef SCREEN_H
#define SCREEN_H

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include "ProcessMap.h"
#include "Process.h"
#include "RQ.h"

class LinuxInput {
    struct termios orig_termios;
public:
    LinuxInput() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
    ~LinuxInput() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
    char getch_nonblocking() {
        char ch = 0;
        ssize_t nread = read(STDIN_FILENO, &ch, 1);
        if (nread > 0) {
            if (ch == 27) { 
                char garbage[2];
                while(read(STDIN_FILENO, garbage, 1) > 0); 
                return 0; 
            }
            return ch;
        }
        return 0;
    }
};

class ConsoleBuffer {
    int width, height;
    std::vector<std::string> frontBuffer, backBuffer;

public:
    ConsoleBuffer(int w, int h) : width(w), height(h), 
        frontBuffer(h, std::string(w, ' ')), 
        backBuffer(h, std::string(w, ' ')) {
        std::cout << "\033[2J" << std::flush; 
    }

    void write(int x, int y, const std::string& text) {
        if (y < 0 || y >= height) return;
        for (size_t i = 0; i < text.length(); ++i) {
            if (x + i < (size_t)width && x + i >= 0) {
                backBuffer[y][x + i] = text[i];
            }
        }
    }

    void render(int cursorX, int cursorY) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (backBuffer[y][x] != frontBuffer[y][x]) {
                    std::cout << "\033[" << y + 1 << ";" << x + 1 << "H" << backBuffer[y][x];
                    frontBuffer[y][x] = backBuffer[y][x];
                }
            }
        }
        std::cout << "\033[" << cursorY << ";" << cursorX << "H" << std::flush;
    }

    void clearBackBuffer() {
        for (auto& row : backBuffer) std::fill(row.begin(), row.end(), ' ');
    }
};

class Screen {
public:
    Screen() = delete; // Pure static UI component

    // OVERLOAD 1: Standard Re-attach (Equivalent to screen -r)
    static bool enterProcessScreen(const std::string& processName) {
        Process* proc = ProcessMap::getProcessByName(processName);

        // Guard: Process doesn't exist at all in the registry
        if (proc == nullptr) {
            std::cout << "Process " << processName << " not found.\n";
            return false;
        }

        // Guard: Process exists but has already finished (Strict spec match)
        if (proc->isFinished()) {
            std::cout << "Process " << processName << " not found.\n";
            return false;
        }

        //My ass is on Linux
        const int WIDTH = 114;
        const int HEIGHT = 24;
        
        ConsoleBuffer console(WIDTH, HEIGHT);
        LinuxInput inputSystem;

        std::string currentCommand = "";
        auto lastRefreshTime = std::chrono::steady_clock::now();
        
        bool running = true;
        bool forceRedraw = true;

        // New loop bound completely inside new isolated screen space
        while (running) {
            bool shouldRender = false;
            char ch = inputSystem.getch_nonblocking();
            
            if (ch != 0) {
                shouldRender = true; // Keypress instantly marks buffer dirty for execution
                if (ch == 127 || ch == 8) { 
                    if (!currentCommand.empty()) currentCommand.pop_back();
                }
                else if (ch == '\n' || ch == '\r') { 
                    if (!currentCommand.empty()) {
                        if (currentCommand == "process-smi") {
                            forceRedraw = true;
                        } 
                        else if (currentCommand == "exit") {
                            running = false;
                        } 
                        else {
                            // Print command output placeholder logic without disrupting core layout matrices
                            forceRedraw = true;
                        }
                        currentCommand = ""; 
                    }
                }
                else if (ch >= 32 && ch <= 126) { 
                    if (currentCommand.length() < 80) currentCommand += ch;
                }
            }

            // Fixed refresh rate targeting 1 Hz (every 1000 milliseconds)
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefreshTime).count() >= 1000 || forceRedraw) {
                shouldRender = true;
                forceRedraw = false;
                lastRefreshTime = now;
            }

            if (shouldRender) {
                console.clearBackBuffer();

                if (proc != nullptr) {
                    console.write(0, 1, "Process name: " + proc->getName());
                    console.write(0, 2, "ID: " + std::to_string(proc->getPid()));
                    console.write(0, 3, "Logs:");

                    // Fetch and print out chronological screen print entries
                    const auto& logs = proc->getScreenLogBuffer();
                    int currentRow = 4;
                    for (const auto& logLine : logs) {
                        if (currentRow < HEIGHT - 5) {
                            console.write(0, currentRow++, logLine);
                        }
                    }

                    // Spec & Mockup Match: If finished, print "Finished!" and skip line numbers entirely
                    if (proc->isFinished()) {
                        console.write(0, currentRow + 1, "Finished!");
                    } else {
                        // Only prints if the process is actively running or waiting
                        console.write(0, currentRow + 1, "Current instruction line: " + std::to_string(proc->getCurrentInstruction()));
                        console.write(0, currentRow + 2, "Lines of code: " + std::to_string(proc->getTotalInstructions()));
                    }
                }

                std::string promptStr = "root:\\> " + currentCommand;
                console.write(0, HEIGHT - 2, promptStr);

                // Dynamically tracks the terminal standard output character cursor array matrix offset
                int targetCursorX = promptStr.length() + 1;
                console.render(targetCursorX, HEIGHT - 1);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        std::cout << "\033[" << HEIGHT << ";1H\033[2J" << std::flush;
        return true;
    }

    // OVERLOAD 2: Spawn & Attach (Equivalent to screen -s)
    static bool enterProcessScreen(const std::string& processName, int minIns, int maxIns) {
        // Check if a process with this name already exists to prevent duplication collisions
        if (ProcessMap::getProcessByName(processName) != nullptr) {
            std::cout << "Error: A process named '" << processName << "' already exists.\n";
            return false;
        }

        // Call the function from ProcessMap to instantiate and register it
        int newPid = ProcessMap::createNewProcess(processName, minIns, maxIns);

        // Put the new process into the global Ready Queue so the CPU starts crunching it
        RQ::addToReady(newPid);

        // Fall back directly to the original function to clear the screen and start the loop
        return enterProcessScreen(processName);
    }
};

#endif