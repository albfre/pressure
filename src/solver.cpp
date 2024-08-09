#include "solver.h"

#include <omp.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <ranges>

namespace PressureOptimization {
State Solver::solve(const State initial_state, const size_t depth_left,
                    const size_t num_chunks, size_t max_num_of_tests) {
  std::cout << std::endl
            << "Solving with maximum number of connections: " << depth_left
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
    const auto extra_targets = (chunk_index < remaining_targets) ? 1 : 0;
    const auto target_begin_index = chunk_index * targets_per_chunk;
    const auto target_end_index =
        target_begin_index + targets_per_chunk + extra_targets;
    auto state = initial_state;
    auto best_state = initial_state;
    size_t num_tests = 0;
    solve_(state, best_state, num_tests, depth_left, max_num_of_tests,
           std::pair{target_begin_index, target_end_index});
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

void Solver::solve_(
    State& state, State& best_state, size_t& num_tests, const size_t depth_left,
    const size_t max_num_of_tests,
    std::optional<std::pair<size_t, size_t>> target_index_range) {
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

      // Restricted target index range should only be used in outermost call
      solve_(state, best_state, num_tests, depth_left - 1, max_num_of_tests);
      state.unapply_last_event();
    }
  }
}

}  // namespace PressureOptimization