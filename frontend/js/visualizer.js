/**
 * MultiCoreX - Module 1: Thread Execution Visualizer
 */

const VisualizerModule = {
  init: function() {
    const runBtn = document.getElementById('btn-vis-run');
    const resetBtn = document.getElementById('btn-vis-reset');

    if (runBtn) {
      runBtn.addEventListener('click', () => this.runWorkload());
    }
    if (resetBtn) {
      resetBtn.addEventListener('click', () => this.resetLanes());
    }
  },

  resetLanes: function() {
    const container = document.getElementById('vis-lanes-container');
    container.innerHTML = `
      <div style="text-align:center; padding:2rem; color:var(--text-dim);">
        No task running. Select options and click "Execute Task".
      </div>`;
    document.getElementById('vis-time-ms').textContent = '0.0';
    document.getElementById('vis-elements-count').textContent = '0';
    document.getElementById('vis-correctness-status').textContent = '-';
    document.getElementById('vis-summary-text').textContent =
      'Configure task parameters and press "Execute Task" to visualize thread distribution in real C++.';
    
    const chip = document.getElementById('vis-status-chip');
    chip.className = 'thread-badge state-waiting';
    chip.textContent = 'Ready';
  },

  runWorkload: async function() {
    const taskType = document.getElementById('vis-task-type').value;
    const threads = parseInt(document.getElementById('vis-threads').value, 10);
    const sizePreset = document.getElementById('vis-input-size').value;
    const runBtn = document.getElementById('btn-vis-run');
    const chip = document.getElementById('vis-status-chip');
    const container = document.getElementById('vis-lanes-container');

    runBtn.disabled = true;
    chip.className = 'thread-badge state-running';
    chip.textContent = 'Running in C++...';

    // 1. Initial visual setup: Show lanes in "Created" state
    container.innerHTML = '';
    for (let i = 0; i < threads; i++) {
      const lane = document.createElement('div');
      lane.id = `vis-lane-${i}`;
      lane.className = 'thread-lane';
      lane.innerHTML = `
        <div class="thread-lane-header">
          <div class="thread-title">
            <svg width="18" height="18" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z"/></svg>
            <span>Worker Thread #${i}</span>
          </div>
          <span class="thread-badge state-created" id="vis-lane-badge-${i}">Created</span>
        </div>
        <div class="timeline-bar-track">
          <div class="timeline-bar-chunk" id="vis-lane-chunk-${i}" style="width: 0%;"></div>
        </div>
        <div class="thread-lane-footer">
          <span id="vis-lane-info-${i}">Waiting for OS thread dispatch...</span>
          <span id="vis-lane-dur-${i}">0.0 ms</span>
        </div>
      `;
      container.appendChild(lane);
    }

    // Brief simulation step to visualize "Waiting" state
    await new Promise(r => setTimeout(r, 80));
    for (let i = 0; i < threads; i++) {
      const b = document.getElementById(`vis-lane-badge-${i}`);
      if (b) {
        b.className = 'thread-badge state-waiting';
        b.textContent = 'Waiting';
      }
    }

    try {
      const startTime = performance.now();
      const response = await fetch('/api/visualizer/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          task_type: taskType,
          threads: threads,
          size_preset: sizePreset
        })
      });

      if (!response.ok) {
        const err = await response.json();
        throw new Error(err.message || 'Execution failed');
      }

      const data = await response.json();

      // Update UI with real C++ execution data
      document.getElementById('vis-time-ms').textContent = data.duration_ms.toFixed(2);
      document.getElementById('vis-elements-count').textContent = data.input_size.toLocaleString();
      document.getElementById('vis-correctness-status').textContent = data.is_correct ? 'Verified OK' : 'Discrepancy';
      document.getElementById('vis-summary-text').textContent = data.result_summary;

      chip.className = 'thread-badge state-completed';
      chip.textContent = 'Completed';

      // Animate chunk bars with real calculated start offset and duration
      const maxDuration = Math.max(0.1, data.duration_ms);
      data.chunks.forEach((chunk, idx) => {
        const lane = document.getElementById(`vis-lane-${chunk.thread_id}`);
        const badge = document.getElementById(`vis-lane-badge-${chunk.thread_id}`);
        const bar = document.getElementById(`vis-lane-chunk-${chunk.thread_id}`);
        const info = document.getElementById(`vis-lane-info-${chunk.thread_id}`);
        const dur = document.getElementById(`vis-lane-dur-${chunk.thread_id}`);

        if (lane && badge && bar) {
          badge.className = 'thread-badge state-completed';
          badge.textContent = 'Completed';

          const pct = Math.min(100, Math.max(12, (chunk.duration_ms / maxDuration) * 100));
          bar.style.width = `${pct}%`;
          bar.textContent = `${chunk.duration_ms.toFixed(1)} ms`;

          info.textContent = `TID: 0x${chunk.native_thread_id.toString(16)} | Processed: ${chunk.items_processed.toLocaleString()} items | ${chunk.partial_result}`;
          dur.textContent = `${chunk.duration_ms.toFixed(2)} ms`;
        }
      });

      App.showToast(`Completed ${data.task_type} across ${threads} threads in ${data.duration_ms.toFixed(2)} ms!`, 'success');
    } catch (e) {
      chip.className = 'thread-badge state-created';
      chip.textContent = 'Error';
      App.showToast(`Error running visualizer: ${e.message}`, 'error');
    } finally {
      runBtn.disabled = false;
    }
  }
};
