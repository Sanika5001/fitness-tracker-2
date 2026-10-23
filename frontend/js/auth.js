document.addEventListener('DOMContentLoaded', () => {
  const loginForm = document.getElementById('loginForm');
  const registerForm = document.getElementById('registerForm');
  const authMsg = document.getElementById('authMsg');

  function showMessage(text, isError = true) {
    if (!authMsg) return;
    authMsg.textContent = text;
    authMsg.className = 'form-message ' + (isError ? 'error' : 'success');
  }

  // Check URL query parameters for registration success message
  const params = new URLSearchParams(window.location.search);
  if (params.get('registered') === '1') {
    showMessage('Account created. Please log in.', false);
  }

  // If already logged in, redirect to dashboard
  if (localStorage.getItem('fittrack_token')) {
    // Verify token validity by calling /api/me
    fetch('/api/me', {
      headers: { 'Authorization': 'Bearer ' + localStorage.getItem('fittrack_token') }
    }).then(res => {
      if (res.ok) window.location.href = 'dashboard.html';
    }).catch(() => {});
  }

  // Login handler
  if (loginForm) {
    loginForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const username = loginForm.username.value.trim();
      const password = loginForm.password.value;

      if (!username || !password) {
        showMessage('Please enter username and password');
        return;
      }

      try {
        const res = await fetch('/api/login', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ username, password })
        });
        const data = await res.json();
        if (!res.ok) {
          showMessage(data.error || 'Invalid credentials');
          return;
        }

        localStorage.setItem('fittrack_token', data.token);
        window.location.href = 'dashboard.html';
      } catch (err) {
        showMessage('Unable to connect to server');
      }
    });
  }

  // Register handler
  if (registerForm) {
    registerForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const username = registerForm.username.value.trim();
      const password = registerForm.password.value;
      const confirmPassword = registerForm.confirmPassword.value;
      const weight = parseFloat(registerForm.weight.value);

      if (!/^[a-zA-Z0-9_]{3,20}$/.test(username)) {
        showMessage('Username must be 3-20 letters, digits or underscore');
        return;
      }

      if (password.length < 6) {
        showMessage('Password must be at least 6 characters');
        return;
      }

      if (password !== confirmPassword) {
        showMessage('Passwords do not match');
        return;
      }

      if (isNaN(weight) || weight < 20 || weight > 300) {
        showMessage('Weight must be between 20 and 300 kg');
        return;
      }

      try {
        const res = await fetch('/api/register', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ username, password, weight })
        });
        const data = await res.json();
        if (!res.ok) {
          showMessage(data.error || 'Registration failed');
          return;
        }

        window.location.href = 'login.html?registered=1';
      } catch (err) {
        showMessage('Unable to connect to server');
      }
    });
  }
});
