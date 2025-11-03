// #include "Helper.h"
// #include "Controller.h"

// String Helper::htmlEscape(const String& input) {
//     String s = input;
//     s.replace("&", "&amp;");
//     s.replace("<", "&lt;");
//     s.replace(">", "&gt;");
//     s.replace("\"", "&quot;");
//     s.replace("'", "&#39;");
//     return s;
// }

// String Helper::generateFieldsTable(const std::vector<Field>& fields) {
//     String table = "<table border='1' style='border-collapse: collapse;'>";
//     table += "<tr><th>ID</th><th>Name</th><th>Type</th><th>Value</th></tr>";
//     for (auto& f : fields) {
//         table += "<tr>";
//         table += "<td>" + htmlEscape(f.getId()) + "</td>";
//         table += "<td>" + htmlEscape(f.getName()) + "</td>";
//         table += "<td>" + htmlEscape(f.getType()) + "</td>";
//         table += "<td>" + htmlEscape(f.getValue()) + "</td>";
//         table += "</tr>";
//     }
//     table += "</table>";
//     return table;
// }

// String Helper::generateIndexPage(bool autoRefresh) {
//     String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
//     html += "<title>ESP32 Index</title>";
//     if (autoRefresh) {
//         html += "<meta http-equiv='refresh' content='5'>";
//     }
//     html += "<style>body{font-family:Arial;}table{width:100%;}</style>";
//     html += "</head><body>";
//     html += "<h1>ESP32 Index Page</h1>";
//     html += "<p><a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a></p>";
//     html += "<div id='fields'>" + generateFieldsTable(Controller::getModel().getFields()) + "</div>";
//     html += "</body></html>";
//     return html;
// }

// String Helper::generateMetadataPage() {
//     String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
//     html += "<title>Metadata Editor</title>";
//     html += "<style>body{font-family:Arial;}table{width:100%;}</style>";
//     html += "</head><body>";
//     html += "<h1>Metadata Page</h1>";
//     html += "<p><a href='/'>Index</a> | <a href='/debug'>Debug</a></p>";
//     html += "<div id='metadata'>" + generateFieldsTable(Controller::getModel().getFields()) + "</div>";
//     html += "</body></html>";
//     return html;
// }

// String Helper::generateDebugPage() {
//     String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
//     html += "<title>Debug Page</title>";
//     html += "<style>body{font-family:Arial; white-space: pre-wrap;}</style>";
//     html += "</head><body>";
//     html += "<h1>Debug / Logs</h1>";
//     html += "<p><a href='/'>Index</a> | <a href='/metadata'>Metadata</a></p>";
//     html += "<div id='logs'>[Logs will appear here]</div>";
//     html += "</body></html>";
//     return html;
// }
