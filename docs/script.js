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
    document.getElementById('results').classList.remove('hidden');

    const donorData = getTubeData('donorInputs');
    const targetData = getTubeData('targetInputs');
    const depthLeft = parseInt(document.getElementById('depthLeft').value);
    const maxTests = 1e7;

    worker.postMessage({
        type: 'solve',
        donorData,
        targetData,
        depthLeft,
        maxTests
    });
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

function getTubeData(tableId) {
    const tubeData = [];
    const rows = document.getElementById(tableId).getElementsByTagName('tbody')[0].rows;
    for (const row of rows) {
        const volume = parseFloat(row.querySelector('.volume').value);
        const pressure = parseFloat(row.querySelector('.pressure').value);
        const maxPressure = row.querySelector('.maxPressure') ? parseFloat(row.querySelector('.maxPressure').value) : 0.0;
        tubeData.push({volume, pressure, maxPressure});
    }
    return tubeData;
}

function initializeWorker() {
    worker = new Worker('worker.js');
    
    worker.onmessage = function(e) {
        if (e.data.type === 'ready') {
            console.log('Worker is ready');
        } else if (e.data.type === 'update') {
            updateUI(e.data);
        } else if (e.data.type === 'result') {
            displayEventData(e.data);
        }
    };
}

function updateUI({ numTests, worstObjective, averageObjective, elapsedTime }) {
    requestAnimationFrame(() => {
        document.getElementsByClassName('elapsed-time')[0].textContent = (elapsedTime / 1000).toFixed(3);
        document.getElementsByClassName('num-tests')[0].textContent = numTests;
        document.getElementsByClassName('worst-objective')[0].textContent = worstObjective.toFixed(1);
        document.getElementsByClassName('average-objective')[0].textContent = averageObjective.toFixed(1);
    });
}

function displayEventData({finalDonorPressures, finalTargetPressures, donationEventData}) {
    // Display final pressure in target and donor tables
    updateFinalPressureRows("donorInputs", index => finalDonorPressures[index]);
    updateFinalPressureRows("targetInputs", index => finalTargetPressures[index]);

    // Display list of donation events in results table
    const table = document.querySelector('#connectionSequence tbody');
    for (let i = 0; i < donationEventData.length; i++) {
        const {
            donorNumber,
            targetNumber,
            donorPressureBefore,
            donorPressureAfter,
            targetPressureBefore,
            targetPressureAfter
        } = donationEventData[i];
        const row = table.insertRow();
        row.innerHTML = `
            <td><span class="tubeName">${i + 1}. D${donorNumber} &rarr; T${targetNumber}</span></td>
            <td><span class="tubeName">${donorPressureBefore.toFixed(1)}</span></td>
            <td><span class="tubeName">${donorPressureAfter.toFixed(1)}</span></td>
            <td><span class="tubeName">${targetPressureBefore.toFixed(1)}</span></td>
            <td><span class="tubeName">${targetPressureAfter.toFixed(1)}</span></td>
        `;
    }
}


initializeWorker()
addTube('donor', 12, 232);
addTube('donor', 12, 232);
addTube('donor', 10, 300);
addTube('donor', 10, 300);
addTube('donor', 12, 232);
addTube('donor', 12, 232);

addTube('target', 12, 100, 200);
addTube('target', 12, 80, 200);
addTube('target', 8, 70, 300);
addTube('target', 8, 100, 300);
addTube('target', 10, 100, 200);
addTube('target', 10, 100, 200);
