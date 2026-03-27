const webapp_url = "YOUR_GOOGLE_SCRIPT_URL_HERE";

function logout() {
    localStorage.removeItem('isLoggedIn');
    window.location.href = 'login.html';
}

async function fetchTrackerData() {
    // RECTIFIED: Matching the ID from your index.html
    const tableBody = document.getElementById('logTable');
    if (!tableBody) return;

    try {
        const response = await fetch(webapp_url);
        const data = await response.json(); 

        tableBody.innerHTML = ""; // Clear old rows

        if (Array.isArray(data)) {
            data.forEach((row, index) => {
                const tr = document.createElement('tr');
                
                // Highlight Emergency or Inactivity statuses
                if (row.status === "SOS" || row.status === "INACTIVITY") {
                    tr.style.color = "#f87171"; 
                    tr.style.fontWeight = "bold";
                }

                // Add Google Maps link to coordinates
                const mapLink = `https://www.google.com/maps?q=${row.lat},${row.lon}`;

                tr.innerHTML = `
                    <td>${row.date}</td>
                    <td>${row.time}</td>
                    <td><a href="${mapLink}" target="_blank" style="color: #38bdf8;">${row.lat}, ${row.lon}</a></td>
                    <td>${row.status}</td>
                `;
                tableBody.appendChild(tr);

                // Auto-update the iframe map to the very latest coordinate
                if (index === 0) {
                    updateMapDisplay(`${row.lat},${row.lon}`);
                }
            });
        }
    } catch (e) {
        console.error("Connection to Google Sheets failed.", e);
    }
}

// INTERACTIVE MAP LOGIC
function updateMapDisplay(coords) {
    const mapIframe = document.getElementById('googleMap');
    if (mapIframe && coords) {
        // Standard Google Maps Embed URL
        const embedUrl = `https://maps.google.com/maps?q=${coords}&t=&z=15&ie=UTF8&iwloc=&output=embed`;
        mapIframe.src = embedUrl;
    }
}

// Manual search from sidebar
function updateMap() {
    const coordInput = document.getElementById('latLong').value;
    if (coordInput.trim() !== "") {
        updateMapDisplay(coordInput.trim());
    } else {
        alert("Please enter valid coordinates.");
    }
}

// INITIALIZATION (Only run if on dashboard)
window.onload = () => {
    if (window.location.pathname.includes('index.html') || window.location.pathname === "/") {
        fetchTrackerData();
        setInterval(fetchTrackerData, 30000); // 30s auto-refresh
    }
};
