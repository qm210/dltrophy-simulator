//
// Created by qm210 on 27.07.2025.
//
// ... also part of my borderline maniac approach to "shim" the WLED base.
//     due to the structure of the FX_DEADLINE_TROPHY.h Header, this
//     can only be included by one translation unit, i.e. one .cpp, not any .h

#include "Prototyper.h"
#include "FX_DEADLINE_TROPHY.h"

mode_ptr Prototyper::_mode = mode_DeadlineTrophy;

float beat = 0.;
float bpm = 0.;
float globalTime = DeadlineTrophy::FxHelpers::secondNow();