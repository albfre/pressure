importScripts('pressure_optimization_wasm.js');

self.onmessage = function(e) {
    if (e.data.type === 'solve') {
        const { donorData, targetData, depthLeft, maxTests } = e.data;

        const startTime = performance.now();

        const updateStatus = (numTests, worstObjective, averageObjective) => {
            self.postMessage({ 
                type: 'update', 
                numTests, 
                worstObjective, 
                averageObjective,
                elapsedTime: self.performance.now() - startTime
            });
        };

        const donors = createTubeVector(donorData);
        const targets = createTubeVector(targetData);

        const initialState = new Module.State(donors, targets);

        const finalState = Module.solve(initialState, depthLeft, maxTests, updateStatus);

        const finalDonorPressures = [];
        for (let i = 0; i < finalState.num_donors(); ++i) {
            finalDonorPressures.push(finalState.get_donor_pressure(i));
        }
        const finalTargetPressures = [];
        for (let i = 0; i < finalState.num_targets(); ++i) {
            finalTargetPressures.push(finalState.get_target_pressure(i));
        }

        const donationEventData = createDonationEventData(finalState.get_donation_events());

        self.postMessage({ type: 'result', finalDonorPressures, finalTargetPressures, donationEventData });
    }
};

function createTubeVector(tubeData) {
    const tubeVector = new Module.TubeVector();
    for (const {volume, pressure, maxPressure} of tubeData) {
        const tube = new Module.Tube();
        tube.volume = volume,
        tube.pressure = pressure;
        tube.max_pressure = maxPressure;
        tubeVector.push_back(tube);
    }
    return tubeVector;
}

function createDonationEventData(donationEvents)
{
    const data = [];
    for (let i = 0; i < donationEvents.size(); i++) {
        const event = donationEvents.get(i);
        const donorNumber = event.donor_index + 1;
        const targetNumber = event.target_index + 1;
        const donorPressureBefore = event.donor_pressure_before;
        const donorPressureAfter = event.donor_pressure_after;
        const targetPressureBefore = event.target_pressure_before;
        const targetPressureAfter = event.target_pressure_after;
        data.push({donorNumber, targetNumber, donorPressureBefore, donorPressureAfter, targetPressureBefore, targetPressureAfter});
    }
    return data;
}
