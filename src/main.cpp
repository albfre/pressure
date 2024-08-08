// main.cpp : This file contains the 'main' function. Program execution begins
// and ends there.

#include <omp.h>

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
           const size_t begin_target, const size_t end_target,
           const size_t depth) {
  if (depth == 0) {
    return;
  }
  if (num_tests >= max_num_of_tests) {
    return;
  }
  if (state.is_worse_than(best_state)) {
    return;
  }
  for (size_t ti = begin_target; ti < end_target; ++ti) {
    for (size_t di = 0; di < state.num_donors(); ++di) {
      if (!state.is_admissible(di, ti)) {
        continue;
      }
      state.apply(di, ti);
      if (state.objective_value() < best_state.objective_value()) {
        best_state = state;
      }
      ++num_tests;
      solve(state, best_state, num_tests, 0, state.num_targets(), depth - 1);
      state.unapply_last_event();
    }
  }
}

State start_solve(const State initial_state, const size_t depth,
                  size_t num_chunks = 1) {
  std::cout << std::endl
            << "Solving with maximum number of connections: " << depth
            << std::endl;
  const auto t0 = std::chrono::high_resolution_clock::now();
  max_num_of_tests /= num_chunks;
  const auto num_targets = initial_state.num_targets();
  const auto targets_per_chunk = num_targets / num_chunks;
  const auto remaining_targets = num_targets % num_chunks;

  auto best_states = std::vector<State>(num_chunks, initial_state);
  auto num_tests_vec = std::vector<size_t>(num_chunks, 0);

#pragma omp parallel for
  for (size_t chunk_index = 0; chunk_index < num_chunks; ++chunk_index) {
    const auto target_begin_index = chunk_index * targets_per_chunk;
    const auto target_end_index =
        target_begin_index + targets_per_chunk +
        (chunk_index + 1 == num_chunks ? remaining_targets : 0);
    auto state = initial_state;
    auto best_state = initial_state;
    size_t num_tests = 0;
    solve(state, best_state, num_tests, target_begin_index, target_end_index,
          depth);
    best_states[chunk_index] = std::move(best_state);
    num_tests_vec[chunk_index] = num_tests;
  }

  const auto best_state =
      *std::ranges::min_element(best_states, [](const auto& a, const auto& b) {
        return a.objective_value() < b.objective_value();
      });
  const auto num_tests =
      std::accumulate(num_tests_vec.cbegin(), num_tests_vec.cend(), 0);
  const auto t1 = std::chrono::high_resolution_clock::now();

  std::cout << "Solution found:" << std::endl;
  best_state.print();
  std::cout << "Num tests: " << num_tests << std::endl;
  std::cout
      << "Elapsed time: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
      << " ms" << std::endl;
  return best_state;
}

int main() {
  omp_set_num_threads(2);
  if (true) {
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
    // solve_with_depth(state, best_state, 8);  // 67 in 0.1 sec, 63227998
    // solve_with_depth(state, best_state, 9);  // 58 in 0.4 sec, 205034831
    // solve_with_depth(state, best_state, 10);  // 51 in 1 sec
    // solve_with_depth(state, best_state, 11);  // 46.96 in 2 sec
    start_solve(state, 12, 2);  // 44.78 in 3 sec
  } else {
    auto targets =
        std::vector<Tube>{{12, 100, 200}, {12, 80, 200}, {8, 70, 300},
                          {8, 100, 300},  {12, 80, 232}, {12, 70, 232}};

    auto donors = std::vector<Tube>{{12, 232}, {12, 232}, {12, 232},
                                    {12, 232}, {10, 300}, {10, 300}};
    State state(std::move(targets), std::move(donors));
    std::cout << "Initial state:" << std::endl;
    state.print();
    // solve_with_depth(state, best_state, 7);  // 88.8 in 0.16 s
    // solve_with_depth(state, best_state, 8);  // 81 in 0.9 s
    // solve_with_depth(state, best_state, 9);  // 76 in 4.3 s
    start_solve(state, 10, 2);  // 58.7 in 16 s
    // solve_with_depth(state, best_state, 11); // 51.5 in 48 s
    // solve_with_depth(state, best_state, 12);  // 47.1 in 102 s
  }
}