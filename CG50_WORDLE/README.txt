WORDLE for Casio fx-CG50 — source project
========================================

This folder contains the complete native add-in source project. It is designed
to build into WORDLE.g3a, which appears as WORDLE in the calculator MAIN MENU.

FEATURES
--------
- Random five-letter answer on each new game.
- Six guesses.
- Green = correct letter and position; yellow = correct letter elsewhere;
  gray = letter not in the answer. Duplicate letters are scored correctly.
- F1 opens the statistics screen.
- Statistics are saved in /WORDLE.DAT on calculator storage.
- Games played, wins, losses, win percentage, current/best streak, and
  distribution of wins in 1–6 guesses.
- Statistics remain after closing the add-in or powering off the calculator.

CONTROLS
--------
UP / DOWN     Change selected letter
LEFT / RIGHT  Move between letter slots
DEL           Set selected letter to A
EXE           Submit guess; after a finished game, start a new game
F1            View statistics; F1 or EXIT returns to the game
EXIT          Quit the add-in

BUILD REQUIREMENTS
------------------
You need the fxSDK, gint, fxlibc, OpenLibm, and the SH cross-compiler.
The supported Windows workflow is WSL2 with Ubuntu. A native Windows compiler
is not included. The build has not been run in this environment because the
fxSDK/SH toolchain is not installed here.

BUILD ON WINDOWS WITH WSL2
--------------------------
1. Extract this entire folder somewhere simple, e.g. C:\CG50\WORDLE.
2. Install WSL2 and Ubuntu, then install/configure fxSDK and gint in Ubuntu.
3. Double-click build-wsl.bat, or open Ubuntu and run:
     cd /mnt/c/CG50/WORDLE
     fxsdk build-cg
4. The compiled file should be WORDLE.g3a in this project folder.

The Makefile is included as a convenience wrapper; `make` runs `fxsdk build-cg`.
The recommended command is still `fxsdk build-cg`, because fxSDK configures
the correct CMake cross-compilation environment.

INSTALL ON THE CALCULATOR
-------------------------
1. Connect the fx-CG50 by USB and choose USB Flash / mass-storage mode.
2. Copy WORDLE.g3a to the calculator storage root (not @MainMem/PROGRAM).
3. Safely eject the calculator. WORDLE should appear in MAIN MENU.

STATISTICS FILE
---------------
The add-in writes /WORDLE.DAT in calculator storage. Do not delete this file
if you want to keep your statistics. Removing it resets the statistics.
