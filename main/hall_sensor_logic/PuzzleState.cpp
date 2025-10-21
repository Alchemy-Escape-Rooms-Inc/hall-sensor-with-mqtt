// Implementation for PuzzleState
#include "hall_sensor_logic/PuzzleState.h"

void PuzzleState::markSolved() {
    solved = true;
}

bool PuzzleState::isSolved() const {
    return solved;
}

void PuzzleState::reset() {
    solved = false;
}