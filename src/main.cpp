// main.cpp : This file contains the 'main' function. Program execution begins
// and ends there.

#include <omp.h>

#include <iostream>
#include <vector>

#include "solver.h"
#include "state.h"

using Tube = PressureOptimization::Tube;
using State = PressureOptimization::State;
using Solver = PressureOptimization::Solver;

int main() {
  omp_set_num_threads(2);
  if (false) {
    auto targets = std::vector<Tube>{{12, 100, 200},
                                     {12, 80, 200},
                                     {8, 70, 300},
                                     {8, 100, 300},
                                     {24, 80, 232}};
    auto donors = std::vector<Tube>{{12, 232}, {12, 232}, {12, 232},
                                    {12, 232}, {10, 300}, {10, 300}};
    State state(std::move(targets), std::move(donors));
    std::cout << "Initial state:" << std::endl;
    state.print();
    // Solver::solve(state, 8);  // 67 in 0.1 sec, 63227998
    // Solver::solve(state, 9);  // 58 in 0.4 sec, 205034831
    // Solver::solve(state, 10);  // 51 in 1 sec
    // Solver::solve(state, 11);  // 46.96 in 2 sec
    Solver::solve(std::move(state), 12, 2);  // 44.78 in 3 sec
  } else {
    auto targets =
        std::vector<Tube>{{12, 100, 200}, {12, 80, 200}, {8, 70, 300},
                          {8, 100, 300},  {12, 80, 232}, {12, 70, 232}};

    auto donors = std::vector<Tube>{{12, 232}, {12, 232}, {12, 232},
                                    {12, 232}, {10, 300}, {10, 300}};
    State state(std::move(targets), std::move(donors));
    std::cout << "Initial state:" << std::endl;
    state.print();
    // Solver::solve(state, 7);  // 88.8 in 0.16 s
    // Solver::solve(state, 8);  // 81 in 0.9 s
    // Solver::solve(state, 9);  // 76 in 4.3 s
    // Solver::solve(state, 10, 2);  // 58.7 in 16 s
    Solver::solve(std::move(state), 11, 2);  // 51.5 in 48 s
    // Solver::solve(state, 12);  // 47.1 in 102 s
  }
}