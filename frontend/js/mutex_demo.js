/**
 * MultiCoreX - Module 4: Mutex and Critical Section Demonstration
 */

const MutexDemoModule = {
  init: function() {
    const runBtn = document.getElementById('btn-mutex-run');
    if (runBtn) {
      runBtn.addEventListener('click', () => this.runSimulation());
    }
  },

  runSimulation: async function() {
    const threads = parseInt(document.getElementById('mutex-threads').value, 10);
    const iterations = parseInt(document.getElementById('mutex-iterations').value, 10);
    const delay = parseInt(document.getElementById('mutex-delay').value, 10);
    const runBtn = document.getElementById('btn-mutex-run');

    runBtn.disabled = true;
    runBtn.innerHTML = `<span>Simulating Mutex in C++...</span>`;

    try {
      const response = await fetch('/api/mutex-demo', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          threads: threads,
          iterations: iterations,
          delay_ms: delay
        })
      });

      if (!response.ok) {
        const err = await response.json();
        throw new Error(err.message || 'Simulation failed');
      }

      const data = await response.json();
      this.renderSimulation(data);
      App.showToast(`Mutex contention simulation completed!`, 'success');
    } catch (e) {
      App.showToast(`Error running mutex demo: ${e.message}`, 'error');
    } finally {
      runBtn.disabled = false;
      runBtn.innerHTML = `
        <svg width="18" height="18" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z"/></svg>
        <span>Run Mutex Contention Test</span>
      `;
    }
  },

  renderSimulation: function(data) {
    document.getElementById('mutex-total-dur').textContent = data.total_duration_ms.toFixed(1);
    document.getElementById('mutex-avg-wait').textContent = data.avg_wait_time_ms.toFixed(1);
    document.getElementById('mutex-max-wait').textContent = data.max_wait_time_ms.toFixed(1);
    document.getElementById('mutex-completed-count').textContent = data.events.length;
    document.getElementById('mutex-shared-val').textContent = `$${data.final_resource_val}`;

    // Populate events table
    const tbody = document.getElementById('mutex-events-tbody');
    tbody.innerHTML = '';

    data.events.forEach(ev => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>#${ev.event_id}</td>
        <td><strong style="color:var(--accent-cyan);">Thread ${ev.thread_id}</strong></td>
        <td>${ev.request_time_ms.toFixed(1)} ms</td>
        <td>${ev.acquire_time_ms.toFixed(1)} ms</td>
        <td><span style="color:var(--accent-amber); font-weight:600;">+${ev.wait_duration_ms.toFixed(1)} ms</span></td>
        <td>${ev.hold_duration_ms.toFixed(1)} ms</td>
        <td>$${ev.shared_resource_before} &rarr; <strong style="color:var(--accent-green);">$${ev.shared_resource_after}</strong></td>
      `;
      tbody.appendChild(tr);
    });

    // Animate stage briefly
    const chamber = document.getElementById('mutex-chamber');
    const badge = document.getElementById('mutex-lock-badge');
    const activeText = document.getElementById('mutex-active-thread-text');
    const chipsContainer = document.getElementById('mutex-waiting-chips');

    if (data.events.length > 0) {
      const lastEv = data.events[data.events.length - 1];
      activeText.textContent = `Completed All Cycles`;
      badge.className = 'lock-badge unlocked';
      badge.textContent = 'LOCK: RELEASED';
      chamber.className = 'critical-section-zone';
      chipsContainer.innerHTML = `<span class="thread-chip" style="color:var(--accent-green);">All Threads Exited Safely</span>`;
    }
  }
};
