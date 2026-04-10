#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

/* * SMART STREET LIGHT POWER MONITORING SYSTEM
 * Powered by Tronix365
 */

// --- CONFIGURATION ---
const char* ssid = "Air";
const char* password = "99999999";

WebServer server(80);

struct NodeData {
  float V;
  float I;
  float P;
  bool SL;
  bool FAULT;
};

NodeData node1, node2, node3;

// --- DATA PARSING LOGIC ---
void parseNode(String s, NodeData &n) {
  int i1 = s.indexOf(',');                
  int i2 = s.indexOf(',', i1 + 1);            
  int i3 = s.indexOf(',', i2 + 1);            
  int i4 = s.indexOf(',', i3 + 1);            
  int i5 = s.indexOf(',', i4 + 1);            

  if(i1 == -1) return; // Basic validation

  n.V     = s.substring(i1 + 1, i2).toFloat();
  n.I     = s.substring(i2 + 1, i3).toFloat();
  n.P     = s.substring(i3 + 1, i4).toFloat();
  n.SL    = s.substring(i4 + 1, i5).toInt();
  n.FAULT = s.substring(i5 + 1).toInt();
}

// --- WEB SERVER HANDLERS ---
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Street Light Monitor</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600;800&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg: #050505;
            --card-bg: rgba(20, 20, 20, 0.8);
            --accent: #2563eb;
            --neon-blue: #00f2ff;
            --neon-yellow: #ffdd00;
            --danger: #ff4d4d;
            --success: #00ff88;
        }

        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background-color: var(--bg);
            color: white;
            font-family: 'Inter', sans-serif;
            overflow-x: hidden;
            background: radial-gradient(circle at top right, #1e1b4b, #000);
        }

        header {
            padding: 2rem;
            text-align: center;
            border-bottom: 1px solid rgba(255,255,255,0.1);
            position: relative;
        }

        .powered-by {
            position: absolute;
            top: 10px;
            right: 20px;
            font-size: 0.8rem;
            color: var(--neon-blue);
            text-transform: uppercase;
            letter-spacing: 2px;
            font-weight: 800;
        }

        h1 {
            font-size: 2.5rem;
            font-weight: 800;
            text-transform: uppercase;
            letter-spacing: -1px;
            background: linear-gradient(to right, #fff, #666);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            padding: 2rem;
            max-width: 1400px;
            margin: auto;
        }

        .card {
            background: var(--card-bg);
            border: 1px solid rgba(255,255,255,0.05);
            border-radius: 20px;
            padding: 25px;
            backdrop-filter: blur(10px);
            transition: transform 0.3s ease;
            position: relative;
        }

        .card:hover { transform: translateY(-5px); border-color: var(--accent); }

        .node-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }

        .status-badge {
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 0.75rem;
            font-weight: bold;
            text-transform: uppercase;
        }

        .status-ok { background: rgba(0, 255, 136, 0.2); color: var(--success); }
        .status-fault { background: rgba(255, 77, 77, 0.2); color: var(--danger); animation: blink 1s infinite; }

        @keyframes blink { 50% { opacity: 0.3; } }

        .bulb-container {
            width: 80px;
            height: 120px;
            margin: 0 auto 20px;
            position: relative;
        }

        .bulb {
            width: 60px;
            height: 60px;
            background: #333;
            border-radius: 50%;
            margin: 0 auto;
            position: relative;
            transition: all 0.5s ease;
            box-shadow: inset 0 0 10px rgba(0,0,0,0.5);
        }

        .bulb::after {
            content: '';
            position: absolute;
            bottom: -15px;
            left: 50%;
            transform: translateX(-50%);
            width: 30px;
            height: 20px;
            background: #555;
            border-radius: 0 0 5px 5px;
        }

        .bulb.on {
            background: var(--neon-yellow);
            box-shadow: 0 0 40px var(--neon-yellow), 0 0 80px rgba(255, 221, 0, 0.3);
        }

        .metrics {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }

        .metric {
            background: rgba(255,255,255,0.03);
            padding: 12px;
            border-radius: 12px;
            text-align: center;
        }

        .metric-val {
            display: block;
            font-size: 1.2rem;
            font-weight: 800;
            color: var(--neon-blue);
        }

        .metric-label {
            font-size: 0.7rem;
            color: #888;
            text-transform: uppercase;
        }

        .charts-container {
            grid-column: 1 / -1;
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }

        canvas { width: 100% !important; height: 300px !important; }

        @media (max-width: 768px) {
            .charts-container { grid-template-columns: 1fr; }
            h1 { font-size: 1.5rem; }
        }
    </style>
</head>
<body>
    <header>
        <div class="powered-by">Powered by Tronix365</div>
        <h1>Smart Street Light Power Monitoring System</h1>
    </header>

    <main class="dashboard">
        <!-- Node 1 -->
        <div class="card">
            <div class="node-header">
                <h3>STREET LIGHT 01</h3>
                <span id="n1-status" class="status-badge">OK</span>
            </div>
            <div class="bulb-container">
                <div id="n1-bulb" class="bulb"></div>
            </div>
            <div class="metrics">
                <div class="metric"><span class="metric-val" id="n1-v">0</span><span class="metric-label">Voltage (V)</span></div>
                <div class="metric"><span class="metric-val" id="n1-i">0</span><span class="metric-label">Current (A)</span></div>
                <div class="metric"><span class="metric-val" id="n1-p">0</span><span class="metric-label">Power (W)</span></div>
                <div class="metric"><span class="metric-val" id="n1-sl">OFF</span><span class="metric-label">Light Status</span></div>
            </div>
        </div>

        <!-- Node 2 -->
        <div class="card">
            <div class="node-header">
                <h3>STREET LIGHT 02</h3>
                <span id="n2-status" class="status-badge">OK</span>
            </div>
            <div class="bulb-container">
                <div id="n2-bulb" class="bulb"></div>
            </div>
            <div class="metrics">
                <div class="metric"><span class="metric-val" id="n2-v">0</span><span class="metric-label">Voltage (V)</span></div>
                <div class="metric"><span class="metric-val" id="n2-i">0</span><span class="metric-label">Current (A)</span></div>
                <div class="metric"><span class="metric-val" id="n2-p">0</span><span class="metric-label">Power (W)</span></div>
                <div class="metric"><span class="metric-val" id="n2-sl">OFF</span><span class="metric-label">Light Status</span></div>
            </div>
        </div>

        <!-- Node 3 -->
        <div class="card">
            <div class="node-header">
                <h3>STREET LIGHT 03</h3>
                <span id="n3-status" class="status-badge">OK</span>
            </div>
            <div class="bulb-container">
                <div id="n3-bulb" class="bulb"></div>
            </div>
            <div class="metrics">
                <div class="metric"><span class="metric-val" id="n3-v">0</span><span class="metric-label">Voltage (V)</span></div>
                <div class="metric"><span class="metric-val" id="n3-i">0</span><span class="metric-label">Current (A)</span></div>
                <div class="metric"><span class="metric-val" id="n3-p">0</span><span class="metric-label">Power (W)</span></div>
                <div class="metric"><span class="metric-val" id="n3-sl">OFF</span><span class="metric-label">Light Status</span></div>
            </div>
        </div>

        <!-- Charts Section -->
        <div class="charts-container">
            <div class="card">
                <h3>Current Consumption (A)</h3>
                <canvas id="currentChart"></canvas>
            </div>
            <div class="card">
                <h3>System Voltage (V)</h3>
                <canvas id="voltageChart"></canvas>
            </div>
        </div>
    </main>

    <script>
        let currentData = { n1: [], n2: [], n3: [], labels: [] };
        let voltageData = { n1: [], n2: [], n3: [], labels: [] };

        const ctxI = document.getElementById('currentChart').getContext('2d');
        const ctxV = document.getElementById('voltageChart').getContext('2d');

        const chartConfig = (label, color) => ({
            label: label,
            borderColor: color,
            borderWidth: 2,
            pointRadius: 0,
            fill: true,
            backgroundColor: color + '22',
            tension: 0.4,
            data: []
        });

        const iChart = new Chart(ctxI, {
            type: 'line',
            data: {
                labels: [],
                datasets: [chartConfig('Node 1', '#00f2ff'), chartConfig('Node 2', '#ff00ff'), chartConfig('Node 3', '#00ff88')]
            },
            options: { responsive: true, maintainAspectRatio: false, scales: { y: { grid: { color: '#222' } }, x: { grid: { display: false } } } }
        });

        const vChart = new Chart(ctxV, {
            type: 'line',
            data: {
                labels: [],
                datasets: [chartConfig('Node 1', '#00f2ff'), chartConfig('Node 2', '#ff00ff'), chartConfig('Node 3', '#00ff88')]
            },
            options: { responsive: true, maintainAspectRatio: false, scales: { y: { grid: { color: '#222' } }, x: { grid: { display: false } } } }
        });

        async function updateData() {
            try {
                const response = await fetch('/data');
                const data = await response.json();

                // Update UI elements
                [1, 2, 3].forEach(id => {
                    const node = data[`n${id}`];
                    document.getElementById(`n${id}-v`).innerText = node.V.toFixed(1);
                    document.getElementById(`n${id}-i`).innerText = node.I.toFixed(2);
                    document.getElementById(`n${id}-p`).innerText = node.P.toFixed(1);
                    document.getElementById(`n${id}-sl`).innerText = node.SL ? "ON" : "OFF";
                    
                    const bulb = document.getElementById(`n${id}-bulb`);
                    if(node.SL) bulb.classList.add('on'); else bulb.classList.remove('on');

                    const status = document.getElementById(`n${id}-status`);
                    if(node.FAULT) {
                        status.innerText = "FAULTY";
                        status.className = "status-badge status-fault";
                    } else {
                        status.innerText = "OK";
                        status.className = "status-badge status-ok";
                    }

                    // Update Charts
                    const time = new Date().toLocaleTimeString();
                    if (iChart.data.labels.length > 20) {
                        iChart.data.labels.shift();
                        iChart.data.datasets.forEach(d => d.data.shift());
                        vChart.data.labels.shift();
                        vChart.data.datasets.forEach(d => d.data.shift());
                    }
                    
                    if(id === 1) { // Only add timestamp once per cycle
                        iChart.data.labels.push(time);
                        vChart.data.labels.push(time);
                    }
                    iChart.data.datasets[id-1].data.push(node.I);
                    vChart.data.datasets[id-1].data.push(node.V);
                });

                iChart.update('none');
                vChart.update('none');

            } catch (e) { console.error("Update failed", e); }
        }

        setInterval(updateData, 2000);
    </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  
  auto nodeToJson = [](int id, NodeData n) {
    return "\"n" + String(id) + "\":{\"V\":" + String(n.V) + 
           ",\"I\":" + String(n.I) + ",\"P\":" + String(n.P) + 
           ",\"SL\":" + String(n.SL) + ",\"FAULT\":" + String(n.FAULT) + "}";
  };

  json += nodeToJson(1, node1) + ",";
  json += nodeToJson(2, node2) + ",";
  json += nodeToJson(3, node3);
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  // WiFi Connectivity
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("SYSTEM READY");
}

void loop() {
  server.handleClient();

  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      int s1 = line.indexOf(';');
      int s2 = line.indexOf(';', s1 + 1);

      if (s1 != -1 && s2 != -1) {
        String n1 = line.substring(0, s1);
        String n2 = line.substring(s1 + 1, s2);
        String n3 = line.substring(s2 + 1);

        parseNode(n1, node1);
        parseNode(n2, node2);
        parseNode(n3, node3);

        // Debug to Serial
        Serial.println("--- Update Received ---");
      }
    }
  }
}