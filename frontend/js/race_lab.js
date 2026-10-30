/**
 * MultiCoreX - Module 3: Race Condition Laboratory
 */

const RaceLabModule = {
  init: function() {
    const runBtn = document.getElementById('btn-race-run');
    if (runBtn) {
      runBtn.addEventListener('click', () => this.runExperiment());
    }
  },

  runExperiment: async function() {
    const mode = document.getElementById('race-mode').value;
    const threads = parseInt(document.getElementById('race-threads').value, 10);
    const iterations = parseInt(document.getElementById('race-iterations').value, 10);
    const runBtn = document.getElementById('btn-race-run');

    runBtn.disabled = true;
    runBtn.innerHTML = `<span>Simulating Race in C++...</span>`;

    try {
      const response = await fetch('/api/race-condition', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          mode: mode,
          threads: threads,
          iterations: iterations
        })
      });

      if (!response.ok) {
        const err = await response.json();
        throw new Error(err.message || 'Experiment failed');
      }

      const data = await response.json();
      this.renderResults(data);

      if (data.lost_updates > 0) {
        App.showToast(`Data Race detected! Lost ${data.lost_updates.toLocaleString()} updates (${data.loss_percentage.toFixed(1)}%)`, 'error');
      } else {
        App.showToast(`Synchronization successful! 0 updates lost with ${data.synchronization_method}`, 'success');
      }
    } catch (e) {
      App.showToast(`Error running experiment: ${e.message}`, 'error');
    } finally {
      runBtn.disabled = false;
      runBtn.innerHTML = `
        <svg width="18" height="18" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z"/></svg>
        <span>Execute Race Condition Experiment</span>
      `;
    }
  },

  renderResults: function(data) {
    document.getElementById('race-expected-val').textContent = data.expected_value.toLocaleString();
    
    const actualElem = document.getElementById('race-actual-val');
    actualElem.textContent = data.actual_value.toLocaleString();
    if (data.lost_updates > 0) {
      actualElem.className = 'counter-val error';
    } else {
      actualElem.className = 'counter-val success';
    }

    document.getElementById('race-discrepancy-sub').textContent =
      data.lost_updates > 0 ? `Discrepancy: -${data.lost_updates.toLocaleString()} updates` : 'Exact Match (0 Discrepancy)';

    document.getElementById('race-lost-updates').textContent = data.lost_updates.toLocaleString();
    document.getElementById('race-loss-pct').textContent = `${data.loss_percentage.toFixed(1)}%`;
    document.getElementById('race-duration-ms').textContent = data.duration_ms.toFixed(2);

    const methodTag = document.getElementById('race-method-tag');
    if (data.lost_updates > 0) {
      methodTag.className = 'thread-badge state-waiting';
    } else {
      methodTag.className = 'thread-badge state-completed';
    }
    methodTag.textContent = data.mode.toUpperCase();

    document.getElementById('race-explanation').innerHTML = `
      <strong>Observed Behavior (${data.synchronization_method}):</strong><br>
      ${data.explanation}
    `;

    document.getElementById('race-assembly').textContent = data.low_level_assembly_insight;
  }
};
