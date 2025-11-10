#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include <vector>

#include "BackEnd.h"
#include "Controller.h"
#include "Field.h"
#include "Helper.h"
#include "JsonWrapper.h"
#include "Model.h"
#include "Pass.h"

AsyncWebServer server(80);  // needs to persist beyond the method

String generateMenu() {
    return "<p>"
           "<a href='/'>Index</a> | "
           "<a href='/info'>Info</a> | "
           "<a href='/metadata'>Metadata</a> | "
           "<a href='/advanced'>Advanced</a> | "
           "<a href='/chart'>Chart</a> | "
           "</p>";
}

String generateIndexPage(bool brief) {  // from device to display
    String html = generateMenu();
    std::vector<Field> fi = Controller::model.getScreenFields(brief);
    if (brief) {
        html += "<h1>Status Page</h1>";
    } else {
        html += "<h1>Extended Page</h1>";
    }
    html += "<table border=1><tr><th>Name</th><th>Type</th><th>Value</th><th>Description</th></tr>";
    for (auto& f : fi) {
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td>";
        if (f.getReadOnly())
            html += "<td><input value='" + f.getValue() + "' disabled></td>";
        else
            html += "<td><input data-id='" + f.getId() + "' value='" + f.getValue() + "' onchange='onChange(this)'></td>";
        html += "<td>" + f.getDescription() + "</td></tr>";
    }
    html += R"(<script>
        var ws = new WebSocket('ws://' + location.hostname + '/ws');
                    ws.onmessage = function(evt) {
                try {
                    var data = JSON.parse(evt.data);
                    if (!Array.isArray(data)) return;
                    var inputs = document.querySelectorAll("input[data-id]");
                    // reload if number of fields changed (add or delete)
                   data.forEach(f => {
                        var el = document.querySelector("input[data-id='" + f.id + "']");
                        if (el && document.activeElement !== el) {
                            if (el.value !== f.value) {
                                el.value = f.value;                                
                                el.style.transition = "background-color 0.8s";// highlight change
                                el.style.backgroundColor = "#fff3a0"; // light yellow
                                setTimeout(() => {
                                    el.style.backgroundColor = "";
                                }, 800);
                            }
                        }
                    });
                } catch(e) {
                    console.error("WS update error:", e);
                }
            };
    
    function onChange(el){
        var val = el.value;
        var id = el.getAttribute('data-id');
        ws.send(JSON.stringify({action:'update',id:id,value:val}));
    }
    </script>)";
    return html;
}
String generateMetadataPage() {
    String html;
    html += "<h1>Metadata</h1>";
    html += "<p><a href='/'>Index</a> | <a href='/info'>Info</a> | <a href='/advanced'>Advanced</a></p>";
    html += "<table border=1><thead><tr><th>Id</th><th>Name</th><th>Type</th><th>Value</th><th>Description</th><th>ReadOnly</th><th>IsShown</th><th>IsPersisted</th><th>Reorder</th><th>Delete</th></tr></thead><tbody id='meta-body'>";
    for (auto& f : Controller::model.getFields()) {
        html += "<tr>";
        html += "<td>" + f.getId() + "</td>";
        html += "<td>" + f.getName() + "</td>";
        html += "<td>" + f.getType() + "</td>";
        html += "<td>" + f.getValue() + "</td>";
        html += "<td>" + f.getDescription() + "</td>";
        html += "<td>" + String(f.getReadOnly()) + "</td>";
        html += "<td>" + String(f.getIsShown()) + "</td>";
        html += "<td>" + String(f.getIsPersisted()) + "</td>";
        html += "<td><button onclick='reorder(\"" + f.getId() + "\",true)'>&#9650;</button><button onclick='reorder(\"" + f.getId() + "\",false)'>&#9660;</button></td>";
        html += "<td><button onclick='delField(\"" + f.getId() + "\")'>Delete</button></td>";
        html += "</tr>";
    }
    html += "</tbody></table>";
    html += "<h3>Add New Field</h3>";
    html += "ID: <input id='fid'><br>";
    html += "Name: <input id='fname'><br>";
    html += "Type: <input id='ftype'><br>";
    html += "Value: <input id='fvalue'><br>";
    html += "Description: <input id='fdesc'><br>";
    html += "ReadOnly: <input id='freadonly' type='checkbox'><br>";
    html += "IsShown: <input id='isshown' type='checkbox'><br>";
    html += "IsPersisted: <input id='ispersisted' type='checkbox'><br>";
    html += "<button onclick='addField()'>Add Field</button>";
    html += R"rawliteral(
                            <script>
                            var ws = new WebSocket('ws://' + location.hostname + '/ws');
                            ws.onmessage = function(evt){
    try{
        var data = JSON.parse(evt.data);
        if(!Array.isArray(data)) return;
        var tbody = document.querySelector('#meta-body');
        if(!tbody) return;
        tbody.innerHTML = "";
        data.forEach(f=>{
            var row = document.createElement("tr");
            row.innerHTML =
            "<td>"+f.id+"</td>"+
            "<td>"+f.name+"</td>"+
            "<td>"+f.type+"</td>"+
            "<td>"+f.value+"</td>"+
            "<td>"+f.description+"</td>"+
            "<td>"+f.readOnly+"</td>"+
            "<td>"+f.isShown+"</td>"+
            "<td>"+f.isPersisted+"</td>"+            
            "<td><button onclick='reorder(\""+f.id+"\",true)'>&#9650;</button>"+
            "<button onclick='reorder(\""+f.id+"\",false)'>&#9660;</button></td>"+
            "<td><button onclick='delField(\""+f.id+"\")'>Delete</button></td>";
            tbody.appendChild(row);
            });
            }catch(e){console.error(e);}
            };
            function delField(id){ws.send(JSON.stringify({action:'delete',id:id}));}
            function reorder(id,up){ws.send(JSON.stringify({action:up?'moveUp':'moveDown',id:id}));}
            function addField(){
                var fid=document.getElementById('fid').value.trim();
                if(!fid){
                    var maxId=0;
                    document.querySelectorAll('#meta-body tr td:first-child').forEach(td=>{
                        var n=parseInt(td.innerText);
                        if(!isNaN(n)&&n>maxId) maxId=n;
                        });
        fid=(maxId+1).toString();
        document.getElementById('fid').value=fid;
    }
    var msg={action:'add',field:{
        id:fid,
        name:document.getElementById('fname').value,
        type:document.getElementById('ftype').value,
        value:document.getElementById('fvalue').value,
        description:document.getElementById('fdesc').value,
        readOnly:document.getElementById('freadonly').checked
        }};
        ws.send(JSON.stringify(msg));
        // clear inputs after sending
        document.getElementById('fid').value="";
        document.getElementById('fname').value="";
        document.getElementById('ftype').value="";
        document.getElementById('fvalue').value="";
        document.getElementById('fdesc').value="";
        document.getElementById('freadonly').checked=false;
        document.getElementById('isshown').checked=false;
        document.getElementById('ispersisted').checked=true;
        }
        </script>
        )rawliteral";

    return html;
}

String generateAdvancedPage() {
    String html = generateMenu();
    html += "<h1>Advanced</h1>";
    html += "<h3>Device Management</h3>";
    html += "<button onclick='reboot()' style=\"background-color:#333;color:white;\">Reboot ESP32</button>";
    html += "<h3>Upload New Model JSON</h3>";
    html += "<textarea id='jsonInput' rows='10' cols='80' placeholder='Paste new model JSON here'></textarea><br>";
    html += "<button onclick='uploadModel()'>Upload Model</button>";
    html += "<h3>Factory Reset</h3>";
    html += "<button onclick='factoryReset()' style=\"background-color:#f66;color:white;\">Initialize Model to Factory Defaults</button>";
    html += "<h3>Factory Defaults Preview</h3>";
    html += "<button onclick='showFactoryModel()'>Show Current Model</button> ";
    html += "<button onclick='showFactoryJson()'>Show Factory Default Model</button>";

    // ✅ Re-enable the display area for JSON output:
    html += "<h3>Model Output</h3><pre id='model-json' style='background:#eee;padding:10px;border:1px solid #ccc;max-height:400px;overflow:auto;'></pre>";

    html += R"rawliteral(
        <script>
        var ws = new WebSocket('ws://' + location.hostname + '/ws');
        ws.onmessage = function(evt){
            try{
                var data = JSON.parse(evt.data);
                document.getElementById('model-json').innerText = JSON.stringify(data, null, 2);
            }catch(e){
                console.error(e);
                // If parsing fails, just show the raw message
                document.getElementById('model-json').innerText = evt.data;
            }
        };

        function uploadModel(){
            var json = document.getElementById('jsonInput').value.trim();
            if(!json){alert('Please paste JSON first');return;}
            try{JSON.parse(json);}catch(e){alert('Invalid JSON: '+e);return;}
            ws.send(JSON.stringify({action:'uploadModel', json:json}));
            document.getElementById('jsonInput').value = '';
        }

        function factoryReset(){
            if(confirm('Restore factory model? This will overwrite all current fields.')){
                ws.send(JSON.stringify({action:'factoryReset'}));
            }
        }   

        function showFactoryModel(){
            ws.send(JSON.stringify({action:'showFactoryModel'}));
        }

        function showFactoryJson(){
            ws.send(JSON.stringify({action:'showFactoryJson'}));
        }  
        function reboot(){
            if(confirm('Reboot the ESP32 now?')){
                fetch('/reboot');
            }
        }                   
        </script>
    )rawliteral";
    return html;
}

String generateChartPage() {
    String html = generateMenu();
    html += "<h1>Live Temperature Chart</h1>";
    html += "<canvas id='tempChart' width='800' height='400' style='border:1px solid #ccc;'></canvas>";
    html += R"rawliteral(
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <script>
        const ctx = document.getElementById('tempChart').getContext('2d');
        const chartData = {
            labels: [],
            datasets: [{
                label: 'Temperature (°C)',
                data: [],
                fill: false,
                borderColor: 'rgb(255, 99, 132)',
                tension: 0.1
            }]
        };
        const config = {
            type: 'line',
            data: chartData,
            options: {
                responsive: false,
                animation: false,
                scales: {
                    x: { title: { display: true, text: 'Time (s)' } },
                    y: { title: { display: true, text: 'Temperature' } }
                }
            }
        };
        const tempChart = new Chart(ctx, config);

        let startTime = Date.now();
        let lastUpdate = 0;

        // 🔸 Load previous chart state from localStorage
        window.addEventListener('load', () => {
            const saved = localStorage.getItem('tempChartData');
            if (saved) {
                try {
                    const parsed = JSON.parse(saved);
                    chartData.labels = parsed.labels || [];
                    chartData.datasets[0].data = parsed.data || [];
                    tempChart.update();
                    console.log("[Chart] Restored from localStorage (" + chartData.labels.length + " pts)");
                } catch(e) {
                    console.warn("[Chart] Failed to restore chart data:", e);
                }
            }
        });

        const ws = new WebSocket('ws://' + location.hostname + '/ws');
        ws.onmessage = function(evt) {
            const now = Date.now();
            if (now - lastUpdate < 2000) return; // only update every 2 sec
            lastUpdate = now;

            try {
                const data = JSON.parse(evt.data);
                if (!Array.isArray(data)) return;
                const tempField = data.find(f => f.name === 'currentTemperature');
                if (!tempField) return;
                const temp = parseFloat(tempField.value);
                if (isNaN(temp)) return;
                const t = ((Date.now() - startTime) / 1000).toFixed(1);

                chartData.labels.push(t);
                chartData.datasets[0].data.push(temp);

                // keep last 15 hours (54000 points @ 1s)
                if (chartData.labels.length > 54000) {
                    chartData.labels.shift();
                    chartData.datasets[0].data.shift();
                }

                tempChart.update();

                // 🔸 Save chart data every few updates to localStorage
                if (chartData.labels.length % 10 === 0) {
                    localStorage.setItem('tempChartData', JSON.stringify({
                        labels: chartData.labels,
                        data: chartData.datasets[0].data
                    }));
                }
            } catch (e) { console.error(e); }
        };

        // 🔸 Clear storage button (optional)
        function clearChartStorage(){
            localStorage.removeItem('tempChartData');
            alert('Saved chart data cleared.');
        }
        </script>
        <button onclick="clearChartStorage()" style="margin-top:10px;">Clear Saved Data</button>
        )rawliteral";

    return html;
}

void handleWebSocketMessage(String msg) {  // from the UI to board
    JsonDocument doc;
    if (deserializeJson(doc, msg)) return;
    String action = doc["action"] | "";
    if (action == "update") {
        String id = doc["id"] | "";
        String val = doc["value"] | "";
        Field* f = Controller::model.getById(id);
        if (f && !f->getReadOnly()) {
            f->setValue(val);
            Serial.println("[SYS] From UI set value for:" + f->getName() + " value:" + f->getValue());
            if (f->getIsPersisted()) {
                Controller::model.saveToFile();
            }
            Controller::webSocket.textAll(Controller::model.toJsonString());
        }
    } else if (action == "delete") {
        String id = doc["id"] | "";
        if (Controller::model.remove(id)) {
            Controller::model.saveToFile();
            Controller::webSocket.textAll(Controller::model.toJsonString());
        }
    } else if (action == "moveUp" || action == "moveDown") {
        String id = doc["id"] | "";
        Controller::model.reorder(id, action == "moveUp");
        Controller::model.saveToFile();
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "add") {
        JsonObject fld = doc["field"].as<JsonObject>();
        Field f;
        f.fromJson(fld);
        Controller::model.add(f);
        Controller::model.saveToFile();
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "uploadModel") {
        String jsonStr = doc["json"] | "";
        if (JsonWrapper::checkJson(jsonStr)) {
            Controller::model.loadFromJson(jsonStr);
            Controller::model.saveToFile();
            Serial.println("[WEB] Uploaded new model via Advanced page");
        } else {
            Serial.println("[WEB] Invalid JSON upload ignored");
        }
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "factoryReset") {
        Serial.println("[WEB] Factory reset requested from Advanced page");
        Controller::model.initialize();
        Controller::model.saveToFile();
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "showFactoryModel") {
        Serial.println("[WEB] Showing factory default model (object view)");
        Model temp;
        temp.initialize();  // create default fields
        Controller::webSocket.textAll(temp.toJsonString());
    } else if (action == "showFactoryJson") {
        Serial.println("[WEB] Showing factory default model (raw JSON view)");
        Model temp;
        temp.initialize();
        String json = temp.toJsonString();
        Controller::webSocket.textAll(json);
    }
}

// NTP Server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;    // Set your GMT offset in seconds (e.g., EST is -5 hours * 3600 seconds/hour = -18000)
const int daylightOffset_sec = 3600;  // Set your daylight saving offset in seconds (e.g., 1 hour = 3600 seconds)
//
void loopBackendTask(void* param);

//
void setup() {
    Serial.begin(115200);
    Serial.println("[SYS] Booting...");
    WiFi.begin(getSsid(), getPass());
    Serial.printf("[WiFi] Connecting to %s ", getSsid());
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        // Initialize and get the time from NTP server
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        Serial.println(" at:" + Helper::getTime());
        if (MDNS.begin("bio"))
            Serial.println("[mDNS] Registered as bio.local");
        else
            Serial.println("[mDNS] Failed to start mDNS");
    } else
        Serial.println("[WiFi] Connection failed!");
    if (!SPIFFS.begin(false))
        Serial.println("[FS] Mount failed!");
    else
        Serial.println("[FS] Mounted successfully.");
    // TODO stop everyhing if no SPIFF
    Controller::model.load();
    // Controller::model.initialize();
    Serial.println("Controller::model object created and content is:" + Controller::model.toBriefJsonString());
    Serial.println(Controller::Controller::webSocket.url());
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndexPage(true)); });
    server.on("/info", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndexPage(false)); });
    server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateMetadataPage()); });
    server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateAdvancedPage()); });
    server.on("/chart", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateChartPage()); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) {r->send(200,"text/plain","Rebooting...");delay(100);ESP.restart(); });

    Controller::webSocket.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_DATA) {
            String msg;
            for (size_t i = 0; i < len; i++)
                msg += (char)data[i];
            handleWebSocketMessage(msg);
        }
    });
    server.addHandler(&Controller::webSocket);
    server.begin();
    BackEnd::setupBackend();
    xTaskCreate(loopBackendTask, "LoopBackend", 8192, nullptr, 1, nullptr);
    Serial.println("[SYS] Main Setup complete.");
}
void loopBackendTask(void* param) {
    for (;;) {
        // Place the original code from loopBackend here
        BackEnd::loopBackend();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//
bool first = true;
void serialLoop() {
    if (first) {
        Serial.println("serial loop started");
        Serial.println("in loop brief Controller::model via Controller is:" + Controller::model.toBriefJsonString());
        first = false;
    }
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "m")
            Controller::model.listSerial();
        else if (line == "j") {
            Serial.println("memory model is:");
            Serial.println(Controller::model.toJsonString());
        } else if (line.startsWith("upload ")) {
            String jsonStr = line.substring(line.indexOf(' ') + 1);
            jsonStr.trim();
            if (JsonWrapper::checkJson(jsonStr)) {
                Controller::model.loadFromJson(jsonStr);
                Serial.println("New replaced Json:" + String(Controller::model.toJsonString()));  // todo remove
            } else {
                Serial.println("New entered Json does not parse so Model remained unchanged");
            }
            Controller::webSocket.textAll(Controller::model.toJsonString());
        } else if (line.startsWith("add ")) {
            Field f;
            f.setReadOnly(false);
            int idx;
            idx = line.indexOf("name=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setName(line.substring(idx + 5, e));
            }
            idx = line.indexOf("id=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setId(line.substring(idx + 3, e));
            } else {
                Serial.println("TODO should default the id if not present int the field");
            }
            idx = line.indexOf("type=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setType(line.substring(idx + 5, e));
            }
            idx = line.indexOf("value=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setValue(line.substring(idx + 6, e));
            }
            idx = line.indexOf("description=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setDescription(line.substring(idx + 12, e));
            }
            idx = line.indexOf("readonly=");
            if (idx >= 0) {
                f.setReadOnly((line.substring(idx + 9, idx + 10) == "1"));
            }
            idx = line.indexOf("isshown=");
            if (idx >= 0) {
                f.setIsShown((line.substring(idx + 9, idx + 10) == "1"));
            }
            idx = line.indexOf("ispersisted=");
            if (idx >= 0) {
                f.setIsPersisted((line.substring(idx + 9, idx + 10) == "1"));
            }
            if (f.getName() != "") {
                Controller::model.add(f);
                Serial.printf("[SERIAL] Added field %s\n", f.getName().c_str());
                Controller::model.saveToFile();
                Controller::webSocket.textAll(Controller::model.toJsonString());
            }
        } else if (line.startsWith("delete ")) {
            String name = line.substring(7);
            Field* f = Controller::model.getByName(name);
            if (f) {
                Controller::model.remove(f->getId());
                Serial.printf("[SERIAL] Deleted field  %s\n", name.c_str());
                Controller::model.saveToFile();
                Controller::webSocket.textAll(Controller::model.toJsonString());
            }
        } else if (line == "r") {
            Controller::model.initialize();
            Serial.printf("[SERIAL] Reinitialized the whole Controller::model");
        } else if (line.startsWith("reset")) {
            Serial.println("[SERIAL] Resetting ESP32...");
            delay(200);
            ESP.restart();
        } else if (line == "?") {
            Serial.println("\n--- Available Serial Commands ---");
            Serial.println("? or help          : Display this help message");
            Serial.println("m                  : List current Controller::model fields details");
            Serial.println("j                  : Print full Controller::model as JSON string");
            Serial.println("upload <json_str>  : Replace entire Controller::model with new JSON data");
            Serial.println("add name=... id=...: Add a new field (supply all params)");
            Serial.println("delete <name>      : Delete a field by name");
            Serial.println("<name>=<value>     : Update the value of an existing field");
            Serial.println("r                  : reset reinitialize fields with factory setting");
            Serial.println("reset              : Restart the ESP32");
            Serial.println("-----------------------------------\n");
        } else {
            int eq = line.indexOf('=');
            if (eq > 0) {
                String name = line.substring(0, eq);
                String val = line.substring(eq + 1);
                Field* f = Controller::model.getByName(name);
                if (f && !f->getReadOnly()) {
                    f->setValue(val);
                    Controller::model.saveToFile();
                    Serial.printf("[SERIAL] Updated %s = %s\n", f->getName().c_str(), f->getValue().c_str());
                    Controller::webSocket.textAll(Controller::model.toJsonString());
                }
            }
        }
    }
}
//
void loop() {
    serialLoop();
}
