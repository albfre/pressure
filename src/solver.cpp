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

namespace {
constexpr size_t callback_interval_ = 200000;
}

namespace PressureOptimization {
State Solver::solve(const State initial_state, const size_t depth_left,
                    const size_t max_num_of_tests, const JSCallback callback) {
  const auto num_targets = initial_state.num_targets();

  size_t num_tests = 0;
#ifdef EMSCRIPTEN
  auto state = initial_state;
  auto best_state = initial_state;
  solve_(state, best_state, depth_left, num_tests, max_num_of_tests,
         std::pair{0, num_targets}, callback);
#else
  omp_set_num_threads(2);
  const auto max_num_of_tests_per_target = max_num_of_tests / num_targets;
  auto best_states = std::vector<State>(num_targets, initial_state);
  auto num_tests_vec = std::vector<size_t>(num_targets, 0);
#pragma omp parallel for
  for (size_t target_index = 0; target_index < num_targets; ++target_index) {
    auto state = initial_state;
    solve_(state, best_states.at(target_index), depth_left,
           num_tests_vec.at(target_index), max_num_of_tests_per_target,
           std::pair{target_index, target_index + 1});
  }
  auto best_state =
      *std::ranges::min_element(best_states, [](const auto& a, const auto& b) {
        return a.objective_value() < b.objective_value();
      });
  num_tests = std::accumulate(num_tests_vec.cbegin(), num_tests_vec.cend(), 0);
  best_state.set_num_tests(num_tests);
#endif

  if (num_tests > max_num_of_tests) {
    solve_bisected_(initial_state, best_state, depth_left, num_tests,
                    num_tests + max_num_of_tests, callback);
  }

  call_callback_(callback, best_state, num_tests, num_tests, true);

  return best_state;
}

State Solver::solve_bisected(const State initial_state, const size_t depth_left,
                             const size_t max_num_of_tests,
                             const JSCallback callback) {
  auto best_state = initial_state;
  size_t num_tests = 0;
  solve_bisected_(initial_state, best_state, depth_left, num_tests,
                  max_num_of_tests, callback);

  call_callback_(callback, best_state, num_tests, num_tests, true);

  return best_state;
}

void Solver::solve_(State& state, State& best_state, const size_t depth_left,
                    size_t& num_tests, const size_t max_num_of_tests,
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
  auto previous_callback_count = num_tests;
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
      call_callback_(callback, best_state, num_tests, previous_callback_count);

      // Restricted target index range should only be used in outermost call
      solve_(state, best_state, depth_left - 1, num_tests, max_num_of_tests,
             std::pair{0, state.num_targets()}, callback);
      state.unapply_last_event();
    }
  }
}

void Solver::solve_bisected_(const State& initial_state, State& best_state,
                             const size_t depth_left, size_t& num_tests,
                             const size_t max_num_of_tests,
                             const JSCallback callback) {
  const auto num_donors = initial_state.num_donors();
  const auto num_targets = initial_state.num_targets();
  const auto s2 = static_cast<size_t>(2);

  const auto donor_partitions =
      get_bipartitions_(num_donors, std::min(s2, num_donors / 2));
  const auto target_partitions =
      get_bipartitions_(num_targets, std::min(s2, num_targets / 2));

  auto previous_callback_count = num_tests;
  const auto min_depth = std::min(s2, depth_left / 2);
  for (const auto& [t1, t2] : target_partitions) {
    for (const auto& [d1, d2] : donor_partitions) {
      auto state1 = initial_state.substate(d1, t1);
      auto state2 = initial_state.substate(d2, t2);
      for (size_t depth1 = min_depth; depth1 + min_depth < depth_left;
           ++depth1) {
        if (num_tests > max_num_of_tests) {
          break;
        }
        call_callback_(callback, best_state, num_tests,
                       previous_callback_count);
        assert(!state1.is_modified());
        assert(!state2.is_modified());
        auto depth2 = depth_left - depth1;
        auto b1 = state1;
        solve_(state1, b1, depth1, num_tests, max_num_of_tests);
        if (b1.objective_value() > best_state.objective_value()) {
          continue;  // state 1 gets more depth in future iterations
        }
        auto b2 = state2;
        solve_(state2, b2, depth2, num_tests, max_num_of_tests);
        if (b2.objective_value() > best_state.objective_value()) {
          break;  // state 2 gets less depth in future iterations
        }
        const auto combined = State::combine(b1, b2, d1, d2, t1, t2);
        if (combined.objective_value() < best_state.objective_value()) {
          best_state = combined;
        }
      }
    }
  }
  best_state.set_num_tests(num_tests);
}

std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>>
Solver::get_bipartitions_(const size_t num_elements, const size_t min_size) {
  std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> bipartitions;
  const auto next_state = [](auto& state) {
    const auto it = std::ranges::find(state, false);
    if (it == state.cend()) {
      return false;
    }
    *it = true;
    std::fill(state.begin(), it, false);
    return true;
  };
  std::deque<bool> state(num_elements, false);

  // Don't include all false or all true
  while (next_state(state)) {
    std::vector<size_t> set1, set2;
    for (size_t i = 0; i < state.size(); ++i) {
      (state[i] ? set1 : set2).push_back(i);
    }
    if (set1.size() >= min_size && set2.size() >= min_size) {
      bipartitions.emplace_back(std::move(set1), std::move(set2));
    }
  }
  return bipartitions;
}

void Solver::call_callback_(const JSCallback& callback, const State& best_state,
                            const size_t num_tests,
                            size_t& previous_callback_count,
                            const bool force_callback) {
  if (callback != nullptr &&
      (num_tests > previous_callback_count + callback_interval_ ||
       force_callback)) {
    callback(static_cast<int>(num_tests),
             best_state.get_worst_case_difference(),
             best_state.get_average_difference());
    previous_callback_count = num_tests;
  }
}
}  // namespace PressureOptimization
