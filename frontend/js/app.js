/**
 * MultiCoreX - Main Application Controller & Router
 */

const App = {
  quickChart: null,

  init: function() {
    this.setupNavigation();
    this.setupQuickBenchmark();
    this.checkHealth();
    this.startTelemetryLoop();

    // Initialize child modules
    VisualizerModule.init();
    AnalyzerModule.init();
    RaceLabModule.init();
    MutexDemoModule.init();
    ThreadPoolModule.init();
    LearningModule.init();
  },

  setupNavigation: function() {
    const navItems = document.querySelectorAll('.nav-item');
    const sections = document.querySelectorAll('.view-section');

    const viewTitles = {
      'view-dashboard': {
        title: 'Dashboard Overview',
        desc: 'Live system metrics, hardware execution statistics, and module navigator.'
      },
      'view-visualizer': {
        title: 'Module 1: Thread Execution Visualizer',
        desc: 'Interactive computational task division across independent C++ worker threads.'
      },
      'view-analyzer': {
        title: 'Module 2: Performance Analyzer',
        desc: 'Single-thread versus multithread speedup, efficiency, and Amdahl\'s Law analysis.'
      },
      'view-race-lab': {
        title: 'Module 3: Race Condition Laboratory',
        desc: 'Demonstrate lost updates under data races vs std::mutex and std::atomic.'
      },
      'view-mutex-demo': {
        title: 'Module 4: Mutex & Critical Section',
        desc: 'Visualize lock acquisition, contention delays, and mutually exclusive execution.'
      },
      'view-thread-pool': {
        title: 'Module 5: Thread Pool & Task Queue',
        desc: 'Reusable C++ worker thread pool with thread-safe task queue and dynamic sizing.'
      },
      'view-learning': {
        title: 'Module 6: OS Concepts & Learning Center',
        desc: 'Foundational Operating System multithreading theory and knowledge-check quiz.'
      },
      'view-about': {
        title: 'About MultiCoreX Project',
        desc: 'Technical architecture specifications, REST API reference, and tech stack.'
      }
    };

    const navigateTo = (targetId) => {
      navItems.forEach(item => {
        if (item.getAttribute('data-target') === targetId) {
          item.classList.add('active');
        } else {
          item.classList.remove('active');
        }
      });

      sections.forEach(sec => {
        if (sec.id === targetId) {
          sec.classList.add('active');
        } else {
          sec.classList.remove('active');
        }
      });

      if (viewTitles[targetId]) {
        document.getElementById('current-view-title').textContent = viewTitles[targetId].title;
        document.getElementById('current-view-desc').textContent = viewTitles[targetId].desc;
      }
    };

    navItems.forEach(item => {
      item.addEventListener('click', (e) => {
        e.preventDefault();
        const targetId = item.getAttribute('data-target');
        navigateTo(targetId);
      });
    });

    // Handle hash in URL if present
    const hash = window.location.hash.replace('#', '');
    if (hash) {
      const match = document.querySelector(`.nav-item[data-target="view-${hash}"]`);
      if (match) {
        navigateTo(`view-${hash}`);
      }
    }
  },

  checkHealth: async function() {
    try {
      const res = await fetch('/api/health');
      if (res.ok) {
        const data = await res.json();
        document.getElementById('server-status-dot').className = 'status-dot';
        document.getElementById('server-status-text').textContent = 'C++ Engine Online';
        document.getElementById('badge-hw-cores').textContent = data.hardware_concurrency || '4';
      } else {
        throw new Error('Offline');
      }
    } catch (e) {
      document.getElementById('server-status-dot').className = 'status-dot offline';
      document.getElementById('server-status-text').textContent = 'C++ Backend Offline';
    }
  },

  startTelemetryLoop: function() {
    const update = async () => {
      try {
        const res = await fetch('/api/dashboard');
        if (res.ok) {
          const data = await res.json();
          document.getElementById('dash-total-tasks').textContent = (data.total_tasks_executed || 0).toLocaleString();
          document.getElementById('dash-active-threads').textContent = data.active_worker_threads || 0;
          document.getElementById('dash-total-workers').textContent = data.total_worker_threads || 4;
          document.getElementById('dash-execution-time').textContent = (data.total_execution_time_ms || 0).toFixed(1);
          document.getElementById('dash-best-speedup').textContent = (data.best_speedup || 1.0).toFixed(2);
          document.getElementById('badge-uptime').textContent = data.server_uptime || '00:00:00';
          if (data.os_info) {
            document.getElementById('dash-env-os').textContent = data.os_info;
          }
        }
      } catch (e) {
        // Silent retry
      }
    };

    update();
    setInterval(update, 2000);
  },

  setupQuickBenchmark: function() {
    const btn = document.getElementById('btn-quick-bench-run');
    const canvas = document.getElementById('dash-quick-chart');

    if (canvas && !this.quickChart) {
      this.quickChart = new Chart(canvas, {
        type: 'bar',
        data: {
          labels: ['1 Thread', '2 Threads', '4 Threads', '8 Threads'],
          datasets: [{
            label: 'Execution Time (ms)',
            data: [0, 0, 0, 0],
            backgroundColor: [
              'rgba(148, 163, 184, 0.4)',
              'rgba(0, 240, 255, 0.6)',
              'rgba(16, 185, 129, 0.6)',
              'rgba(139, 92, 246, 0.6)'
            ],
            borderWidth: 1,
            borderRadius: 6
          }]
        },
        options: MultiCoreCharts.getDefaultOptions('Quick Benchmark')
      });
    }

    if (btn) {
      btn.addEventListener('click', async () => {
        const task = document.getElementById('quick-bench-task').value;
        const size = document.getElementById('quick-bench-size').value;

        btn.disabled = true;
        btn.innerHTML = `<span>Running in C++...</span>`;

        try {
          const res = await fetch('/api/benchmark', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              task_type: task,
              size_preset: size,
              thread_counts: [1, 2, 4, 8]
            })
          });

          if (!res.ok) throw new Error('Benchmark failed');
          const data = await res.json();

          if (this.quickChart && data.thread_runs) {
            this.quickChart.data.labels = data.thread_runs.map(r => `${r.thread_count}T`);
            this.quickChart.data.datasets[0].data = data.thread_runs.map(r => r.duration_ms);
            this.quickChart.update();
          }

          App.showToast('Quick benchmark executed successfully!', 'success');
        } catch (e) {
          App.showToast(`Error: ${e.message}`, 'error');
        } finally {
          btn.disabled = false;
          btn.innerHTML = `
            <svg width="18" height="18" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z"/></svg>
            <span>Run Quick Comparison</span>
          `;
        }
      });
    }
  },

  showToast: function(message, type = 'info') {
    const container = document.getElementById('toast-container');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.textContent = message;
    container.appendChild(toast);

    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateY(10px)';
      toast.style.transition = 'all 0.3s ease';
      setTimeout(() => toast.remove(), 300);
    }, 3500);
  }
};

document.addEventListener('DOMContentLoaded', () => {
  App.init();
});
