#pragma once

#include <stddef.h>

#include <functional>
#include <optional>

#include "state.h"

namespace PressureOptimization {

using JSCallback = std::function<void(int, double, double)>;

class Solver {
 public:
  static State solve(const State initial_state, size_t depth_left,
                     size_t max_num_of_tests = static_cast<size_t>(1e8),
                     JSCallback callback = nullptr);

  // An approximate solver that explores partitions of the tubes into two
  // subsets that are handled individually
  static State solve_bisected(
      const State initial_state, size_t depth_left,
      size_t max_num_of_tests = static_cast<size_t>(1e8),
      JSCallback callback = nullptr);

 private:
  static void solve_(State& state, State& best_state, size_t depth_left,
                     size_t& num_tests, size_t max_num_of_tests,
                     std::optional<std::pair<size_t, size_t>>
                         target_index_range = std::nullopt,
                     JSCallback callback = nullptr);

  static void solve_bisected_(const State& initial_state, State& best_state,
                              size_t depth_left, size_t& num_tests,
                              size_t max_num_of_tests,
                              JSCallback callback = nullptr);

  static std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>>
  get_bipartitions_(size_t num_elements, size_t min_size);

  static void call_callback_(const JSCallback& callback,
                             const State& best_state, size_t num_tests,
                             size_t& previous_callback_count,
                             bool force_callback = false);
};
}  // namespace PressureOptimization
