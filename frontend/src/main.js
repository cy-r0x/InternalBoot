import './style.css';
import './app.css';

import {
    SelectISO,
    ValidateISO,
    GetPartitions,
    StartInstallation,
    GetProgress,
    Reboot,
    Cleanup,
} from '../wailsjs/go/main/App';

// State
let state = {
    step: 1,
    isoPath: '',
    isoInfo: null,
    partitions: [],
    selectedPartition: null,
    installRequest: null,
    progressInterval: null,
};

// DOM refs
const app = document.getElementById('app');

// Render the wizard shell
function renderWizard() {
    app.innerHTML = `
        <div class="wizard">
            <div class="header">
                <h1>InternalBoot</h1>
                <p>USB-free Windows installation</p>
            </div>
            <div class="steps">
                <div class="step ${state.step === 1 ? 'active' : state.step > 1 ? 'completed' : ''}" id="step-ind-1">
                    <div class="step-number">1</div>
                    <span>Select ISO</span>
                </div>
                <div class="step ${state.step === 2 ? 'active' : state.step > 2 ? 'completed' : ''}" id="step-ind-2">
                    <div class="step-number">2</div>
                    <span>Partition</span>
                </div>
                <div class="step ${state.step === 3 ? 'active' : state.step > 3 ? 'completed' : ''}" id="step-ind-3">
                    <div class="step-number">3</div>
                    <span>Review</span>
                </div>
                <div class="step ${state.step === 4 ? 'active' : state.step > 4 ? 'completed' : ''}" id="step-ind-4">
                    <div class="step-number">4</div>
                    <span>Install</span>
                </div>
                <div class="step ${state.step === 5 ? 'active' : ''}" id="step-ind-5">
                    <div class="step-number">5</div>
                    <span>Finish</span>
                </div>
            </div>
            <div class="panel" id="panel"></div>
        </div>
    `;
    renderStep();
}

function renderStep() {
    const panel = document.getElementById('panel');
    panel.innerHTML = '';

    switch (state.step) {
        case 1:
            renderStep1(panel);
            break;
        case 2:
            renderStep2(panel);
            break;
        case 3:
            renderStep3(panel);
            break;
        case 4:
            renderStep4(panel);
            break;
        case 5:
            renderStep5(panel);
            break;
    }
}

// Step 1: Select ISO
function renderStep1(panel) {
    panel.innerHTML = `
        <h2>Select Windows ISO</h2>
        <p>Choose a Windows installation ISO file. It will be extracted to an internal partition and configured for boot.</p>
        <div class="iso-picker">
            <div class="iso-path ${state.isoPath ? 'selected' : ''}" id="iso-path">
                ${state.isoPath ? state.isoPath : 'No ISO selected — click Browse to choose'}
            </div>
            <button class="btn btn-primary" onclick="browseISO()">Browse for ISO</button>
            ${state.isoInfo?.error ? `<div class="error-box"><h3>Validation Failed</h3><p>${state.isoInfo.error}</p></div>` : ''}
            <div id="iso-info"></div>
        </div>
        <div class="actions">
            <div></div>
            <div class="actions-right">
                <button class="btn btn-primary" id="btn-next-1" onclick="goToStep(2)" ${!state.isoInfo?.isValid ? 'disabled' : ''}>Next</button>
            </div>
        </div>
    `;
}

window.browseISO = async function () {
    try {
        const path = await SelectISO();
        if (!path) return;
        state.isoPath = path;
        renderWizard();
        // Validate
        const info = await ValidateISO(path);
        state.isoInfo = info;
        renderWizard();
    } catch (err) {
        console.error(err);
        state.isoInfo = { isValid: false, error: err.toString() };
        renderWizard();
    }
};

// Step 2: Select Partition
function renderStep2(panel) {
    panel.innerHTML = `
        <h2>Select Target Partition</h2>
        <p>Choose a partition with enough free space. The ISO contents will be extracted here and a boot entry will be created.</p>
        <div class="partition-list" id="partition-list">
            <p style="text-align:center;color:#64748b;padding:20px;">Loading partitions...</p>
        </div>
        <div class="actions">
            <button class="btn btn-secondary" onclick="goToStep(1)">Back</button>
            <div class="actions-right">
                <button class="btn btn-primary" id="btn-next-2" onclick="goToStep(3)" ${!state.selectedPartition ? 'disabled' : ''}>Next</button>
            </div>
        </div>
    `;

    loadPartitions();
}

async function loadPartitions() {
    try {
        const parts = await GetPartitions();
        state.partitions = parts || [];
    } catch (err) {
        console.error(err);
        state.partitions = [];
    }
    renderPartitionList();
}

function renderPartitionList() {
    const list = document.getElementById('partition-list');
    if (!list) return;

    if (state.partitions.length === 0) {
        list.innerHTML = `
            <div class="error-box">
                <h3>Could not load partitions</h3>
                <p>Partition enumeration is only available on Windows.</p>
            </div>
            <div style="margin-top:12px;">
                <label style="font-size:0.8rem;color:#94a3b8;">Target Drive Letter:</label>
                <input type="text" id="manual-drive" maxlength="1" placeholder="D" style="margin-top:6px;width:60px;padding:8px 12px;background:#0f172a;border:1px solid #334155;border-radius:6px;color:#e2e8f0;font-size:0.875rem;text-transform:uppercase;">
            </div>
        `;
        const input = document.getElementById('manual-drive');
        if (input) {
            input.addEventListener('input', (e) => {
                const val = e.target.value.toUpperCase();
                if (/^[A-Z]$/.test(val)) {
                    state.selectedPartition = { driveLetter: val, label: 'Manual', sizeGB: 'Unknown' };
                    document.getElementById('btn-next-2').disabled = false;
                } else {
                    state.selectedPartition = null;
                    document.getElementById('btn-next-2').disabled = true;
                }
            });
        }
        return;
    }

    list.innerHTML = state.partitions.map((p, i) => `
        <div class="partition-item ${state.selectedPartition?.driveLetter === p.driveLetter ? 'selected' : ''}" onclick="selectPartition(${i})">
            <input type="radio" name="partition" ${state.selectedPartition?.driveLetter === p.driveLetter ? 'checked' : ''}>
            <div class="partition-details">
                <div class="drive">${p.driveLetter}: ${p.label || 'Local Disk'}</div>
                <div class="meta">${p.fileSystem || 'NTFS'} · ${p.sizeGB || 'Unknown size'}</div>
            </div>
        </div>
    `).join('');
}

window.selectPartition = function (index) {
    state.selectedPartition = state.partitions[index];
    renderPartitionList();
    const btn = document.getElementById('btn-next-2');
    if (btn) btn.disabled = false;
};

// Step 3: Review
function renderStep3(panel) {
    panel.innerHTML = `
        <h2>Review & Confirm</h2>
        <p>Please review the details before starting the installation.</p>
        <table class="review-table">
            <tr><td>ISO File</td><td>${state.isoInfo?.path || state.isoPath}</td></tr>
            <tr><td>ISO Size</td><td>${state.isoInfo?.sizeGB || 'Unknown'}</td></tr>
            <tr><td>Target Drive</td><td>${state.selectedPartition?.driveLetter || '?'}</td></tr>
            <tr><td>Target Label</td><td>${state.selectedPartition?.label || 'Local Disk'}</td></tr>
            <tr><td>Extract Folder</td><td>InternalBoot_Installer</td></tr>
            <tr><td>Boot Entry</td><td>InternalBoot Installer</td></tr>
        </table>
        <div class="actions">
            <button class="btn btn-secondary" onclick="goToStep(2)">Back</button>
            <div class="actions-right">
                <button class="btn btn-success" onclick="startInstall()">Start Installation</button>
            </div>
        </div>
    `;
}

window.startInstall = async function () {
    state.installRequest = {
        isoPath: state.isoPath,
        driveLetter: state.selectedPartition.driveLetter,
        targetPath: 'InternalBoot_Installer',
        setAsDefault: false,
    };
    goToStep(4);
    try {
        await StartInstallation(state.installRequest);
    } catch (err) {
        console.error(err);
    }
};

// Step 4: Progress
function renderStep4(panel) {
    panel.innerHTML = `
        <div class="progress-container">
            <div class="progress-ring">
                <svg width="140" height="140" viewBox="0 0 140 140">
                    <circle class="bg" cx="70" cy="70" r="60"></circle>
                    <circle class="fg" id="progress-circle" cx="70" cy="70" r="60" 
                        stroke-dasharray="377" stroke-dashoffset="377"></circle>
                </svg>
                <div class="progress-text" id="progress-percent">0%</div>
            </div>
            <div class="progress-stage" id="progress-stage">Preparing</div>
            <div class="progress-message" id="progress-message">Initializing installation...</div>
        </div>
    `;

    // Start polling progress
    if (state.progressInterval) clearInterval(state.progressInterval);
    state.progressInterval = setInterval(async () => {
        try {
            const prog = await GetProgress();
            updateProgress(prog);
            if (!prog.isRunning && (prog.canReboot || prog.hasError)) {
                clearInterval(state.progressInterval);
                state.progressInterval = null;
                if (prog.canReboot) {
                    setTimeout(() => goToStep(5), 800);
                }
            }
        } catch (err) {
            console.error(err);
        }
    }, 500);
}

function updateProgress(prog) {
    const circle = document.getElementById('progress-circle');
    const percentText = document.getElementById('progress-percent');
    const stageText = document.getElementById('progress-stage');
    const msgText = document.getElementById('progress-message');

    if (!circle || !percentText) return;

    const circumference = 2 * Math.PI * 60; // ~377
    const offset = circumference - (prog.percent / 100) * circumference;
    circle.style.strokeDashoffset = offset;
    percentText.textContent = prog.percent + '%';
    if (stageText) stageText.textContent = prog.stage;
    if (msgText) msgText.textContent = prog.message;

    if (prog.hasError) {
        circle.style.stroke = '#ef4444';
        if (stageText) stageText.textContent = 'Error';
        if (msgText) msgText.textContent = prog.errorMsg;
    }
}

// Step 5: Complete
function renderStep5(panel) {
    panel.innerHTML = `
        <div class="success-box">
            <div class="success-icon">✓</div>
            <h2>Installation Ready</h2>
            <p>Your system is configured to boot into the Windows installer. Click Reboot to restart now, or you can reboot manually later.</p>
            <div style="display:flex;gap:12px;margin-top:8px;">
                <button class="btn btn-success" onclick="doReboot()">Reboot Now</button>
                <button class="btn btn-secondary" onclick="doCleanup()">Remove Boot Entry</button>
            </div>
        </div>
    `;
}

window.doReboot = async function () {
    try {
        await Reboot();
    } catch (err) {
        console.error(err);
        alert('Reboot failed: ' + err);
    }
};

window.doCleanup = async function () {
    try {
        await Cleanup(state.selectedPartition?.driveLetter || '', 'InternalBoot_Installer');
        alert('Boot entry removed successfully.');
    } catch (err) {
        console.error(err);
        alert('Cleanup failed: ' + err);
    }
};

// Navigation
window.goToStep = function (step) {
    state.step = step;
    renderWizard();
};

// Initialize
renderWizard();
