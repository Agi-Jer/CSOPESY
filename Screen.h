#ifndef SCREEN_H
#define SCREEN_H

#include <ncurses.h>
#include <vector>
#include <string>
#include "CPU.h"
#include "RQ.h"
#include "Process.h"

/*
Handles all the scren processing logic

e.g. Screen -ls, Cpu Utilization

Everything is handled statically
All data is gathered and handled at time of UI calling
*/

class Screen {
private:
    // Initializes ncurses properties safely for a brief snapshot view
    static void initialize() {
        initscr();
        cbreak();
        noecho();
        curs_set(0); // Hide the terminal cursor
        keypad(stdscr, TRUE);
    }

    // Restores standard terminal properties on exit so std::cin works again
    static void finalize() {
        endwin();
    }

public:
    // Takes a snapshot, displays it, and blocks until a key is pressed
    static void displayScreenLS(const CPU& cpu) {
        // 1. Boot up ncurses window layer
        initialize();
        erase();

        int currentRow = 0;
        
        // --- 1. Header Separation Line ---
        mvprintw(currentRow++, 0, "---------------------------------------------------------");
        mvprintw(currentRow++, 0, "Running processes:");

        // --- 2. Render Active Running Processes ---
        std::vector<Process> running = cpu.getRunningProcesses();
        for (const auto& p : running) {
            // Replicating column-width spacing matching your screenshots exactly:
            // Name at x=0, Timestamp at x=12, Core label at x=44, Progress fraction at x=56
            mvprintw(currentRow, 0, "%s", p.getName().c_str());
            mvprintw(currentRow, 12, "%s", p.getCreationTime().c_str());
            mvprintw(currentRow, 44, "Core: %d", p.getAssignedCore());
            mvprintw(currentRow, 56, "%d / %d", p.getCurrentInstruction(), p.getTotalInstructions());
            currentRow++;
        }

        currentRow++; // Spacing layout gap

        // --- 3. Render Finished Processes Section ---
        mvprintw(currentRow++, 0, "Finished processes:");
        
        std::vector<Process> finished = RQ::getFinishedProcesses();
        size_t totalFinished = finished.size();

        // Determine slice boundaries for filtering down to the last 10 elements
        size_t startIdx = 0;
        if (totalFinished > 10) {
            startIdx = totalFinished - 10;
        }

        // Loop and print only the last 10 entries from the vector snapshot
        for (size_t i = startIdx; i < totalFinished; ++i) {
            const auto& p = finished[i];
            mvprintw(currentRow, 0, "%s", p.getName().c_str());
            mvprintw(currentRow, 12, "%s", p.getCreationTime().c_str());
            mvprintw(currentRow, 44, "Finished");
            mvprintw(currentRow, 56, "%d / %d", p.getTotalInstructions(), p.getTotalInstructions());
            currentRow++;
        }

        // --- 4. Overflow Cut-off Indicator Check ---
        if (totalFinished > 10) {
            mvprintw(currentRow++, 0, "... [More than 10 finished processes. Older records cut off] ...");
        }

        // --- 5. Footer Separation Line ---
        mvprintw(currentRow++, 0, "---------------------------------------------------------");
        mvprintw(currentRow++, 0, "Press any key to return to console prompt...");

        // Refresh window buffer to display changes
        refresh();

        // 2. BLOCKING CHECK: Safely wait right here until user hits any key
        nodelay(stdscr, FALSE); // Force getch to wait synchronously
        getch(); 

        // 3. Deconstruct ncurses buffer screen and drop cleanly back to standard CLI terminal
        finalize();
    }
};

#endif