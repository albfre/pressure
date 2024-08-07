#pragma once

#include <stddef.h>

#include <iostream>
#include <tuple>
#include <vector>

namespace PressureOptimization {
class State {
  using ObjectiveValue = std::tuple<double, double, double, double>;

  struct Tube_ {
    double volume;
    double pressure;
    double max_pressure;
    size_t num_of_connections;
    auto operator<=>(const Tube_&) const = default;

    void print(const size_t i) const {
      std::cout << i << ". (volume, pressure, max pressure): " << volume << ", "
                << pressure << ", " << max_pressure << std::endl;
    }
  };

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
  void add_target(const double volume, const double pressure,
                  const double max_pressure);
  void add_donor(const double volume, const double pressure);
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
  double unbounded_pressure_after_(const Tube_& donor,
                                   const Tube_& target) const;
  double objective_value_(ObjectiveValue value) const;
  std::tuple<double, double, double, double> lexicographic_objective_() const;

  bool allow_early_stopping_ = false;
  double minimum_improvement_fraction_ = 0.2;
  double upper_pressure_tolerance_ = 1e-6;
  double lower_pressure_tolerance_ = 20.0;
  size_t max_num_of_donor_connections_ = 2;
  size_t max_num_of_target_connections_ = 3;

  std::vector<Tube_> targets_;
  std::vector<Tube_> donors_;
  std::vector<DonationEvent_> donor_events_;
};
}  // namespace PressureOptimization

template <>
struct std::hash<PressureOptimization::State> {
  std::size_t operator()(const PressureOptimization::State& k) const {
    return k.hash();
  }
};