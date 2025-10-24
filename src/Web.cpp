#include "Web.h"

Web::Web(Model& m)
    : server(80), ws("/ws"), model(m) {}

// ... [other methods unchanged] ...

String Web::generateHtmlPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Model Fields</title>
  <meta charset="UTF-8">
  <style>
    table { border-collapse: collapse; width: 80%; }
    td, th { padding: 8px; border: 1px solid #ccc; text-align: left; }
    input { width: 120px; }
    .desc { color: #666; font-size: 0.9em; margin-left: 8px; }
  </style>
</head>
<body>
  <h2>Model Fields</h2>
  <table>
    <tr><th>ID</th><th>Name</th><th>Type</th><th>Value</th></tr>
)rawliteral";

    for (auto& f : model.fields) {
        html += "<tr>";
        html += "<td>" + String(f.id) + "</td>";
        html += "<td>" + f.name + "</td>";
        html += "<td>" + f.type + "</td>";

        // Select input type based on field type
        String inputType = "text";
        String extraAttrs = "";
        if (f.isNumeric())
            inputType = "number";
        else if (f.isBoolean()) {
            inputType = "checkbox";
            extraAttrs = (f.getValue() == "true") ? " checked" : "";
        }

        // Add description span after value
        if (inputType == "checkbox") {
            html += "<td><input id=\"" + f.name + "\" type=\"" + inputType + "\"" + extraAttrs +
                    " onchange=\"sendCheckboxUpdate('" + f.name + "', this.checked)\">" +
                    "<span class='desc'>" + f.description + "</span></td>";
        } else {
            html += "<td><input id=\"" + f.name + "\" type=\"" + inputType + "\" value=\"" + f.getValue() +
                    "\" onchange=\"sendUpdate('" + f.name + "')\">" +
                    "<span class='desc'>" + f.description + "</span></td>";
        }

        html += "</tr>";
    }

    html += R"rawliteral(
  </table>

  <script>
    const ws = new WebSocket(`ws://${window.location.host}/ws`);

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      for (const [key, value] of Object.entries(data)) {
        const input = document.getElementById(key);
        if (!input) continue;
        if (input.type === "checkbox") {
          input.checked = (value === "true");
        } else {
          if (input.value != value) input.value = value;
        }
      }
    };

    function sendUpdate(field) {
      const val = document.getElementById(field).value;
      ws.send(`${field}=${val}`);
    }

    function sendCheckboxUpdate(field, checked) {
      ws.send(`${field}=${checked}`);
    }
  </script>
</body>
</html>
)rawliteral";

    return html;
}
