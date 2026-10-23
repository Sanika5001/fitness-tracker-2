// FitTrack Frontend Core Logic
document.addEventListener('DOMContentLoaded', () => {
  let currentUser = null;
  let activeView = 'dashboard';
  let activeLogType = 'cardio';

  // API helper with authentication and 401 redirect
  async function api(path, options = {}) {
    const token = localStorage.getItem('fittrack_token');
    if (!token) {
      window.location.href = 'login.html';
      return null;
    }
    options.headers = options.headers || {};
    options.headers['Authorization'] = 'Bearer ' + token;

    if (options.body && typeof options.body === 'object' && !(options.body instanceof FormData)) {
      options.headers['Content-Type'] = 'application/json';
      options.body = JSON.stringify(options.body);
    }

    try {
      const res = await fetch(path, options);
      if (res.status === 401) {
        localStorage.removeItem('fittrack_token');
        window.location.href = 'login.html';
        return null;
      }
      return res;
    } catch (err) {
      return null;
    }
  }

  // Helper to format date strings
  function getTodayDateString() {
    const d = new Date();
    const year = d.getFullYear();
    const month = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
  }

  function getLongDateString() {
    const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
    return new Date().toLocaleDateString('en-US', options);
  }

  function getTimeGreeting() {
    const hour = new Date().getHours();
    if (hour < 12) return 'Good morning';
    if (hour < 18) return 'Good afternoon';
    return 'Good evening';
  }

  // Elements
  const topUsername = document.getElementById('topUsername');
  const topWeight = document.getElementById('topWeight');
  const userAvatar = document.getElementById('userAvatar');
  const headerTitle = document.getElementById('headerTitle');
  const headerSubtitle = document.getElementById('headerSubtitle');
  const headerActionBtn = document.getElementById('headerActionBtn');
  const navTabs = document.getElementById('navTabs');
  const mobileMenuBtn = document.getElementById('mobileMenuBtn');
  const logoutBtn = document.getElementById('logoutBtn');
  const mobileLogoutBtn = document.getElementById('mobileLogoutBtn');

  // Navigation tab switching
  function switchView(viewName) {
    activeView = viewName;

    // Update active nav tab
    document.querySelectorAll('.nav-tab').forEach(tab => {
      if (tab.dataset.view === viewName) {
        tab.classList.add('active');
      } else {
        tab.classList.remove('active');
      }
    });

    // Close mobile menu if open
    navTabs.classList.remove('open');

    // Show selected section
    document.querySelectorAll('.view-section').forEach(sec => {
      sec.classList.remove('active');
    });
    const targetSection = document.getElementById('view-' + viewName);
    if (targetSection) targetSection.classList.add('active');

    // Update Header Card
    headerSubtitle.style.display = 'block';
    if (viewName === 'dashboard') {
      const uname = currentUser ? currentUser.username : 'there';
      headerTitle.textContent = `${getTimeGreeting()}, ${uname}`;
      headerSubtitle.textContent = getLongDateString();
      headerActionBtn.style.display = 'inline-flex';
      headerActionBtn.textContent = 'Log exercise';
      headerActionBtn.onclick = () => switchView('log');
      loadDashboard();
    } else if (viewName === 'log') {
      headerTitle.textContent = 'Log exercise';
      headerSubtitle.textContent = 'Track your cardio and strength workouts';
      headerActionBtn.style.display = 'none';
      loadLogView();
    } else if (viewName === 'plans') {
      headerTitle.textContent = 'Workout plans';
      headerSubtitle.textContent = 'Manage and view routine templates';
      headerActionBtn.style.display = 'inline-flex';
      headerActionBtn.textContent = 'New plan';
      headerActionBtn.onclick = () => toggleNewPlanBox();
      loadPlans();
    } else if (viewName === 'history') {
      headerTitle.textContent = 'History';
      headerSubtitle.textContent = 'Review your previous workouts';
      headerActionBtn.style.display = 'none';
      loadHistory();
    } else if (viewName === 'profile') {
      headerTitle.textContent = 'Profile';
      headerSubtitle.textContent = '';
      headerSubtitle.style.display = 'none';
      headerActionBtn.style.display = 'none';
      loadProfile();
    }
  }

  // Bind nav tab clicks
  document.querySelectorAll('.nav-tab[data-view]').forEach(tab => {
    tab.addEventListener('click', () => {
      switchView(tab.dataset.view);
    });
  });

  // Mobile menu toggle
  if (mobileMenuBtn) {
    mobileMenuBtn.addEventListener('click', () => {
      navTabs.classList.toggle('open');
    });
  }

  // Logout handler
  async function performLogout() {
    await api('/api/logout', { method: 'POST' });
    localStorage.removeItem('fittrack_token');
    window.location.href = 'login.html';
  }

  if (logoutBtn) logoutBtn.addEventListener('click', performLogout);
  if (mobileLogoutBtn) mobileLogoutBtn.addEventListener('click', performLogout);

  // Initialize User Profile
  async function initUser() {
    const res = await api('/api/me');
    if (!res || !res.ok) return;
    currentUser = await res.json();

    if (topUsername) topUsername.textContent = currentUser.username;
    if (topWeight) topWeight.textContent = `${currentUser.weight} kg`;
    if (userAvatar) {
      userAvatar.textContent = currentUser.username.substring(0, 2);
    }

    switchView('dashboard');
  }

  // --- DASHBOARD LOGIC ---
  async function loadDashboard() {
    const res = await api('/api/summary');
    if (!res || !res.ok) return;
    const data = await res.json();

    // Summary cards
    document.getElementById('summaryCalories').textContent = `${data.todayCalories} kcal`;
    document.getElementById('summaryMinutes').textContent = `${data.todayMinutes} min`;
    document.getElementById('summaryStreak').textContent = `${data.streak} ${data.streak === 1 ? 'day' : 'days'}`;

    // Today progress
    const goal = data.goal || 500;
    document.getElementById('todayProgressText').textContent = `${data.todayCalories} of ${goal} kcal goal`;
    const pct = Math.min(100, Math.round((data.todayCalories / goal) * 100));
    const progressBar = document.getElementById('todayProgressBar');
    progressBar.style.setProperty('--target-width', `${pct}%`);
    progressBar.style.width = `${pct}%`;

    // Today exercises list
    const listEl = document.getElementById('todayExerciseList');
    listEl.innerHTML = '';
    if (!data.today || data.today.length === 0) {
      listEl.innerHTML = '<div class="empty-state">No exercises recorded today.</div>';
    } else {
      data.today.forEach((ex, idx) => {
        const row = document.createElement('div');
        row.className = 'row-b50 animate-rise';
        row.style.setProperty('--stagger', idx);

        let detail = '';
        if (ex.type === 'cardio') {
          detail = `${ex.durationMin} min (MET ${ex.met})`;
        } else {
          detail = `${ex.durationMin} min (${ex.sets} sets x ${ex.reps} reps)`;
        }

        row.innerHTML = `
          <div class="exercise-row-left">
            <span class="exercise-name">${escapeHtml(ex.name)}</span>
            <span class="muted-text">${detail}</span>
          </div>
          <span class="exercise-kcal">${ex.calories} kcal</span>
        `;
        listEl.appendChild(row);
      });
    }

    // Chart Rendering
    renderChart(data.week || []);
  }

  function renderChart(weekData) {
    const yAxisEl = document.getElementById('chartYAxis');
    const gridlinesEl = document.getElementById('chartGridlines');
    const barsEl = document.getElementById('chartBars');
    const xAxisEl = document.getElementById('chartXAxis');

    yAxisEl.innerHTML = '';
    gridlinesEl.innerHTML = '';
    barsEl.innerHTML = '';
    xAxisEl.innerHTML = '';

    // Calculate maximum calories
    let maxCal = 0;
    weekData.forEach(d => {
      if (d.calories > maxCal) maxCal = d.calories;
    });

    // Next multiple of 200 above maximum
    let axisMax = Math.ceil(maxCal / 200) * 200;
    if (axisMax < 200) axisMax = 200;

    const steps = 4;
    const stepVal = axisMax / steps;

    for (let i = steps; i >= 0; i--) {
      const val = Math.round(i * stepVal);
      const label = document.createElement('span');
      label.className = 'chart-y-label';
      label.textContent = val;
      yAxisEl.appendChild(label);

      const gridline = document.createElement('div');
      gridline.className = 'chart-gridline';
      gridlinesEl.appendChild(gridline);
    }

    // Find current day name
    const todayDayName = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'][new Date().getDay()];

    weekData.forEach((d, idx) => {
      const isToday = (d.day === todayDayName);
      const heightPct = axisMax > 0 ? Math.min(100, (d.calories / axisMax) * 100) : 0;

      // Bar column
      const col = document.createElement('div');
      col.className = 'chart-bar-column' + (isToday ? ' today' : '');

      const valLabel = document.createElement('span');
      valLabel.className = 'chart-bar-value';
      valLabel.textContent = `${d.calories} kcal`;

      const bar = document.createElement('div');
      bar.className = 'chart-bar' + (d.future ? ' future' : '');
      bar.style.setProperty('--bar-index', idx);
      bar.style.height = `${d.future ? 100 : Math.max(heightPct, 3)}%`;

      col.appendChild(valLabel);
      col.appendChild(bar);
      barsEl.appendChild(col);

      // X-axis label
      const xLabel = document.createElement('span');
      xLabel.className = 'chart-x-label' + (isToday ? ' today' : '');
      xLabel.textContent = d.day;
      xAxisEl.appendChild(xLabel);
    });
  }

  // --- LOG EXERCISE LOGIC ---
  const segCardio = document.getElementById('segCardio');
  const segStrength = document.getElementById('segStrength');
  const fieldGroupCardio = document.getElementById('fieldGroupCardio');
  const fieldGroupStrength = document.getElementById('fieldGroupStrength');
  const logForm = document.getElementById('logExerciseForm');
  const logMessage = document.getElementById('logMessage');
  const estimatedBurnValue = document.getElementById('estimatedBurnValue');

  function setLogType(type) {
    activeLogType = type;
    if (type === 'cardio') {
      segCardio.classList.add('active');
      segStrength.classList.remove('active');
      fieldGroupCardio.style.display = 'block';
      fieldGroupStrength.style.display = 'none';
      document.getElementById('exMet').required = true;
      document.getElementById('exSets').required = false;
      document.getElementById('exReps').required = false;
    } else {
      segCardio.classList.remove('active');
      segStrength.classList.add('active');
      fieldGroupCardio.style.display = 'none';
      fieldGroupStrength.style.display = 'block';
      document.getElementById('exMet').required = false;
      document.getElementById('exSets').required = true;
      document.getElementById('exReps').required = true;
    }
    updateEstimatedBurn();
  }

  if (segCardio) segCardio.addEventListener('click', () => setLogType('cardio'));
  if (segStrength) segStrength.addEventListener('click', () => setLogType('strength'));

  let prevEstimate = 0;
  function updateEstimatedBurn() {
    const duration = parseFloat(document.getElementById('exDuration').value) || 0;
    const weight = (currentUser && currentUser.weight) ? currentUser.weight : 70;
    let burn = 0;

    if (activeLogType === 'cardio') {
      const met = parseFloat(document.getElementById('exMet').value) || 0;
      burn = Math.round((met * weight * duration) / 60);
    } else {
      const sets = parseInt(document.getElementById('exSets').value) || 0;
      const reps = parseInt(document.getElementById('exReps').value) || 0;
      burn = Math.round(sets * reps * 0.5);
    }

    if (burn !== prevEstimate) {
      prevEstimate = burn;
      estimatedBurnValue.textContent = `${burn} kcal`;
      estimatedBurnValue.classList.remove('pop');
      void estimatedBurnValue.offsetWidth; // trigger reflow
      estimatedBurnValue.classList.add('pop');
    }
  }

  ['exDuration', 'exMet', 'exSets', 'exReps'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.addEventListener('input', updateEstimatedBurn);
  });

  async function loadLogView() {
    document.getElementById('exDate').value = getTodayDateString();
    updateEstimatedBurn();
    loadRecentExercises();
  }

  async function loadRecentExercises() {
    const res = await api('/api/exercises');
    if (!res || !res.ok) return;
    const list = await res.json();
    const recentEl = document.getElementById('recentExerciseList');
    recentEl.innerHTML = '';

    const recents = list.slice(0, 3);
    if (recents.length === 0) {
      recentEl.innerHTML = '<div class="empty-state">No recent workouts.</div>';
      return;
    }

    recents.forEach(ex => {
      const row = document.createElement('div');
      row.className = 'row-b50';
      let detail = ex.type === 'cardio' ? `${ex.durationMin} min (MET ${ex.met})` : `${ex.durationMin} min (${ex.sets}x${ex.reps})`;
      row.innerHTML = `
        <div class="exercise-row-left">
          <span class="exercise-name">${escapeHtml(ex.name)}</span>
          <span class="muted-text">${detail} &bull; ${ex.date}</span>
        </div>
        <span class="exercise-kcal">${ex.calories} kcal</span>
      `;
      recentEl.appendChild(row);
    });
  }

  if (logForm) {
    logForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      logMessage.textContent = '';

      const name = document.getElementById('exName').value.trim();
      const durationMin = parseInt(document.getElementById('exDuration').value);
      const date = document.getElementById('exDate').value;

      const payload = {
        type: activeLogType,
        name,
        durationMin,
        date
      };

      if (activeLogType === 'cardio') {
        payload.met = parseFloat(document.getElementById('exMet').value);
      } else {
        payload.sets = parseInt(document.getElementById('exSets').value);
        payload.reps = parseInt(document.getElementById('exReps').value);
      }

      const res = await api('/api/exercises', {
        method: 'POST',
        body: payload
      });

      if (res && res.ok) {
        logMessage.className = 'form-message success';
        logMessage.textContent = 'Exercise saved successfully.';
        logForm.reset();
        document.getElementById('exDate').value = getTodayDateString();
        updateEstimatedBurn();
        loadRecentExercises();
      } else {
        const data = res ? await res.json() : {};
        logMessage.className = 'form-message error';
        logMessage.textContent = data.error || 'Failed to save exercise.';
      }
    });
  }

  // --- WORKOUT PLANS LOGIC ---
  const newPlanBox = document.getElementById('newPlanBox');
  const createPlanForm = document.getElementById('createPlanForm');
  const newPlanMsg = document.getElementById('newPlanMsg');
  const planSelect = document.getElementById('planSelect');
  const planExType = document.getElementById('planExType');
  const planGroupStrength = document.getElementById('planGroupStrength');
  const planGroupCardio = document.getElementById('planGroupCardio');
  const addPlanExerciseForm = document.getElementById('addPlanExerciseForm');
  const addPlanExMsg = document.getElementById('addPlanExMsg');

  function toggleNewPlanBox() {
    newPlanBox.classList.toggle('active');
    if (newPlanBox.classList.contains('active')) {
      document.getElementById('newPlanName').focus();
    }
  }

  if (planExType) {
    planExType.addEventListener('change', () => {
      if (planExType.value === 'cardio') {
        planGroupCardio.style.display = 'block';
        planGroupStrength.style.display = 'none';
      } else {
        planGroupCardio.style.display = 'none';
        planGroupStrength.style.display = 'flex';
      }
    });
  }

  async function loadPlans() {
    const res = await api('/api/plans');
    if (!res || !res.ok) return;
    const plans = await res.json();

    const grid = document.getElementById('plansGrid');
    grid.innerHTML = '';
    planSelect.innerHTML = '';

    if (plans.length === 0) {
      grid.innerHTML = '<div class="empty-state" style="grid-column: 1 / -1;">No plans yet. Create your first plan.</div>';
      return;
    }

    plans.forEach((plan, idx) => {
      // Add option to select
      const opt = document.createElement('option');
      opt.value = plan.planName;
      opt.textContent = plan.planName;
      planSelect.appendChild(opt);

      // Render plan card
      const card = document.createElement('div');
      card.className = 'card plan-card animate-rise';
      card.style.setProperty('--stagger', idx);

      const exercisesCount = plan.exercises ? plan.exercises.length : 0;
      let exercisesHtml = '';
      if (plan.exercises && plan.exercises.length > 0) {
        exercisesHtml = plan.exercises.map(ex => {
          let detail = ex.type === 'cardio' ? `${ex.durationMin} min (MET ${ex.met})` : `${ex.durationMin} min (${ex.sets}x${ex.reps})`;
          return `
            <div class="row-b50" style="padding: 6px 10px; font-size: 13px;">
              <span class="exercise-name" style="font-size: 13px;">${escapeHtml(ex.name)}</span>
              <span class="muted-text">${detail}</span>
            </div>
          `;
        }).join('');
      } else {
        exercisesHtml = '<span class="muted-text" style="font-size: 12px;">No exercises in this plan yet.</span>';
      }

      card.innerHTML = `
        <div class="plan-header">
          <h3 class="card-title">${escapeHtml(plan.planName)}</h3>
          <span class="muted-text">${exercisesCount} exercises, ${plan.totalDuration} min</span>
        </div>
        <div class="plan-exercises-list">
          ${exercisesHtml}
        </div>
      `;
      grid.appendChild(card);
    });
  }

  if (createPlanForm) {
    createPlanForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      newPlanMsg.textContent = '';
      const planName = document.getElementById('newPlanName').value.trim();
      if (!planName) return;

      const res = await api('/api/plans', {
        method: 'POST',
        body: { planName }
      });

      if (res && res.ok) {
        document.getElementById('newPlanName').value = '';
        newPlanBox.classList.remove('active');
        loadPlans();
      } else {
        const d = res ? await res.json() : {};
        newPlanMsg.className = 'form-message error';
        newPlanMsg.textContent = d.error || 'Failed to create plan';
      }
    });
  }

  if (addPlanExerciseForm) {
    addPlanExerciseForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      addPlanExMsg.textContent = '';
      const selectedPlan = planSelect.value;
      if (!selectedPlan) return;

      const type = planExType.value;
      const name = document.getElementById('planExName').value.trim();
      const durationMin = parseInt(document.getElementById('planExDuration').value);

      const payload = { type, name, durationMin };
      if (type === 'cardio') {
        payload.met = parseFloat(document.getElementById('planExMet').value) || 6.0;
      } else {
        payload.sets = parseInt(document.getElementById('planExSets').value) || 3;
        payload.reps = parseInt(document.getElementById('planExReps').value) || 10;
      }

      const res = await api(`/api/plans/${encodeURIComponent(selectedPlan)}/exercises`, {
        method: 'POST',
        body: payload
      });

      if (res && res.ok) {
        addPlanExMsg.className = 'form-message success';
        addPlanExMsg.textContent = 'Exercise added to plan.';
        document.getElementById('planExName').value = '';
        loadPlans();
      } else {
        const d = res ? await res.json() : {};
        addPlanExMsg.className = 'form-message error';
        addPlanExMsg.textContent = d.error || 'Failed to add exercise.';
      }
    });
  }

  // --- HISTORY LOGIC ---
  async function loadHistory() {
    const container = document.getElementById('historyDaysContainer');
    container.innerHTML = '<div class="empty-state">Loading exercise history...</div>';

    const res = await api('/api/exercises');
    if (!res || !res.ok) return;
    const exercises = await res.json();

    container.innerHTML = '';
    if (exercises.length === 0) {
      container.innerHTML = '<div class="empty-state">No exercise records found.</div>';
      return;
    }

    // Group by date
    const byDate = {};
    exercises.forEach(ex => {
      const d = ex.date || 'Unknown Date';
      if (!byDate[d]) byDate[d] = [];
      byDate[d].push(ex);
    });

    const dates = Object.keys(byDate).sort().reverse();
    dates.forEach((dateStr, idx) => {
      const dayExercises = byDate[dateStr];
      let dayTotalCal = 0;
      dayExercises.forEach(e => dayTotalCal += (e.calories || 0));

      const card = document.createElement('div');
      card.className = 'card card-b100 history-day-card animate-rise';
      card.style.setProperty('--stagger', idx);

      const rowsHtml = dayExercises.map(ex => {
        let detail = ex.type === 'cardio' ? `${ex.durationMin} min (MET ${ex.met})` : `${ex.durationMin} min (${ex.sets} sets x ${ex.reps} reps)`;
        return `
          <div class="row-b50">
            <div class="exercise-row-left">
              <span class="exercise-name">${escapeHtml(ex.name)}</span>
              <span class="muted-text">${detail}</span>
            </div>
            <span class="exercise-kcal">${ex.calories} kcal</span>
          </div>
        `;
      }).join('');

      card.innerHTML = `
        <div class="history-day-header">
          <h3 class="card-title">${dateStr}</h3>
          <span class="muted-text">${Math.round(dayTotalCal)} kcal</span>
        </div>
        ${rowsHtml}
      `;
      container.appendChild(card);
    });
  }

  // --- PROFILE LOGIC ---
  const profileForm = document.getElementById('profileForm');
  const profileMsg = document.getElementById('profileMsg');

  async function loadProfile() {
    const res = await api('/api/me');
    if (!res || !res.ok) return;
    const user = await res.json();
    currentUser = user;

    const usernameInput = document.getElementById('profileUsername');
    if (usernameInput) usernameInput.value = user.username || '';
    document.getElementById('profileWeight').value = user.weight;
    document.getElementById('profileGoal').value = user.dailyGoal;

    // Load totals from summary
    const sumRes = await api('/api/summary');
    if (sumRes && sumRes.ok) {
      const sumData = await sumRes.json();
      if (sumData.totals) {
        const workoutsEl = document.getElementById('statTotalWorkouts');
        const minutesEl = document.getElementById('statTotalMinutes');
        const caloriesEl = document.getElementById('statTotalCalories');

        workoutsEl.textContent = Number(sumData.totals.workouts || 0).toLocaleString();
        minutesEl.textContent = Number(sumData.totals.minutes || 0).toLocaleString();
        caloriesEl.textContent = Number(sumData.totals.calories || 0).toLocaleString();

        [workoutsEl, minutesEl, caloriesEl].forEach(el => {
          el.classList.remove('profile-pop');
          void el.offsetWidth;
          el.classList.add('profile-pop');
        });
      }
    }
  }

  if (profileForm) {
    profileForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      profileMsg.textContent = '';

      const weight = parseFloat(document.getElementById('profileWeight').value);
      const dailyGoal = parseInt(document.getElementById('profileGoal').value);

      if (isNaN(weight) || weight < 20 || weight > 300) {
        profileMsg.className = 'form-message error';
        profileMsg.textContent = 'Weight must be between 20 and 300 kg';
        return;
      }

      if (isNaN(dailyGoal) || dailyGoal < 100 || dailyGoal > 10000) {
        profileMsg.className = 'form-message error';
        profileMsg.textContent = 'Daily goal must be between 100 and 10000 kcal';
        return;
      }

      const res = await api('/api/me', {
        method: 'PUT',
        body: { weight, dailyGoal }
      });

      if (res && res.ok) {
        currentUser = await res.json();
        topWeight.textContent = `${currentUser.weight} kg`;
        profileMsg.className = 'form-message success';
        profileMsg.textContent = 'Profile updated successfully.';
      } else {
        const d = res ? await res.json() : {};
        profileMsg.className = 'form-message error';
        profileMsg.textContent = d.error || 'Failed to update profile.';
      }
    });
  }

  // Sanitizer helper
  function escapeHtml(str) {
    if (!str) return '';
    return str.replace(/&/g, '&amp;')
              .replace(/</g, '&lt;')
              .replace(/>/g, '&gt;')
              .replace(/"/g, '&quot;')
              .replace(/'/g, '&#039;');
  }

  // Start app
  initUser();
});
