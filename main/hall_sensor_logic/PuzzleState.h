#pragma once

// Tracks the puzzle's solved state
class PuzzleState {
public:
    void markSolved();
    bool isSolved() const;
    void reset();

private:
    bool solved = false;
};