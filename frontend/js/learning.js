/**
 * MultiCoreX - Module 6: Learning Center & Quiz
 */

const LearningModule = {
  score: 0,
  answered: new Set(),

  questions: [
    {
      id: 1,
      question: "Which of the following resources is NOT shared between threads belonging to the same process?",
      options: [
        "Heap Memory",
        "Open File Descriptors",
        "CPU Registers and Call Stack",
        "Global Variables"
      ],
      correctIndex: 2,
      explanation: "Each thread possesses its own private Call Stack and register context (including Program Counter and Stack Pointer) to execute independent function chains."
    },
    {
      id: 2,
      question: "In the C++ ISO Memory Model (since C++11), concurrent unsynchronized access where at least one thread writes causes:",
      options: [
        "A guaranteed deadlock",
        "Undefined Behavior (Data Race)",
        "Automatic fallback to single-core execution",
        "Compile-time syntax error"
      ],
      correctIndex: 1,
      explanation: "A data race produces Undefined Behavior (UB) under ISO C++. Compilers and CPU caches may reorder instructions, tear memory words, or overwrite increments non-deterministically."
    },
    {
      id: 3,
      question: "According to Amdahl's Law, if 20% of an application must execute sequentially (P = 0.8), what is the maximum theoretical speedup with infinite CPU cores?",
      options: [
        "2.5x",
        "5.0x",
        "10.0x",
        "Infinite Speedup"
      ],
      correctIndex: 1,
      explanation: "Speedup = 1 / ((1 - P) + P/N). As N approaches infinity, Speedup = 1 / (1 - 0.8) = 1 / 0.2 = 5.0x."
    },
    {
      id: 4,
      question: "What is the primary advantage of std::atomic over std::mutex for a single shared integer counter?",
      options: [
        "Atomic variables use less RAM memory",
        "Atomic operations use lock-free CPU instructions without OS thread context switches",
        "Atomic variables prevent memory leaks",
        "Atomic variables execute faster on a single thread"
      ],
      correctIndex: 1,
      explanation: "std::atomic translates to hardware CPU instructions (e.g. LOCK XADD / CMPXCHG). They avoid kernel transitions, scheduler blocking, and thread context switches."
    },
    {
      id: 5,
      question: "In a thread pool architecture, what is the primary role of a Condition Variable (std::condition_variable)?",
      options: [
        "To allocate memory for new worker threads",
        "To put idle worker threads to sleep and awaken them when a new task is enqueued",
        "To prevent buffer overflow in network packets",
        "To sort the task queue by priority"
      ],
      correctIndex: 1,
      explanation: "Condition variables allow worker threads to sleep without consuming CPU cycles (0% CPU utilization while waiting) until notified by cv.notify_one() when a task is submitted."
    }
  ],

  init: function() {
    this.renderQuiz();
  },

  renderQuiz: function() {
    const container = document.getElementById('quiz-questions-list');
    if (!container) return;

    container.innerHTML = '';
    document.getElementById('quiz-total').textContent = this.questions.length;
    document.getElementById('quiz-score').textContent = this.score;

    this.questions.forEach((q, qIdx) => {
      const qBox = document.createElement('div');
      qBox.className = 'quiz-question-box';
      qBox.innerHTML = `
        <div class="quiz-question">${qIdx + 1}. ${q.question}</div>
        <div class="quiz-options" id="quiz-opts-${q.id}">
          ${q.options.map((opt, oIdx) => `
            <div class="quiz-option" data-qid="${q.id}" data-oidx="${oIdx}">
              <span style="font-weight:700; color:var(--text-dim);">${String.fromCharCode(65 + oIdx)}.</span>
              <span>${opt}</span>
            </div>
          `).join('')}
        </div>
        <div id="quiz-expl-${q.id}" style="display:none; font-size:0.85rem; color:var(--text-muted); margin-top:0.75rem; padding:0.75rem; background:var(--bg-input); border-radius:var(--radius-sm);"></div>
      `;
      container.appendChild(qBox);
    });

    // Add click listeners
    container.querySelectorAll('.quiz-option').forEach(opt => {
      opt.addEventListener('click', (e) => {
        const target = e.currentTarget;
        const qid = parseInt(target.getAttribute('data-qid'), 10);
        const oidx = parseInt(target.getAttribute('data-oidx'), 10);
        this.selectAnswer(qid, oidx);
      });
    });
  },

  selectAnswer: function(qid, selectedIdx) {
    if (this.answered.has(qid)) return; // already answered
    this.answered.add(qid);

    const q = this.questions.find(item => item.id === qid);
    const optsContainer = document.getElementById(`quiz-opts-${qid}`);
    const explBox = document.getElementById(`quiz-expl-${qid}`);
    const options = optsContainer.querySelectorAll('.quiz-option');

    const isCorrect = selectedIdx === q.correctIndex;
    if (isCorrect) {
      this.score++;
      document.getElementById('quiz-score').textContent = this.score;
      App.showToast('Correct answer! +1 point', 'success');
    } else {
      App.showToast('Incorrect answer. See explanation.', 'error');
    }

    options.forEach((opt, idx) => {
      if (idx === q.correctIndex) {
        opt.className = 'quiz-option correct';
      } else if (idx === selectedIdx && !isCorrect) {
        opt.className = 'quiz-option wrong';
      }
    });

    explBox.style.display = 'block';
    explBox.innerHTML = `
      <strong style="color:${isCorrect ? 'var(--accent-green)' : 'var(--accent-amber)'};">
        ${isCorrect ? 'Correct!' : 'Incorrect.'}
      </strong> ${q.explanation}
    `;
  }
};
