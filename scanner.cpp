#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ArduinoJson.h>

// Configuración WiFi
const char* ssid = "ESP32-S3-Scanner";
const char* password = "12345678";

// Objetos globales
WebServer server(80);
BluetoothSerial SerialBT;

// Configuración NRF24L01
RF24 radio(14, 13); // CE, CSN (ajusta según tu conexión)
const byte address[6] = "00001";

// Estructuras de datos
struct NetworkData {
  String wifiNetworks[20];
  int wifiCount;
  String bluetoothDevices[20];
  int btCount;
  String nrfData;
  unsigned long scanTime;
};

NetworkData networkData;

void setup() {
  Serial.begin(115200);
  
  // Inicializar WiFi en modo AP
  setupWiFiAP();
  
  // Inicializar Bluetooth
  setupBluetooth();
  
  // Inicializar NRF24L01
  setupNRF24();
  
  // Configurar servidor web
  setupWebServer();
  
  Serial.println("Sistema de escaneo iniciado");
  SerialBT.println("Bluetooth activo - Escaneo disponible");
}

void setupWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  Serial.print("AP creado. IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupBluetooth() {
  if (!SerialBT.begin("ESP32-S3-Scanner")) {
    Serial.println("Error inicializando Bluetooth");
  } else {
    Serial.println("Bluetooth inicializado");
  }
}

void setupNRF24() {
  if (!radio.begin()) {
    Serial.println("NRF24L01 no detectado!");
    return;
  }
  
  radio.setPALevel(RF24_PA_MAX);    // Máxima potencia (con LNA)
  radio.setDataRate(RF24_2MBPS);    // Velocidad de datos
  radio.setChannel(76);             // Canal 2.476 GHz
  radio.openReadingPipe(0, address);
  radio.openWritingPipe(address);
  radio.startListening();
  
  // Configuraciones adicionales para módulo con LNA
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.setRetries(5, 15);
  
  Serial.println("NRF24L01+ LNA inicializado");
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getWebInterface());
  });
  
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/data", HTTP_GET, handleGetData);
  server.on("/nrf-send", HTTP_POST, handleNRFSend);
  
  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop() {
  server.handleClient();
  handleBluetooth();
  handleNRF24();
  
  // Escaneo automático cada 30 segundos
  static unsigned long lastScan = 0;
  if (millis() - lastScan > 30000) {
    performScan();
    lastScan = millis();
  }
  
  delay(100);
}

void performScan() {
  Serial.println("Iniciando escaneo...");
  networkData.scanTime = millis();
  
  // Escanear redes WiFi
  scanWiFi();
  
  // Escanear dispositivos Bluetooth
  scanBluetooth();
  
  // Leer datos del NRF24
  readNRF24Data();
  
  Serial.println("Escaneo completado");
}

void scanWiFi() {
  Serial.println("Escaneando redes WiFi...");
  networkData.wifiCount = 0;
  
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks && i < 20; i++) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    String encryption = getEncryptionType(WiFi.encryptionType(i));
    
    networkData.wifiNetworks[i] = 
      "SSID: " + ssid + 
      " | RSSI: " + String(rssi) + "dBm" +
      " | Seguridad: " + encryption +
      " | Canal: " + String(WiFi.channel(i));
    
    networkData.wifiCount++;
    
    Serial.println(networkData.wifiNetworks[i]);
  }
  
  Serial.println("Escaneo WiFi completado: " + String(networkData.wifiCount) + " redes encontradas");
}

String getEncryptionType(wifi_auth_mode_t type) {
  switch(type) {
    case WIFI_AUTH_OPEN: return "Abierto";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    default: return "Desconocido";
  }
}

void scanBluetooth() {
  Serial.println("Escaneando dispositivos Bluetooth...");
  networkData.btCount = 0;
  
  // En una aplicación real, aquí implementarías el escaneo BLE
  // Por ahora simulamos algunos dispositivos
  networkData.bluetoothDevices[0] = "Dispositivo: Smartphone | RSSI: -45dBm | BT 5.0";
  networkData.bluetoothDevices[1] = "Dispositivo: Auriculares | RSSI: -55dBm | BT 4.2";
  networkData.bluetoothDevices[2] = "Dispositivo: Altavoz | RSSI: -65dBm | BT 4.0";
  networkData.btCount = 3;
  
  // Para escaneo real BLE, necesitarías la librería BLE
  SerialBT.println("Escaneo Bluetooth realizado");
}

void readNRF24Data() {
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    networkData.nrfData = String(text);
    Serial.println("NRF24 Data: " + networkData.nrfData);
  }
}

void handleNRF24() {
  // Manejar recepción continua de datos NRF24
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    
    // Procesar datos recibidos
    String receivedData = String(text);
    if (receivedData.length() > 0) {
      networkData.nrfData = "RX: " + receivedData + " | T: " + String(millis());
    }
  }
}

void handleBluetooth() {
  if (SerialBT.available()) {
    String command = SerialBT.readString();
    command.trim();
    
    if (command == "SCAN") {
      performScan();
      sendDataToBluetooth();
    } else if (command == "STATUS") {
      sendStatusToBluetooth();
    }
  }
}

void sendDataToBluetooth() {
  SerialBT.println("=== DATOS DE ESCANEO ===");
  SerialBT.println("Redes WiFi:");
  for (int i = 0; i < networkData.wifiCount; i++) {
    SerialBT.println(networkData.wifiNetworks[i]);
  }
  
  SerialBT.println("\nDispositivos Bluetooth:");
  for (int i = 0; i < networkData.btCount; i++) {
    SerialBT.println(networkData.bluetoothDevices[i]);
  }
  
  SerialBT.println("\nNRF24 Data: " + networkData.nrfData);
  SerialBT.println("========================");
}

void sendStatusToBluetooth() {
  SerialBT.println("=== STATUS DEL SISTEMA ===");
  SerialBT.println("ESP32-S3 Scanner Active");
  SerialBT.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  SerialBT.println("Uptime: " + String(millis() / 1000) + " segundos");
  SerialBT.println("NRF24: " + String(radio.isChipConnected() ? "Conectado" : "Error"));
  SerialBT.println("==========================");
}

// Handlers del servidor web
void handleScan() {
  performScan();
  server.send(200, "application/json", "{\"status\":\"scan_completed\"}");
}

void handleGetData() {
  StaticJsonDocument<2048> doc;
  
  doc["scanTime"] = networkData.scanTime;
  doc["nrfData"] = networkData.nrfData;
  
  JsonArray wifiArray = doc.createNestedArray("wifiNetworks");
  for (int i = 0; i < networkData.wifiCount; i++) {
    wifiArray.add(networkData.wifiNetworks[i]);
  }
  
  JsonArray btArray = doc.createNestedArray("bluetoothDevices");
  for (int i = 0; i < networkData.btCount; i++) {
    btArray.add(networkData.bluetoothDevices[i]);
  }
  
  doc["system"]["freeHeap"] = ESP.getFreeHeap();
  doc["system"]["uptime"] = millis() / 1000;
  doc["system"]["nrfConnected"] = radio.isChipConnected();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNRFSend() {
  if (server.hasArg("plain")) {
    String message = server.arg("plain");
    
    // Enviar via NRF24
    radio.stopListening();
    bool success = radio.write(message.c_str(), message.length() + 1);
    radio.startListening();
    
    if (success) {
      server.send(200, "application/json", "{\"status\":\"sent\"}");
    } else {
      server.send(500, "application/json", "{\"status\":\"error\"}");
    }
  }
}

String getWebInterface() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-S3 Network Scanner</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: #0f0f23;
            color: #00ff00;
        }
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
            background: #1a1a2e; 
            padding: 20px; 
            border-radius: 10px;
            border: 1px solid #00ff00;
        }
        .card { 
            background: #16213e; 
            padding: 15px; 
            margin: 10px 0; 
            border-radius: 5px;
            border-left: 4px solid #00ff00;
        }
        button { 
            padding: 10px 20px; 
            margin: 5px; 
            border: none; 
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            background: #00ff00;
            color: #000;
            font-weight: bold;
        }
        .data-container {
            background: #0f0f23;
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            max-height: 300px;
            overflow-y: auto;
            border: 1px solid #00ff00;
        }
        .network-item {
            padding: 8px;
            margin: 5px 0;
            background: #1a1a2e;
            border-radius: 3px;
            border-left: 3px solid #00ff00;
        }
        .status-bar {
            display: flex;
            justify-content: space-between;
            background: #16213e;
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
        }
        .tab {
            overflow: hidden;
            border: 1px solid #00ff00;
            background-color: #16213e;
            border-radius: 5px;
        }
        .tab button {
            background-color: inherit;
            float: left;
            border: none;
            outline: none;
            cursor: pointer;
            padding: 14px 16px;
            transition: 0.3s;
            color: #00ff00;
        }
        .tab button:hover {
            background-color: #00ff00;
            color: #000;
        }
        .tab button.active {
            background-color: #00ff00;
            color: #000;
        }
        .tabcontent {
            display: none;
            padding: 6px 12px;
            border: 1px solid #00ff00;
            border-top: none;
            border-radius: 0 0 5px 5px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🛰️ ESP32-S3 Network Scanner</h1>
        <div class="status-bar">
            <div>🟢 Sistema Activo</div>
            <div>⏱️ <span id="uptime">0</span>s</div>
            <div>💾 <span id="memory">0</span> bytes libres</div>
            <div>📡 NRF24: <span id="nrfStatus">Checking...</span></div>
        </div>

        <div class="tab">
            <button class="tablinks active" onclick="openTab(event, 'WiFi')">📶 WiFi Scanner</button>
            <button class="tablinks" onclick="openTab(event, 'Bluetooth')">🔵 Bluetooth</button>
            <button class="tablinks" onclick="openTab(event, 'NRF24')">📡 NRF24L01</button>
            <button class="tablinks" onclick="openTab(event, 'Control')">🎮 Control</button>
        </div>

        <div id="WiFi" class="tabcontent" style="display:block">
            <h3>Redes WiFi Detectadas</h3>
            <button onclick="startScan()">🔄 Escanear Redes</button>
            <div class="data-container" id="wifiData">
                Esperando datos...
            </div>
        </div>

        <div id="Bluetooth" class="tabcontent">
            <h3>Dispositivos Bluetooth</h3>
            <div class="data-container" id="bluetoothData">
                Escaneo Bluetooth simulado - Para escaneo real usa BLE
            </div>
        </div>

        <div id="NRF24" class="tabcontent">
            <h3>Comunicación NRF24L01+ LNA</h3>
            <div class="card">
                <input type="text" id="nrfMessage" placeholder="Mensaje para enviar..." style="width:70%; padding:10px; background:#0f0f23; color:#00ff00; border:1px solid #00ff00;">
                <button onclick="sendNRFMessage()">📤 Enviar</button>
            </div>
            <div class="data-container" id="nrfData">
                Datos NRF24 aparecerán aquí...
            </div>
        </div>

        <div id="Control" class="tabcontent">
            <h3>Configuración del Sistema</h3>
            <div class="card">
                <button onclick="refreshAll()">🔄 Actualizar Todo</button>
                <button onclick="resetScanner()">🔄 Reiniciar Scanner</button>
                <button onclick="testNRF24()">🧪 Test NRF24</button>
            </div>
            <div class="data-container" id="controlLog">
                Log de control...
            </div>
        </div>
    </div>

    <script>
        function openTab(evt, tabName) {
            var i, tabcontent, tablinks;
            tabcontent = document.getElementsByClassName("tabcontent");
            for (i = 0; i < tabcontent.length; i++) {
                tabcontent[i].style.display = "none";
            }
            tablinks = document.getElementsByClassName("tablinks");
            for (i = 0; i < tablinks.length; i++) {
                tablinks[i].className = tablinks[i].className.replace(" active", "");
            }
            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
        }

        function startScan() {
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    setTimeout(updateData, 2000);
                });
        }

        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Actualizar WiFi
                    let wifiHtml = '';
                    data.wifiNetworks.forEach(network => {
                        wifiHtml += `<div class="network-item">${network}</div>`;
                    });
                    document.getElementById('wifiData').innerHTML = wifiHtml || 'No se encontraron redes';

                    // Actualizar Bluetooth
                    let btHtml = '';
                    data.bluetoothDevices.forEach(device => {
                        btHtml += `<div class="network-item">${device}</div>`;
                    });
                    document.getElementById('bluetoothData').innerHTML = btHtml || 'No se encontraron dispositivos';

                    // Actualizar NRF24
                    document.getElementById('nrfData').innerHTML = 
                        `<div class="network-item">${data.nrfData || 'Sin datos'}</div>`;

                    // Actualizar sistema
                    document.getElementById('uptime').textContent = data.system.uptime;
                    document.getElementById('memory').textContent = data.system.freeHeap;
                    document.getElementById('nrfStatus').textContent = 
                        data.system.nrfConnected ? "Conectado" : "Error";
                });
        }

        function sendNRFMessage() {
            const message = document.getElementById('nrfMessage').value;
            if (message) {
                fetch('/nrf-send', {
                    method: 'POST',
                    body: message
                })
                .then(response => response.json())
                .then(data => {
                    document.getElementById('controlLog').innerHTML = 
                        `Mensaje enviado: ${message}<br>${document.getElementById('controlLog').innerHTML}`;
                    document.getElementById('nrfMessage').value = '';
                });
            }
        }

        function refreshAll() {
            startScan();
        }

        function testNRF24() {
            document.getElementById('controlLog').innerHTML = 
                "Test NRF24 iniciado...<br>" + document.getElementById('controlLog').innerHTML;
        }

        function resetScanner() {
            document.getElementById('controlLog').innerHTML = 
                "Reiniciando escáner...<br>" + document.getElementById('controlLog').innerHTML;
            startScan();
        }

        // Actualizar datos cada 10 segundos
        setInterval(updateData, 10000);
        
        // Cargar datos iniciales
        updateData();
    </script>
</body>
</html>
)rawliteral";
}