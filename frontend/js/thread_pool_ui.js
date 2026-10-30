/**
 * MultiCoreX - Module 5: Thread Pool Manager
 */

const ThreadPoolModule = {
  pollInterval: null,

  init: function() {
    const submitBtn = document.getElementById('btn-pool-submit');
    const resizeBtn = document.getElementById('btn-pool-resize');
    const resetBtn = document.getElementById('btn-pool-reset');
    const slider = document.getElementById('pool-size-slider');

    if (slider) {
      slider.addEventListener('input', (e) => {
        document.getElementById('pool-workers-label').textContent = e.target.value;
      });
    }

    if (submitBtn) {
      submitBtn.addEventListener('click', () => this.submitTask());
    }

    if (resizeBtn) {
      resizeBtn.addEventListener('click', () => this.resizePool());
    }

    if (resetBtn) {
      resetBtn.addEventListener('click', () => this.resetPool());
    }

    this.fetchStatus();
    // Poll pool status every 2 seconds when in view
    this.pollInterval = setInterval(() => {
      const poolView = document.getElementById('view-thread-pool');
      if (poolView && poolView.classList.contains('active')) {
        this.fetchStatus();
      }
    }, 1500);
  },

  fetchStatus: async function() {
    try {
      const res = await fetch('/api/pool/status');
      if (!res.ok) return;
      const data = await res.json();
      this.renderStatus(data);
    } catch (e) {
      // Backend maybe restarting or offline
    }
  },

  submitTask: async function() {
    const type = document.getElementById('pool-task-type').value;
    const count = parseInt(document.getElementById('pool-batch-size').value, 10);
    const priority = parseInt(document.getElementById('pool-task-priority').value, 10);
    const btn = document.getElementById('btn-pool-submit');

    btn.disabled = true;
    try {
      const res = await fetch('/api/pool/submit', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          name: `Job (${type})`,
          type: type,
          count: count,
          priority: priority,
          duration_target_ms: 100
        })
      });

      if (!res.ok) {
        const err = await res.json();
        throw new Error(err.message || 'Submission failed');
      }

      const data = await res.json();
      App.showToast(`Submitted ${data.submitted_count} task(s) to worker pool!`, 'success');
      this.fetchStatus();
    } catch (e) {
      App.showToast(`Error submitting task: ${e.message}`, 'error');
    } finally {
      btn.disabled = false;
    }
  },

  resizePool: async function() {
    const workers = parseInt(document.getElementById('pool-size-slider').value, 10);
    try {
      const res = await fetch('/api/pool/resize', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ workers: workers })
      });
      if (res.ok) {
        App.showToast(`Thread pool capacity updated to ${workers} worker threads.`, 'success');
        this.fetchStatus();
      }
    } catch (e) {
      App.showToast(`Error resizing pool: ${e.message}`, 'error');
    }
  },

  resetPool: async function() {
    try {
      const res = await fetch('/api/pool/reset', { method: 'POST' });
      if (res.ok) {
        App.showToast(`Thread pool history reset.`, 'success');
        this.fetchStatus();
      }
    } catch (e) {
      App.showToast(`Error resetting pool: ${e.message}`, 'error');
    }
  },

  renderStatus: function(data) {
    document.getElementById('pool-queue-size').textContent = data.queue_size || 0;
    document.getElementById('pool-busy-workers').textContent = `${data.busy_workers || 0} / ${data.worker_count || 4}`;
    document.getElementById('pool-completed-tasks').textContent = data.completed_tasks || 0;

    // Render worker cards
    const container = document.getElementById('pool-workers-container');
    container.innerHTML = '';

    (data.workers || []).forEach(w => {
      const card = document.createElement('div');
      card.className = `worker-card ${w.is_busy ? 'busy' : ''}`;
      card.innerHTML = `
        <div class="worker-top">
          <span class="worker-name">Worker #${w.worker_id}</span>
          <span class="worker-status-badge ${w.is_busy ? 'busy' : 'idle'}">${w.is_busy ? 'BUSY' : 'IDLE'}</span>
        </div>
        <div class="worker-current-task">
          ${w.is_busy ? `Processing: <strong>${w.current_task_name}</strong>` : 'Awaiting tasks from queue...'}
        </div>
        <div style="font-size:0.75rem; color:var(--text-dim); display:flex; justify-content:space-between; margin-top:0.25rem;">
          <span>Tasks: ${w.tasks_completed}</span>
          <span>${w.total_busy_time_ms.toFixed(0)} ms busy</span>
        </div>
      `;
      container.appendChild(card);
    });

    // Render task table
    const tbody = document.getElementById('pool-tasks-tbody');
    const tasks = data.recent_tasks || [];
    if (tasks.length === 0) {
      tbody.innerHTML = `<tr><td colspan="8" style="text-align:center; color:var(--text-dim);">No tasks processed yet. Submit tasks above.</td></tr>`;
      return;
    }

    tbody.innerHTML = '';
    // Reverse order to show newest first
    [...tasks].reverse().forEach(t => {
      const tr = document.createElement('tr');
      let statusClass = 'state-waiting';
      if (t.status === 'RUNNING') statusClass = 'state-running';
      if (t.status === 'COMPLETED') statusClass = 'state-completed';
      if (t.status === 'FAILED') statusClass = 'state-created';

      tr.innerHTML = `
        <td>#${t.id}</td>
        <td><strong>${t.name}</strong></td>
        <td>${t.type}</td>
        <td>${t.priority > 0 ? `<span style="color:var(--accent-amber); font-weight:700;">+${t.priority}</span>` : '0'}</td>
        <td>${t.worker_id >= 0 ? `Worker #${t.worker_id}` : '-'}</td>
        <td><span class="thread-badge ${statusClass}">${t.status}</span></td>
        <td>${t.duration_ms > 0 ? `${t.duration_ms.toFixed(1)} ms` : '-'}</td>
        <td style="font-size:0.8rem; color:var(--text-dim);">${t.summary || '-'}</td>
      `;
      tbody.appendChild(tr);
    });
  }
};
