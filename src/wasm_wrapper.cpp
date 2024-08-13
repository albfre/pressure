#include <emscripten/bind.h>

#include <iomanip>
#include <sstream>

#include "solver.h"
#include "state.h"

using namespace emscripten;
using namespace PressureOptimization;

State solve_wrapper(const State& initial_state, size_t depth_left,
                    size_t max_num_of_tests) {
  return Solver::solve(initial_state, depth_left, max_num_of_tests);
}

State solve(const State& initial_state, size_t depth_left,
            size_t max_num_of_tests) {
  return Solver::solve(initial_state, depth_left, max_num_of_tests);
}

EMSCRIPTEN_BINDINGS(pressure_optimization) {
  class_<Tube>("Tube")
      .constructor<>()
      .property("volume", &Tube::volume)
      .property("pressure", &Tube::pressure)
      .property("max_pressure", &Tube::max_pressure);

  register_vector<Tube>("TubeVector");

  class_<DonationEvent>("DonationEvent")
      .constructor<>()
      .property("donor_index", &DonationEvent::donor_index)
      .property("target_index", &DonationEvent::target_index)
      .property("donor_pressure_before", &DonationEvent::donor_pressure_before)
      .property("donor_pressure_after", &DonationEvent::donor_pressure_after)
      .property("target_pressure_before",
                &DonationEvent::target_pressure_before)
      .property("target_pressure_after", &DonationEvent::target_pressure_after);

  register_vector<DonationEvent>("DonationEventVector");

  class_<State>("State")
      .constructor<std::vector<Tube>, std::vector<Tube>>()
      .function("objective_value", &State::objective_value)
      .function("get_donation_events", &State::get_donation_events)
      .function("get_target_pressure", &State::get_target_pressure)
      .function("get_donor_pressure", &State::get_donor_pressure);

  function("solve", &solve);
}