#pragma once

#include <stddef.h>

#include <deque>
#include <iostream>
#include <tuple>
#include <vector>

namespace PressureOptimization {
struct Tube {
  double volume;
  double pressure;
  double max_pressure = 0.0;
  size_t num_of_connections = 0;
  auto operator<=>(const Tube&) const = default;
  bool is_approximately_equal_to(const Tube& other,
                                 const double volume_tolerance = 0.1,
                                 const double pressure_tolerance = 1.0) const {
    return std::abs(volume - other.volume) < volume_tolerance &&
           std::abs(pressure - other.pressure) < pressure_tolerance;
  }

  void print(const size_t i) const {
    std::cout << i << ". (volume, pressure, max pressure): " << volume << ", "
              << pressure << ", " << max_pressure << std::endl;
  }
};
class State {
  using ObjectiveValue = std::tuple<double, double, double, double>;

 private:
  struct DonationEvent_ {
    size_t donor_index;
    size_t target_index;
    double donor_pressure_before;
    double donor_pressure_after;
    double target_pressure_before;
    double target_pressure_after;
    ObjectiveValue lexicographic_objective_value;
    double objective_value;
  };

 public:
  State(std::vector<Tube> targets, std::vector<Tube> donors);
  bool is_worse_than(const State& other) const;
  bool is_admissible(const size_t donor_index, const size_t target_index) const;
  void apply(const size_t donor_index, const size_t target_index);
  void unapply_last_event();
  double objective_value() const;
  size_t num_targets() const;
  size_t num_donors() const;
  void print() const;
  void clear_events();
  bool operator<(const State& other) const;
  bool operator==(const State& other) const;
  size_t hash() const;

 private:
  double unbounded_pressure_after_(const Tube& donor, const Tube& target) const;
  double objective_value_(ObjectiveValue value) const;
  std::tuple<double, double, double, double> lexicographic_objective_() const;

  bool allow_early_stopping_ = false;
  double minimum_improvement_fraction_ = 0.2;
  double upper_pressure_tolerance_ = 1e-6;
  double lower_pressure_tolerance_ = 20.0;
  size_t max_num_of_donor_connections_ = 2;
  size_t max_num_of_target_connections_ = 3;

  std::vector<Tube> targets_;
  std::vector<Tube> donors_;
  std::vector<DonationEvent_> donor_events_;
  std::vector<std::deque<bool>> are_donors_equivalent_from_start_;
};
}  // namespace PressureOptimization

template <>
struct std::hash<PressureOptimization::State> {
  std::size_t operator()(const PressureOptimization::State& k) const {
    return k.hash();
  }
};