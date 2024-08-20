// main.cpp : This file contains the 'main' function. Program execution begins
// and ends there.

#include <chrono>
#include <iostream>
#include <vector>

#include "solver.h"
#include "state.h"

using Tube = PressureOptimization::Tube;
using State = PressureOptimization::State;
using Solver = PressureOptimization::Solver;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <max_depth>" << std::endl;
    return 1;
  }

  size_t max_depth = 0;
  try {
    const auto max_depth_i = std::stoi(argv[1]);
    if (max_depth_i <= 0) {
      throw std::out_of_range("max_depth must be positive");
    }
    max_depth = static_cast<size_t>(max_depth_i);
  } catch (const std::exception& e) {
    std::cerr << "Error parsing max_depth: " << e.what() << std::endl;
    return 1;
  }

  std::cout << std::endl << "Solving" << std::endl;
  const auto t0 = std::chrono::high_resolution_clock::now();
  auto targets = std::vector<Tube>();
  auto donors = std::vector<Tube>();

  if (false) {
    targets = std::vector<Tube>{{12, 100, 200},
                                {12, 80, 200},
                                {8, 70, 300},
                                {8, 100, 300},
                                {24, 80, 232}};
    donors = std::vector<Tube>{{12, 232}, {12, 232}, {12, 232},
                               {12, 232}, {10, 300}, {10, 300}};
    /*
    8:  67 in 0.1 s, 63227998
    9:  58 in 0.4 s, 205034831
    10: 51 in 1 s
    11: 46.96 in 2 s
    12: 44.78 in 3 s
    */
  } else {
    targets = std::vector<Tube>{{12, 100, 200}, {12, 80, 200}, {8, 70, 300},
                                {8, 100, 300},  {12, 80, 232}, {12, 70, 232}};

    donors = std::vector<Tube>{{12, 232}, {12, 232}, {12, 232},
                               {12, 232}, {10, 300}, {10, 300}};
    /*
    7: 88.8 in 0.16 s
    8: 81 in 0.9 s
    9: 76 in 4.3 s
    10: 58.7 in 16 s
    11: 51.5 in 48 s
    12: 47.1 in 102 s
    */
  }

  State state(std::move(donors), std::move(targets));
  std::cout << "Initial state:" << std::endl;
  state.print();
  auto best_state = argc == 2
                        ? Solver::solve(std::move(state), max_depth)
                        : Solver::solve_bisected(std::move(state), max_depth);

  const auto t1 = std::chrono::high_resolution_clock::now();

  std::cout << "Solution found:" << std::endl;
  best_state.print();
  std::cout
      << "Elapsed time: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
      << " ms" << std::endl;
}
