// main.cpp : This file contains the 'main' function. Program execution begins
// and ends there.

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "state.h"

namespace {
size_t max_num_of_tests = static_cast<size_t>(1e10);
}

using State = PressureOptimization::State;
using Tube = PressureOptimization::Tube;

void solve(State& state, State& best_state, size_t& num_tests,
           std::unordered_set<size_t>& tested, size_t depth = 3) {
  if (depth == 0) {
    return;
  }
  if (num_tests >= max_num_of_tests) {
    return;
  }
  if (state.is_worse_than(best_state)) {
    return;
  }

  for (size_t ti = 0; ti < state.num_targets(); ++ti) {
    for (size_t di = 0; di < state.num_donors(); ++di) {
      if (!state.is_admissible(di, ti)) {
        continue;
      }
      state.apply(di, ti);
      // if (!tested.contains(state()))
      {
        //  tested.insert(state());
        if (state.objective_value() < best_state.objective_value()) {
          best_state = state;
        }
        ++num_tests;
        solve(state, best_state, num_tests, tested, depth - 1);
      }
      state.unapply_last_event();
    }
  }
}

void solve_with_depth(State initial_state, State& best_state, size_t depth) {
  // std::unordered_map<State, bool> tested;
  std::unordered_set<size_t> tested;
  tested.reserve(1e8);

  size_t num_tests = 0;
  std::cout << std::endl
            << "Solving with maximum number of connections: " << depth
            << std::endl;
  auto t0 = std::chrono::high_resolution_clock::now();
  solve(initial_state, best_state, num_tests, tested, depth);
  auto t1 = std::chrono::high_resolution_clock::now();

  std::cout << "Solution found:" << std::endl;
  best_state.print();
  std::cout << "Num tests: " << num_tests << std::endl;
  std::cout
      << "Elapsed time: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
      << " ms" << std::endl;
}

int main() {
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
    auto best_state = state;
    // solve_with_depth(state, best_state, 8);  // 67 in 0.1 sec, 63227998
    // solve_with_depth(state, best_state, 9);  // 58 in 0.4 sec, 205034831
    // solve_with_depth(state, best_state, 10);  // 51 in 1 sec
    // solve_with_depth(state, best_state, 11);  // 46.96 in 2 sec
    solve_with_depth(state, best_state, 12);  // 44.78 in 3 sec
  } else {
    auto targets =
        std::vector<Tube>{{12, 100, 200}, {12, 80, 200}, {8, 70, 300},
                          {8, 100, 300},  {12, 80, 232}, {12, 70, 232}};

    auto donors = std::vector<Tube>{{12, 232}, {12, 232}, {12, 232},
                                    {12, 232}, {10, 300}, {10, 300}};
    State state(std::move(targets), std::move(donors));
    std::cout << "Initial state:" << std::endl;
    state.print();
    auto best_state = state;
    // solve_with_depth(state, best_state, 7);  // 88.8 in 0.16 s
    // solve_with_depth(state, best_state, 8);  // 81 in 0.9 s
    solve_with_depth(state, best_state, 9);  // 76 in 4.3 s
    // solve_with_depth(state, best_state, 10); // 58.7 in 16 s
    // solve_with_depth(state, best_state, 11); // 51.5 in 48 s
    // solve_with_depth(state, best_state, 12); // 47.1 in 102 s
  }
}