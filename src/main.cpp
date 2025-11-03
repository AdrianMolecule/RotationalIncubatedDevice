// #include <ArduinoJson.h>
// #include <ESPAsyncWebServer.h>
// #include <ESPmDNS.h>
// #include <FS.h>
// #include <SPIFFS.h>
// #include <WebSocketsServer.h>
// #include <WiFi.h>

// #include <vector>

// #include "pass.h"

// // ----------------------
// // Field and Model Classes
// // ----------------------
// class Field {
//    public:
//     String id, name, type, value, description;
//     bool readOnly;

//     void fromJson(const JsonObject& obj) {
//         id = obj["id"] | "";
//         name = obj["name"] | "";
//         type = obj["type"] | "";
//         value = obj["value"] | "";
//         description = obj["description"] | "";
//         readOnly = obj["readOnly"] | false;
//     }

//     void toJson(JsonObject& obj) const {
//         obj["id"] = id;
//         obj["name"] = name;
//         obj["type"] = type;
//         obj["value"] = value;
//         obj["description"] = description;
//         obj["readOnly"] = readOnly;
//     }
// };

// class Model {
//    public:
//     std::vector<Field> fields;

//     Field* getById(const String& id) {
//         for (auto& f : fields)
//             if (f.id == id) return &f;
//         return nullptr;
//     }

//     Field* getByName(const String& name) {
//         for (auto& f : fields)
//             if (f.name == name) return &f;
//         return nullptr;
//     }

//     void add(Field f) { fields.push_back(f); }

//     bool remove(const String& id) {
//         for (size_t i = 0; i < fields.size(); i++) {
//             if (fields[i].id == id) {
//                 fields.erase(fields.begin() + i);
//                 return true;
//             }
//         }
//         return false;
//     }

//     void reorder(const String& id, bool up) {
//         for (size_t i = 0; i < fields.size(); i++) {
//             if (fields[i].id == id) {
//                 if (up && i > 0)
//                     std::swap(fields[i], fields[i - 1]);
//                 else if (!up && i < fields.size() - 1)
//                     std::swap(fields[i], fields[i + 1]);
//                 break;
//             }
//         }
//     }

//     bool load() {
//         if (!SPIFFS.exists("/model.json")) return false;
//         File f = SPIFFS.open("/model.json", "r");
//         if (!f) return false;
//         size_t size = f.size();
//         if (size == 0) {
//             f.close();
//             return false;
//         }
//         std::unique_ptr<char[]> buf(new char[size + 1]);
//         f.readBytes(buf.get(), size);
//         buf[size] = 0;
//         f.close();
//         DynamicJsonDocument doc(4096);
//         if (deserializeJson(doc, buf.get())) return false;
//         fields.clear();
//         for (JsonObject fld : doc["fields"].as<JsonArray>()) {
//             Field f;
//             f.fromJson(fld);
//             fields.push_back(f);
//         }
//         return !fields.empty();
//     }

//     bool save() {
//         DynamicJsonDocument doc(4096);
//         JsonArray arr = doc.createNestedArray("fields");
//         for (auto& f : fields) {
//             JsonObject obj = arr.createNestedObject();
//             f.toJson(obj);
//         }
//         File f = SPIFFS.open("/model.json", "w");
//         if (!f) return false;
//         serializeJson(doc, f);
//         f.close();
//         return true;
//     }

//     String toJsonString() {
//         DynamicJsonDocument doc(4096);
//         JsonArray arr = doc.createNestedArray("fields");
//         for (auto& f : fields) {
//             JsonObject obj = arr.createNestedObject();
//             f.toJson(obj);
//         }
//         String s;
//         serializeJson(doc, s);
//         return s;
//     }

//     void listSerial() {
//         for (auto& f : fields) Serial.printf("%s (%s) = %s\n", f.name.c_str(), f.type.c_str(), f.value.c_str());
//     }
// };

// Model model;

// // ----------------------
// // Servers
// // ----------------------
// AsyncWebServer server(80);
// WebSocketsServer webSocket(81);

// // ----------------------
// // Web Pages
// // ----------------------
// String generateMenu() {
//     return "<p><a href='/'>Index</a> | <a href='/info'>Info</a>| <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a> | <a href='/reboot'>Reboot</a></p>";
// }

// String generateIndexPage(bool brief) {
//     String html = generateMenu();
//     html += "<h1>Index Page</h1><table border=1><tr><th>Name</th><th>Type</th><th>Value</th><th>Description</th></tr>";
//     for (auto& f : model.fields) {
//         if (brief && f.readOnly) {
//             continue;
//         }
//         html += "<tr><td>" + f.name + "</td><td>" + f.type + "</td>";
//         if (f.readOnly)
//             html += "<td><input value='" + f.value + "' disabled></td>";
//         else
//             html += "<td><input data-id='" + f.id + "' value='" + f.value + "' onchange='onChange(this)'></td>";
//         html += "<td>" + f.description + "</td></tr>";
//     }
//     html += R"(
//     <script>
//     function onChange(el){
//         var val = el.value;
//         var id = el.getAttribute('data-id');
//         ws.send(JSON.stringify({action:'update',id:id,value:val}));
//     }
//     var ws = new WebSocket('ws://'+location.hostname+':81/');
//     ws.onmessage=function(evt){ location.reload(); }
//     </script>
//     )";
//     return html;
// }

// String generateMetadataPage() {
//     String html = generateMenu();
//     html += "<h1>Metadata</h1><table border=1><tr><th>Name</th><th>Type</th><th>Value</th><th>Description</th><th>ReadOnly</th><th>Reorder</th><th>Delete</th></tr>";
//     for (auto& f : model.fields) {
//         html += "<tr>";
//         html += "<td>" + f.name + "</td><td>" + f.type + "</td><td>" + f.value + "</td><td>" + f.description + "</td>";
//         html += "<td>" + String(f.readOnly) + "</td>";
//         html += "<td><button onclick='reorder(\"" + f.id + "\",true)'>&#9650;</button> <button onclick='reorder(\"" + f.id + "\",false)'>&#9660;</button></td>";
//         html += "<td><button onclick='delField(\"" + f.id + "\")'>Delete</button></td>";
//         html += "</tr>";
//     }
//     html += "</table><h3>Add New Field</h3>";
//     html += "ID: <input id='fid'><br>Name: <input id='fname'><br>Type: <input id='ftype'><br>Value: <input id='fvalue'><br>Description: <input id='fdesc'><br>ReadOnly: <input id='freadonly' type='checkbox'><br>";
//     html += "<button onclick='addField()'>Add Field</button>";
//     html += R"(
//     <script>
//     var ws = new WebSocket('ws://'+location.hostname+':81/');
//     function delField(id){ ws.send(JSON.stringify({action:'delete',id:id})); }
//     function reorder(id,up){ ws.send(JSON.stringify({action:up?'moveUp':'moveDown',id:id})); }
//     function addField(){
//         var msg={action:'add',field:{id:document.getElementById('fid').value,
//                                     name:document.getElementById('fname').value,
//                                     type:document.getElementById('ftype').value,
//                                     value:document.getElementById('fvalue').value,
//                                     description:document.getElementById('fdesc').value,
//                                     readOnly:document.getElementById('freadonly').checked}};
//         ws.send(JSON.stringify(msg));
//     }
//     ws.onmessage=function(evt){ location.reload(); }
//     </script>
//     )";
//     return html;
// }

// String generateDebugPage() {
//     String html = generateMenu();
//     html += "<h1>Debug</h1><pre>" + model.toJsonString() + "</pre>";
//     return html;
// }

// // ----------------------
// // BackEnd Class
// // ----------------------
// class BackEnd {
//    public:
//     static unsigned long durationSinceReboot;

//     static void setupBackend() {
//         durationSinceReboot = 0;
//         Serial.println("[BACKEND] Backend initialized.");
//     }

//     static void loopBackend() {
//         while (true) {
//             durationSinceReboot = millis() / 1000;

//             static unsigned long lastModelUpdate = 0;
//             if (durationSinceReboot - lastModelUpdate >= 5) {
//                 lastModelUpdate = durationSinceReboot;
//                 Serial.println("[BACKEND] 100 second stop start.");
//                 delay(100000);  // ensure we are in the next second
//                 Serial.println("[BACKEND] finished 100 second stop.");
//                 Field* f = model.getByName("duration");
//                 if (!f) {
//                     Field nf;
//                     nf.id = "10";
//                     nf.name = "duration";
//                     nf.type = "string";
//                     nf.value = String(durationSinceReboot);
//                     nf.description = "time since start";
//                     nf.readOnly = true;
//                     model.add(nf);
//                     model.save();
//                     webSocket.broadcastTXT(model.toJsonString().c_str());
//                     Serial.printf("[BACKEND] Created field 'duration' = %s\n", nf.value.c_str());
//                 } else {
//                     f->value = String(durationSinceReboot);
//                     model.save();
//                     webSocket.broadcastTXT(model.toJsonString().c_str());
//                     Serial.printf("[BACKEND] Updated field 'duration' = %s\n", f->value.c_str());
//                 }
//             }
//             delay(1000);
//         }
//     }
// };
// unsigned long BackEnd::durationSinceReboot = 0;

// // ----------------------
// // Setup
// // ----------------------
// void setup() {
//     Serial.begin(115200);
//     delay(1000);
//     Serial.println("Booting...");

//     WiFi.begin(getSsid(), getPass());
//     Serial.print("Connecting to Wi-Fi");
//     int timeout = 0;
//     while (WiFi.status() != WL_CONNECTED && timeout < 20) {
//         delay(500);
//         Serial.print(".");
//         timeout++;
//     }
//     Serial.println();
//     if (WiFi.status() == WL_CONNECTED) {
//         Serial.println("[WiFi] Connected.");
//         Serial.print("[WiFi] IP: ");
//         Serial.println(WiFi.localIP());
//         if (MDNS.begin("bio"))
//             Serial.println("[mDNS] Registered as bio.local");
//         else
//             Serial.println("[mDNS] Failed to start mDNS");
//     } else
//         Serial.println("[WiFi] Connection failed!");

//     if (!SPIFFS.begin(false))
//         Serial.println("[SPIFFS] Mount failed!");
//     else
//         Serial.println("[SPIFFS] Mounted successfully.");

//     if (!model.load()) {
//         Serial.println("[MODEL] Loading factory default");
//         Field f1{"1", "Temperature", "float", "22.5", "Room temp", false};
//         Field f2{"2", "Enabled", "bool", "true", "System enabled", false};
//         Field f3{"3", "DeviceID", "string", "ESP32-001", "Identifier", true};
//         Field f4{"4", "Count", "int", "42", "Event counter", false};
//         model.add(f1);
//         model.add(f2);
//         model.add(f3);
//         model.add(f4);
//         model.save();
//     }

//     server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndexPage(true)); });
//     server.on("/info", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndexPage(false)); });
//     server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateMetadataPage()); });
//     server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateDebugPage()); });
//     server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200,"text/plain","Rebooting..."); delay(100); ESP.restart(); });

//     server.begin();

//     webSocket.begin();
//     webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
//         if (type == WStype_TEXT) {
//             DynamicJsonDocument doc(1024);
//             deserializeJson(doc, payload);
//             String action = doc["action"] | "";
//             if (action == "update") {
//                 String id = doc["id"] | "";
//                 String val = doc["value"] | "";
//                 Field* f = model.getById(id);
//                 if (f && !f->readOnly) {
//                     f->value = val;
//                     model.save();
//                     Serial.printf("[WEB] Updated %s = %s\n", f->name.c_str(), f->value.c_str());
//                     webSocket.broadcastTXT(model.toJsonString().c_str());
//                 }
//             } else if (action == "delete") {
//                 String id = doc["id"] | "";
//                 if (model.remove(id)) {
//                     model.save();
//                     webSocket.broadcastTXT(model.toJsonString().c_str());
//                 }
//             } else if (action == "moveUp") {
//                 String id = doc["id"] | "";
//                 model.reorder(id, true);
//                 model.save();
//                 webSocket.broadcastTXT(model.toJsonString().c_str());
//             } else if (action == "moveDown") {
//                 String id = doc["id"] | "";
//                 model.reorder(id, false);
//                 model.save();
//                 webSocket.broadcastTXT(model.toJsonString().c_str());
//             } else if (action == "add") {
//                 JsonObject fld = doc["field"].as<JsonObject>();
//                 Field f;
//                 f.fromJson(fld);
//                 model.add(f);
//                 model.save();
//                 webSocket.broadcastTXT(model.toJsonString().c_str());
//                 Serial.printf("[WEB] Added new field %s (%s)\n", f.name.c_str(), f.type.c_str());
//             }
//         }
//     });

//     Serial.println("[SERIAL] Ready for commands (? , j , FieldName=value , F ... , delete FieldName)");

//     // Setup backend
//     BackEnd::setupBackend();
//     xTaskCreatePinnedToCore(
//         [](void*) { BackEnd::loopBackend(); },
//         "BackendTask",
//         4096,
//         nullptr,
//         1,
//         nullptr,
//         1  // <-- pinned to Core 1
//     );
// }

// // ----------------------
// // Loop
// // ----------------------
// void loop() {
//     webSocket.loop();
//     if (Serial.available()) {
//         String line = Serial.readStringUntil('\n');
//         line.trim();
//         if (line == "?")
//             model.listSerial();
//         else if (line == "j")
//             Serial.println(model.toJsonString());
//         else if (line.startsWith("F ")) {
//             Field f;
//             f.readOnly = false;
//             int idx;
//             idx = line.indexOf("name=");
//             if (idx >= 0) {
//                 int e = line.indexOf(" ", idx);
//                 if (e < 0) e = line.length();
//                 f.name = line.substring(idx + 5, e);
//             }
//             idx = line.indexOf("id=");
//             if (idx >= 0) {
//                 int e = line.indexOf(" ", idx);
//                 if (e < 0) e = line.length();
//                 f.id = line.substring(idx + 3, e);
//             }
//             idx = line.indexOf("type=");
//             if (idx >= 0) {
//                 int e = line.indexOf(" ", idx);
//                 if (e < 0) e = line.length();
//                 f.type = line.substring(idx + 5, e);
//             }
//             idx = line.indexOf("value=");
//             if (idx >= 0) {
//                 int e = line.indexOf(" ", idx);
//                 if (e < 0) e = line.length();
//                 f.value = line.substring(idx + 6, e);
//             }
//             idx = line.indexOf("description=");
//             if (idx >= 0) {
//                 int e = line.indexOf(" ", idx);
//                 if (e < 0) e = line.length();
//                 f.description = line.substring(idx + 12, e);
//             }
//             idx = line.indexOf("readonly=");
//             if (idx >= 0) {
//                 f.readOnly = (line.substring(idx + 9, idx + 10) == "1");
//             }
//             if (f.name != "") {
//                 model.add(f);
//                 model.save();
//                 Serial.printf("[SERIAL] Added field %s\n", f.name.c_str());
//                 webSocket.broadcastTXT(model.toJsonString().c_str());
//             }
//         } else if (line.startsWith("delete ")) {
//             String name = line.substring(7);
//             Field* f = model.getByName(name);
//             if (f) {
//                 model.remove(f->id);
//                 model.save();
//                 Serial.printf("[SERIAL] Deleted field %s\n", name.c_str());
//                 webSocket.broadcastTXT(model.toJsonString().c_str());
//             }
//         } else {
//             int eq = line.indexOf('=');
//             if (eq > 0) {
//                 String name = line.substring(0, eq);
//                 String val = line.substring(eq + 1);
//                 Field* f = model.getByName(name);
//                 if (f && !f->readOnly) {
//                     f->value = val;
//                     model.save();
//                     Serial.printf("[SERIAL] Updated %s = %s\n", f->name.c_str(), f->value.c_str());
//                     webSocket.broadcastTXT(model.toJsonString().c_str());
//                 }
//             }
//         }
//     }
// }
