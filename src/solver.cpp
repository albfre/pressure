#include "solver.h"

#ifndef EMSCRIPTEN
#include <omp.h>
#endif

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <ranges>
#include <sstream>

namespace PressureOptimization {
State Solver::solve(const State initial_state, const size_t depth_left,
                    const size_t max_num_of_tests_per_target,
                    const JSCallback callback) {
  const auto num_targets = initial_state.num_targets();

#ifdef EMSCRIPTEN
  auto state = initial_state;
  auto best_state = initial_state;
  size_t num_tests = 0;
  solve_(state, best_state, num_tests, depth_left, max_num_of_tests_per_target,
         std::pair{0, num_targets}, callback);
  if (callback != nullptr) {
    callback(static_cast<int>(num_tests),
             best_state.get_worst_case_difference(),
             best_state.get_average_difference());
  }
#else
  omp_set_num_threads(2);
  if (callback != nullptr) {
    std::cout << "callback" << std::endl;
  }
  auto best_states = std::vector<State>(num_targets, initial_state);
  auto num_tests_vec = std::vector<size_t>(num_targets, 0);
#pragma omp parallel for
  for (size_t target_index = 0; target_index < num_targets; ++target_index) {
    auto state = initial_state;
    auto best_state = initial_state;
    size_t num_tests = 0;
    solve_(state, best_state, num_tests, depth_left,
           max_num_of_tests_per_target,
           std::pair{target_index, target_index + 1});
    best_states[target_index] = std::move(best_state);
    num_tests_vec[target_index] = num_tests;
  }
  auto best_state =
      *std::ranges::min_element(best_states, [](const auto& a, const auto& b) {
        return a.objective_value() < b.objective_value();
      });
  const auto num_tests =
      std::accumulate(num_tests_vec.cbegin(), num_tests_vec.cend(), 0);
  best_state.set_num_tests(num_tests);
#endif

  return best_state;
}

void Solver::solve_(State& state, State& best_state, size_t& num_tests,
                    const size_t depth_left, const size_t max_num_of_tests,
                    std::optional<std::pair<size_t, size_t>> target_index_range,
                    const JSCallback callback) {
  if (depth_left == 0) {
    return;
  }
  if (num_tests >= max_num_of_tests) {
    return;
  }
  if (state.is_worse_than(best_state)) {
    return;
  }
  const auto [target_begin_index, target_end_index] =
      target_index_range.value_or(std::pair{0, state.num_targets()});
  for (size_t ti = target_begin_index; ti < target_end_index; ++ti) {
    for (size_t di = 0; di < state.num_donors(); ++di) {
      if (!state.is_admissible(di, ti)) {
        continue;
      }
      state.apply(di, ti);
      if (state.objective_value() < best_state.objective_value()) {
        best_state = state;
      }
      ++num_tests;
      if (callback != nullptr && num_tests % 200000 == 0) {
        callback(static_cast<int>(num_tests),
                 best_state.get_worst_case_difference(),
                 best_state.get_average_difference());
      }

      // Restricted target index range should only be used in outermost call
      solve_(state, best_state, num_tests, depth_left - 1, max_num_of_tests,
             std::pair{0, state.num_targets()}, callback);
      state.unapply_last_event();
    }
  }
}
}  // namespace PressureOptimization