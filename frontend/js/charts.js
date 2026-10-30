/**
 * MultiCoreX Charts Configuration & Theming
 */

const MultiCoreCharts = {
  // Common Dark Theme Options for Chart.js
  getDefaultOptions: function(title) {
    return {
      responsive: true,
      maintainAspectRatio: false,
      animation: {
        duration: 600,
        easing: 'easeOutQuart'
      },
      plugins: {
        legend: {
          display: true,
          position: 'top',
          labels: {
            color: '#94a3b8',
            font: { family: 'inherit', size: 12, weight: 600 },
            boxWidth: 12,
            boxHeight: 12,
            padding: 15
          }
        },
        tooltip: {
          backgroundColor: '#151c2c',
          titleColor: '#f1f5f9',
          bodyColor: '#94a3b8',
          borderColor: '#26334d',
          borderWidth: 1,
          padding: 10,
          cornerRadius: 6
        }
      },
      scales: {
        x: {
          grid: { color: 'rgba(38, 51, 77, 0.4)' },
          ticks: { color: '#64748b', font: { family: 'inherit', size: 11 } }
        },
        y: {
          grid: { color: 'rgba(38, 51, 77, 0.4)' },
          ticks: { color: '#64748b', font: { family: 'inherit', size: 11 } }
        }
      }
    };
  }
};
