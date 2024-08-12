#pragma once

#include <stddef.h>

#include <memory>
#include <optional>

#include "state.h"

namespace PressureOptimization {

// typedef void (*ProgressCallback)(double progress, double current_best);

class Solver {
 public:
  static State solve(
      const State initial_state, size_t depth_left,
      size_t max_num_of_tests_per_target = static_cast<size_t>(1e9));

 private:
  static void solve_(State& state, State& best_state, size_t& num_tests,
                     size_t depth_left, size_t max_num_of_tests,
                     std::optional<std::pair<size_t, size_t>>
                         target_index_range = std::nullopt);
};
}  // namespace PressureOptimization