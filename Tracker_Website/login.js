document.addEventListener('DOMContentLoaded', () => {
    const loginForm = document.getElementById('loginForm');

    if (loginForm) {
        loginForm.addEventListener('submit', function(e) {
            e.preventDefault();

            const user = document.getElementById('username').value.trim();
            const pass = document.getElementById('password').value.trim();

            // Credentials: admin / tracker123
            if (user === 'admin' && pass === 'tracker123') {
                localStorage.setItem('isLoggedIn', 'true');
                localStorage.setItem('username', user);
                window.location.replace('index.html'); 
            } else {
                alert('Invalid Credentials! Please try again.');
            }
        });
    }
});