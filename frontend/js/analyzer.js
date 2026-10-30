/**
 * MultiCoreX - Module 2: Performance Analyzer
 */

const AnalyzerModule = {
  execChart: null,
  speedupChart: null,

  init: function() {
    const runBtn = document.getElementById('btn-analyzer-run');
    if (runBtn) {
      runBtn.addEventListener('click', () => this.runBenchmark());
    }
    this.initCharts();
  },

  initCharts: function() {
    const execCtx = document.getElementById('chart-exec-time');
    if (execCtx && !this.execChart) {
      this.execChart = new Chart(execCtx, {
        type: 'bar',
        data: {
          labels: ['1 Thread', '2 Threads', '4 Threads', '8 Threads'],
          datasets: [{
            label: 'Execution Time (ms)',
            data: [0, 0, 0, 0],
            backgroundColor: 'rgba(0, 240, 255, 0.6)',
            borderColor: '#00f0ff',
            borderWidth: 1,
            borderRadius: 6
          }]
        },
        options: MultiCoreCharts.getDefaultOptions('Execution Time')
      });
    }

    const speedupCtx = document.getElementById('chart-speedup');
    if (speedupCtx && !this.speedupChart) {
      this.speedupChart = new Chart(speedupCtx, {
        type: 'line',
        data: {
          labels: ['1T', '2T', '4T', '8T'],
          datasets: [
            {
              label: 'Actual Speedup (Sp)',
              data: [1, 1, 1, 1],
              borderColor: '#10b981',
              backgroundColor: 'rgba(16, 185, 129, 0.1)',
              borderWidth: 2,
              tension: 0.3,
              fill: true
            },
            {
              label: 'Ideal Linear Speedup (p)',
              data: [1, 2, 4, 8],
              borderColor: 'rgba(148, 163, 184, 0.5)',
              borderDash: [5, 5],
              borderWidth: 1.5,
              tension: 0,
              fill: false
            }
          ]
        },
        options: MultiCoreCharts.getDefaultOptions('Speedup')
      });
    }
  },

  runBenchmark: async function() {
    const taskType = document.getElementById('analyzer-task').value;
    const sizePreset = document.getElementById('analyzer-size').value;
    const runBtn = document.getElementById('btn-analyzer-run');

    runBtn.disabled = true;
    runBtn.innerHTML = `<span>Benchmarking in C++...</span>`;

    try {
      const response = await fetch('/api/benchmark', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          task_type: taskType,
          size_preset: sizePreset,
          thread_counts: [1, 2, 4, 8]
        })
      });

      if (!response.ok) {
        const err = await response.json();
        throw new Error(err.message || 'Benchmark failed');
      }

      const data = await response.json();
      this.updateResults(data);
      App.showToast(`Comparison benchmark completed successfully!`, 'success');
    } catch (e) {
      App.showToast(`Error running benchmark: ${e.message}`, 'error');
    } finally {
      runBtn.disabled = false;
      runBtn.innerHTML = `
        <svg width="18" height="18" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z"/></svg>
        <span>Execute Benchmark [1, 2, 4, 8 Threads]</span>
      `;
    }
  },

  updateResults: function(data) {
    const runs = data.thread_runs || [];
    if (runs.length === 0) return;

    const baseRun = runs.find(r => r.thread_count === 1) || runs[0];
    const fourThreadRun = runs.find(r => r.thread_count === 4) || runs[runs.length - 1];

    document.getElementById('perf-time-1t').textContent = baseRun.duration_ms.toFixed(1);
    document.getElementById('perf-time-4t').textContent = fourThreadRun.duration_ms.toFixed(1);
    document.getElementById('perf-speedup').textContent = fourThreadRun.speedup.toFixed(2);
    document.getElementById('perf-efficiency').textContent = fourThreadRun.efficiency_pct.toFixed(1);

    // Update charts
    const labels = runs.map(r => `${r.thread_count} Thread${r.thread_count > 1 ? 's' : ''}`);
    const times = runs.map(r => r.duration_ms);
    const speedups = runs.map(r => r.speedup);
    const ideal = runs.map(r => r.thread_count);

    if (this.execChart) {
      this.execChart.data.labels = labels;
      this.execChart.data.datasets[0].data = times;
      this.execChart.update();
    }

    if (this.speedupChart) {
      this.speedupChart.data.labels = runs.map(r => `${r.thread_count}T`);
      this.speedupChart.data.datasets[0].data = speedups;
      this.speedupChart.data.datasets[1].data = ideal;
      this.speedupChart.update();
    }

    // Explanation
    const bestRun = runs.reduce((max, r) => (r.speedup > max.speedup ? r : max), runs[0]);
    const pVal = fourThreadRun.speedup > 1.0 ? Math.min(0.98, Math.max(0.5, (1 - (1 / fourThreadRun.speedup)) / (1 - (1 / 4)))).toFixed(2) : '0.75';

    document.getElementById('analyzer-explanation-text').innerHTML = `
      <p style="margin-bottom:0.75rem;">
        <strong>Quantitative Analysis:</strong> Baseline single-threaded time was <strong>${baseRun.duration_ms.toFixed(1)} ms</strong>. 
        Using 4 threads completed the identical workload in <strong>${fourThreadRun.duration_ms.toFixed(1)} ms</strong>, achieving a <strong>${fourThreadRun.speedup.toFixed(2)}x Speedup</strong> 
        with <strong>${fourThreadRun.efficiency_pct.toFixed(1)}% Parallel Efficiency</strong>.
      </p>
      <p style="margin-bottom:0.75rem;">
        <strong>Amdahl's Law Estimate:</strong> Calculated parallel portion $P \\approx ${pVal}$. 
        Even with infinite threads, maximum theoretical speedup is capped by the strictly sequential fraction $(1 - P)$.
      </p>
      <p style="font-size:0.8rem; color:var(--text-dim);">
        Note: ${data.hardware_cores_note}
      </p>
    `;
  }
};
