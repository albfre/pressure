#include "state.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <ranges>

namespace PressureOptimization {
State::State(std::vector<Tube> targets, std::vector<Tube> donors)
    : targets_(std::move(targets)),
      donors_(std::move(donors)),
      are_donors_equivalent_from_start_(donors_.size(),
                                        std::deque<bool>(donors_.size())) {
  for (size_t i = 0; i < donors_.size(); ++i) {
    for (size_t j = i; j < donors_.size(); ++j) {
      are_donors_equivalent_from_start_.at(i).at(j) =
          are_donors_equivalent_from_start_.at(j).at(i) =
              donors_.at(i).is_approximately_equal_to(donors_.at(j));
    }
  }
}

bool State::is_worse_than(const State& other) const {
  if (other.donor_events_.empty()) {
    return false;
  }

  if (std::ranges::any_of(
          targets_,
          [&, other_worst = std::get<1>(
                  other.donor_events_.back().lexicographic_objective_value)](
              const auto& t) {
            return t.num_of_connections == max_num_of_target_connections_ &&
                   (t.max_pressure - t.pressure) > other_worst;
          })) {
    return true;
  }
  return false;
}

bool State::is_admissible(const size_t donor_index,
                          const size_t target_index) const {
  // For independent events, enforce an ordering to avoid multiple sequences
  // with equivalent result
  if (!donor_events_.empty()) {
    const auto previous_donor_index = donor_events_.back().donor_index;
    const auto previous_target_index = donor_events_.back().target_index;

    // The sequence of donations [D1 -> T2, D2 -> T1] is equivalent to [D2 ->
    // T1, D1 -> T2]. Only perform the sequence in which the target index is
    // non-decreasing.
    if (donor_index != previous_donor_index &&
        target_index < previous_target_index) {
      return false;
    }
  }

  // If two donors are equal, start with the one with lowest index
  if (donor_index != 0 && donors_.at(donor_index).num_of_connections == 0) {
    for (size_t i = 0; i < donor_index; ++i) {
      if (are_donors_equivalent_from_start_.at(donor_index).at(i) &&
          donors_.at(i).num_of_connections == 0) {
        return false;
      }
    }
  }

  // Avoid too many connections to each target
  const auto& target = targets_.at(target_index);
  const auto& donor = donors_.at(donor_index);
  if (target.num_of_connections == max_num_of_target_connections_) {
    return false;
  }

  // Avoid too many connections from each donor
  if (donor.num_of_connections == max_num_of_donor_connections_) {
    return false;
  }

  const auto unbounded_pressure_after =
      unbounded_pressure_after_(donor, target);

  // Do not overpressurize target
  if (allow_early_stopping_) {
    if (target.pressure >= target.max_pressure + upper_pressure_tolerance_)
      return false;
  } else {
    if (unbounded_pressure_after >
        target.max_pressure + upper_pressure_tolerance_)
      return false;
  }

  // Make sure connection leads to a sufficient improvement
  const auto diff = std::max(target.max_pressure - target.pressure, 0.0);
  if (unbounded_pressure_after <=
      target.pressure + minimum_improvement_fraction_ * diff)
    return false;

  return true;
}

void State::apply(const size_t donor_index, const size_t target_index) {
  const auto& donor = donors_.at(donor_index);
  const auto& target = targets_.at(target_index);
  const auto donor_pressure_before = donor.pressure;
  const auto target_pressure_before = target.pressure;

  const auto unbounded_pressure_after =
      unbounded_pressure_after_(donor, target);
  auto donor_pressure_after = unbounded_pressure_after;
  auto target_pressure_after = unbounded_pressure_after;
  if (allow_early_stopping_ &&
      unbounded_pressure_after >
          target.max_pressure + upper_pressure_tolerance_) {
    target_pressure_after = target.max_pressure + upper_pressure_tolerance_;
    donor_pressure_after =
        (unbounded_pressure_after * (target.volume + donor.volume) -
         target.volume * target_pressure_after) /
        donor.volume;
  }

  auto& donor_var = donors_.at(donor_index);
  auto& target_var = targets_.at(target_index);
  donor_var.pressure = donor_pressure_after;
  donor_var.num_of_connections += 1;
  target_var.pressure = target_pressure_after;
  target_var.num_of_connections += 1;
  const auto objective = lexicographic_objective_();

  donor_events_.emplace_back(donor_index, target_index, donor_pressure_before,
                             donor_pressure_after, target_pressure_before,
                             target_pressure_after, objective,
                             objective_value_(objective));
}

void State::unapply_last_event() {
  assert(!donor_events_.empty());
  const auto& donor_event = donor_events_.back();
  auto& donor = donors_.at(donor_event.donor_index);
  donor.pressure = donor_event.donor_pressure_before;
  assert(donor.num_of_connections > 0);
  donor.num_of_connections -= 1;
  auto& target = targets_.at(donor_event.target_index);
  target.pressure = donor_event.target_pressure_before;
  assert(target.num_of_connections > 0);
  target.num_of_connections -= 1;
  donor_events_.pop_back();
}

double State::objective_value() const {
  if (donor_events_.empty()) {
    return std::numeric_limits<double>::max();
  }
  return donor_events_.back().objective_value;
}

size_t State::num_targets() const { return targets_.size(); }

size_t State::num_donors() const { return donors_.size(); }

void State::print() const {
  const auto [val1, val2, val3, val4] = lexicographic_objective_();
  std::cout << "Objective: " << val1 << ", " << val2 << ", " << val3 << ", "
            << val4 << std::endl;
  for (size_t i = 0; const auto& tubes : {targets_, donors_}) {
    std::cout << (i++ == 0 ? "Targets:" : "Donors:") << std::endl;
    for (size_t j = 1; const auto& t : tubes) {
      t.print(j++);
    }
  }

  if (!donor_events_.empty()) {
    std::cout << std::endl << "Path:" << std::endl;
    for (size_t i = 1; const auto& e : donor_events_) {
      std::cout << i++ << ". D" << e.donor_index + 1 << " to T"
                << e.target_index + 1
                << " (target pressure: " << e.target_pressure_before << " -> "
                << e.target_pressure_after
                << ", donor pressure: " << e.donor_pressure_before << " -> "
                << e.donor_pressure_after << ")" << std::endl;
    }
  }
}

void State::clear_events() { donor_events_.clear(); }

bool State::operator<(const State& other) const {
  return std::tie(targets_, donors_) < std::tie(other.targets_, other.donors_);
}

bool State::operator==(const State& other) const {
  return std::tie(targets_, donors_) == std::tie(other.targets_, other.donors_);
}

size_t State::hash() const {
  size_t res = 17;

  for (const auto& ts : {targets_, donors_}) {
    for (size_t i = 0; const auto& t : ts) {
      res = res * 31 + std::hash<double>{}(t.volume);
      res = res * 31 + std::hash<double>{}(t.pressure);
      res = res * 31 + std::hash<double>{}(t.max_pressure);
      res = res * 31 + std::hash<size_t>{}(t.num_of_connections);
      ++i;
    }
  }
  return res;
}

double State::unbounded_pressure_after_(const Tube& donor,
                                        const Tube& target) const {
  return (target.volume * target.pressure + donor.volume * donor.pressure) /
         (target.volume + donor.volume);
}

double State::objective_value_(ObjectiveValue value) const {
  const auto& [val1, val2, val3, val4] = value;
  return 1e8 * val1 + 1e4 * val2 + 1e2 * val3 + val4;
}

std::tuple<double, double, double, double> State::lexicographic_objective_()
    const {
  // Lexicographic objective:
  // 1. Is the pressure below the max_pressure + upper tolerance for all
  // targets?
  // 2. Is worst pressure difference in targets within lower tolerance?
  // (objective value 0 if true, value of lowest difference if false)
  // 3. Number of connections
  // 4. Sum of pressure differences
  auto worst_diff = 0.0;
  auto sum = 0.0;
  auto all_within_tolerance = true;

  // For performance reason, only one pass over the targets is performed instead
  // of using standard algorithms
  for (const auto& t : targets_) {
    const auto diff = t.max_pressure - t.pressure;
    all_within_tolerance &= diff <= upper_pressure_tolerance_;
    worst_diff = std::max(worst_diff, diff);
    sum += diff;
  }
  const auto val1 = all_within_tolerance ? 0.0 : 1.0;
  const auto val2 = worst_diff <= lower_pressure_tolerance_ ? 0.0 : worst_diff;
  const auto val3 = static_cast<double>(donor_events_.size());
  const auto val4 = sum;

  return {val1, val2, val3, val4};
}
}  // namespace PressureOptimization