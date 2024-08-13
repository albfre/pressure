let State, Tube, TubeVector, DonationEvent, DonationEventVector, solve;

Module.onRuntimeInitialized = function() {
    State = Module.State;
    Tube = Module.Tube;
    TubeVector = Module.TubeVector;
    DonationEvent = Module.DonationEvent;
    DonationEventVector = Module.DonationEventVector;
    solve = Module.solve;
    addTube('donor', 12, 232);
    addTube('donor', 12, 232);
    addTube('donor', 10, 300);
    addTube('donor', 10, 300);
    addTube('target', 12, 100, 200);
    addTube('target', 12, 80, 200);
    addTube('target', 8, 70, 300);
    addTube('target', 8, 100, 300);
};

document.addEventListener('DOMContentLoaded', () => {
    const modal = document.getElementById('infoModal');
    const btn = document.getElementById('infoButton');
    const span = document.getElementsByClassName('close')[0];

    btn.onclick = () => {
        modal.style.display = 'block';
    };

    span.onclick = () => {
        modal.style.display = 'none';
    };

    window.onclick = (event) => {
        if (event.target === modal) {
            modal.style.display = 'none';
        }
    };
});

function addTube(type, volume, pressure, maxPressure = 0) {
    const table = document.getElementById(`${type}Inputs`).getElementsByTagName('tbody')[0];
    const rowCount = table.rows.length + 1;
    const maxNumOfTubes = 6;
    
    if (rowCount > maxNumOfTubes) {
        alert(`Cannot add more than ${maxNumOfTubes} ${type} tubes.`);
        return;
    }

    const row = table.insertRow();
    row.innerHTML = `
        <td><span class="tubeName">${type.charAt(0).toUpperCase()}${rowCount}</span></td>
        <td><input type="number" class="volume" value="${volume}"></td>
        <td><input type="number" class="pressure" value="${pressure}"></td>
        ${type === 'target' ? '<td><input type="number" class="maxPressure" value="' + maxPressure + '"></td>' : ''}
        <td><span class="finalPressure">-</span></td>
        <td><button class="remove-tube" onclick="removeTube(this)">×</button></td>
    `;
    clearResults();
}

function removeTube(button) {
    const row = button.closest('tr');
    const table = row.closest('tbody');
    table.removeChild(row);
    updateTubeNumbers(table);
    clearResults();
}

function updateTubeNumbers(table) {
    const rows = table.rows;
    for (let i = 0; i < rows.length; i++) {
        const nameCell = rows[i].querySelector(".tubeName");
        nameCell.textContent = nameCell.textContent.replace(/\d+/, i + 1);
    }
}

function clearResults() {
    clearFinalPressure("donor");
    clearFinalPressure("target");
    document.getElementById('results').classList.add('hidden');
    clearResultsTable();
}

function clearFinalPressure(type) {
    const rows = document.querySelectorAll(`#${type}Inputs tbody tr`);
    rows.forEach(row => {
        row.querySelector(".finalPressure").textContent = "-";
    });
}

function clearResultsTable() {
    document.querySelector('#connectionSequence tbody').innerHTML = '';
}

function solveProblem() {
    clearResults();
    const donors = createTubeVector('donorInputs');
    const targets = createTubeVector('targetInputs');
    
    const initialState = new State(targets, donors);
    const depthLeft = parseInt(document.getElementById('depthLeft').value);
    const maxTests = 1e8;

    // Solve the problem
    const state = solve(initialState, depthLeft, maxTests);
    
    // Display final results
    displayResults(state);
}

function createTubeVector(tableId) {
    const tubeVector = new TubeVector();
    const rows = document.getElementById(tableId).getElementsByTagName('tbody')[0].rows;
    
    for (const row of rows) {
        const tube = new Tube();
        tube.volume = parseFloat(row.querySelector('.volume').value);
        tube.pressure = parseFloat(row.querySelector('.pressure').value);
        if (row.querySelector('.maxPressure')) {
            tube.max_pressure = parseFloat(row.querySelector('.maxPressure').value);
        }
        tubeVector.push_back(tube);
    }
    
    return tubeVector;
}

function displayResults(state) {
    // Display final pressure in target and donor tables
    updateFinalPressureRows("targetInputs", index => state.get_target_pressure(index));
    updateFinalPressureRows("donorInputs", index => state.get_donor_pressure(index));

    // Display list of donation events in results table
    const donationEvents = state.get_donation_events();
    const table = document.querySelector('#connectionSequence tbody');
    for (let i = 0; i < donationEvents.size(); i++) {
        const event = donationEvents.get(i);
        const donorNumber = event.donor_index + 1;
        const targetNumber = event.target_index + 1;
        const donorPressureBefore = event.donor_pressure_before.toFixed(1);
        const donorPressureAfter = event.donor_pressure_after.toFixed(1);
        const targetPressureBefore = event.target_pressure_before.toFixed(1);
        const targetPressureAfter = event.target_pressure_after.toFixed(1);
        const row = table.insertRow();
        row.innerHTML = `
            <td><span class="tubeName">${i + 1}. D${donorNumber} &rarr; T${targetNumber}</span></td>
            <td><span class="tubeName">${donorPressureBefore}</span></td>
            <td><span class="tubeName">${donorPressureAfter}</span></td>
            <td><span class="tubeName">${targetPressureBefore}</span></td>
            <td><span class="tubeName">${targetPressureAfter}</span></td>
        `;
    }
    donationEvents.delete();
    document.getElementById('results').classList.remove('hidden');
}

function updateFinalPressureRows(tableId, getPressureFunction) {
    const rows = document.querySelectorAll(`#${tableId} tbody tr`);
    for (let i = 0; i < rows.length; i++) {
        const row = rows[i];
        const pressure = getPressureFunction(i).toFixed(1);
        const finalPressureSpan = row.querySelector('.finalPressure');
        if (finalPressureSpan) {
            finalPressureSpan.textContent = pressure;
        }
    }
}
