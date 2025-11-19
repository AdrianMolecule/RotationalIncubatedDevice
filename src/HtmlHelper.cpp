#include "HtmlHelper.h"
#include "Controller.h"

String HtmlHelper::generateMenu() {
    return "<p>"
           "<a href='/'>Status</a> | "
           "<a href='/extended'>Extended</a> | "
           "<a href='/metadata'>Metadata</a> | "
           "<a href='/advanced'>Advanced</a> | "
           "<a href='/chart'>Chart</a> | "
           "</p>";
}

String HtmlHelper::generateStatusPage(bool brief) {
    String html = HtmlHelper::generateMenu();
    std::vector<Field> fi;
    if (brief) {
        html += "<h2>The Status</h2>";
        fi = Controller::model.getScreenFields();
    } else {
        html += "<h2>Extended Page</h2>";
        fi = Controller::model.getFields();
    }

    html += R"rawliteral(
    <style>
    table { border-collapse: collapse; width: 100%; margin-top: 10px; }
    th, td { border: 1px solid #999; padding: 6px 10px; text-align: left; }
    thead { background-color: #f0f0f0; }
    tbody td input { width: 100%; box-sizing: border-box; }
    </style>
    <table>
        <thead>
            <tr><th>Name</th><th>Type</th><th>Value</th><th>Description</th></tr>
        </thead>
        <tbody id="data-body">
    )rawliteral";

    // Initial table rows
    for (auto& f : fi) {
        html += "<tr>";
        html += "<td>" + f.getName() + "</td>";
        html += "<td>" + f.getType() + "</td>";
        // VALUE CELL WITH TYPE-AWARE INPUT
        html += "<td>";
        String t = f.getType();
        if (t == "bool") {
            String checked = (f.getValue() == "1" || f.getValue() == "true") ? "checked" : "";
            html += "<input type='checkbox' data-id='" + f.getId() + "' " + checked;
            if (f.getReadOnly()) html += " disabled";
            html += ">";
        } else if (t == "int" || t == "float") {
            html += "<input type='number' data-id='" + f.getId() + "' value='" + f.getValue() + "'";
            if (f.getReadOnly()) html += " disabled";
            html += ">";
        } else {
            html += "<input type='text' data-id='" + f.getId() + "' value='" + f.getValue() + "'";
            if (f.getReadOnly()) html += " disabled";
            html += ">";
        }
        html += "</td>";

        html += "<td>" + f.getDescription() + "</td>";
        html += "</tr>";
    }

    html += R"rawliteral(
        </tbody>
    </table>
    <script>
    const ws = new WebSocket('ws://' + location.hostname + '/ws');

    // Attach onchange listeners to inputs
    function attachInputListeners() {
        document.querySelectorAll("input[data-id]").forEach(el => {
            if (!el.dataset.listenerAttached) {
                el.addEventListener("change", () => {
                    let v = (el.type === "checkbox") ? (el.checked ? "1" : "0") : el.value;
                    ws.send(JSON.stringify({ action: "update", id: el.getAttribute("data-id"), value: v }));
                });
                el.dataset.listenerAttached = "true";
            }
        });
    }
    attachInputListeners();

    ws.onmessage = function(evt) {
        try {
            const data = JSON.parse(evt.data);
            if (!Array.isArray(data)) return;

            // Update only existing input values (preserve user edits)
            data.forEach(f => {
                const el = document.querySelector("input[data-id='" + f.id + "']");
                if (el && document.activeElement !== el) {
                    if (el.type === "checkbox") {
                        let shouldCheck = (f.value === "1" || f.value === "true");
                        if (el.checked !== shouldCheck) el.checked = shouldCheck;
                    } else {
                        if (el.value !== f.value) el.value = f.value;
                    }
                }
            });
        } catch(e) {
            console.error("WS update error:", e);
        }
    };
    </script>
    )rawliteral";

    return html;
}

String HtmlHelper::generateMetadataPage() {
    String html = HtmlHelper::generateMenu();
    html += "<h2>Metadata</h2>";
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
    html += "IsShown: <input id='fisshown' type='checkbox'><br>";
    html += "IsPersisted: <input id='fispersisted' type='checkbox'><br>";
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
    try {
        var msg={action:'add',field:{
            id:fid,
            name:document.getElementById('fname').value,
            type:document.getElementById('ftype').value,
            value:document.getElementById('fvalue').value,
            description:document.getElementById('fdesc').value,
            readOnly:document.getElementById('freadonly').checked,
            isShown:document.getElementById('fisshown').checked,
            isPersisted:document.getElementById('fispersisted').checked
            }};
        } catch (error) {
            alert(error.message);
        }
        //alert(" added field\n"+JSON.stringify(msg, null, 4));
        ws.send(JSON.stringify(msg));
        // clear inputs after sending
        document.getElementById('fid').value="";
        document.getElementById('fname').value="";
        document.getElementById('ftype').value="";
        document.getElementById('fvalue').value="";
        document.getElementById('fdesc').value="";
        document.getElementById('freadonly').checked=false;
        document.getElementById('fisshown').checked=false;
        document.getElementById('fispersisted').checked=true;
        }
        </script>
        )rawliteral";

    return html;
}

String HtmlHelper::generateAdvancedPage() {
    String html = HtmlHelper::generateMenu();
    html += "<h2>Advanced Device Management</h2><hl>";
    html += "<button onclick='reboot()' style=\"background-color:#333;color:white;\">Reboot ESP32</button><hl>";
    html += "<h3>Upload New Model JSON</h3>";
    html += "<textarea id='jsonInput' rows='10' cols='80' placeholder='Paste new model JSON here'></textarea><br>";
    html += "<button onclick='uploadModel()'>Upload Model</button><hl>";
    html += "<h3>Factory Reset</h3><hl>";
    html += "<button onclick='factoryReset()' style=\"background-color:#f66;color:white;\">Initialize Model to Factory Defaults and Save it in persistent store</button>";
    html += "<h3>Factory Defaults Preview</h3><hl>";
    html += "<button onclick='showFactoryModel()'>Show Current Model</button><hl> ";
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
String HtmlHelper::generateChartPage() {
    String html = generateMenu();
    html += "<h2>Live Temperature Chart</h2>";
    html += "<canvas id='tempChart' width='800' height='400' style='border:1px solid #ccc;'></canvas>";
    html += R"rawliteral(
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-date-fns"></script>
        <script>
        const ctx = document.getElementById('tempChart').getContext('2d');

        const chartData = {
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
                    x: {
                        type: 'time',
                        time: {
                            unit: 'second',
                            tooltipFormat: 'HH:mm:ss',
                            displayFormats: {
                                second: 'HH:mm:ss'
                            }
                        },
                        title: { display: true, text: 'Time' }
                    },
                    y: { title: { display: true, text: 'Temperature (°C)' } }
                }
            }
        };

        const tempChart = new Chart(ctx, config);
        let lastUpdate = 0;

        // Load previous chart state from localStorage
        window.addEventListener('load', () => {
            const saved = localStorage.getItem('tempChartData');
            if (saved) {
                try {
                    const parsed = JSON.parse(saved);
                    chartData.datasets[0].data = parsed.data || [];
                    tempChart.update();
                    console.log("[Chart] Restored from localStorage (" + chartData.datasets[0].data.length + " pts)");
                } catch(e) {
                    console.warn("[Chart] Failed to restore chart data:", e);
                }
            }
        });

        const ws = new WebSocket('ws://' + location.hostname + '/ws');
        ws.onmessage = function(evt) {
            const now = Date.now();
            if (now - lastUpdate < 2000) return; // update every 2 sec
            lastUpdate = now;

            try {
                const data = JSON.parse(evt.data);
                if (!Array.isArray(data)) return;
                const tempField = data.find(f => f.name === 'currentTemperature');
                if (!tempField) return;
                const temp = parseFloat(tempField.value);
                if (isNaN(temp)) return;

                // push point with real timestamp
                chartData.datasets[0].data.push({ x: now, y: temp });

                // keep last 15 hours (~54000 points @ 1s)
                if (chartData.datasets[0].data.length > 54000) {
                    chartData.datasets[0].data.shift();
                }

                tempChart.update();

                // Save chart data every 10 updates
                if (chartData.datasets[0].data.length % 10 === 0) {
                    localStorage.setItem('tempChartData', JSON.stringify({
                        data: chartData.datasets[0].data
                    }));
                }
            } catch (e) { console.error(e); }
        };

        // Clear storage button
        function clearChartStorage() {
            localStorage.removeItem('tempChartData');
            alert('Saved chart data cleared.');
        }
        </script>
        <button onclick="clearChartStorage()" style="margin-top:10px;">Clear Saved Data</button>
    )rawliteral";

    return html;
}
