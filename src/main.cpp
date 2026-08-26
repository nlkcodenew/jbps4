#include <WiFi.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

const char *SSID = "jbps4";
const char *PASSWORD = "jbps4000";
const IPAddress AP_IP(10, 1, 1, 1);
const IPAddress AP_GATEWAY(10, 1, 1, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

AsyncWebServer server(80);
WiFiUDP udp;
byte dnsBuffer[512];

// Minimal DNS responder: answers ALL queries with AP_IP
void handleDNS() {
    int packetSize = udp.parsePacket();
    if (packetSize < 12) return;

    int len = udp.read(dnsBuffer, sizeof(dnsBuffer));
    if (len < 12) return;

    // Build response in-place
    dnsBuffer[2] = 0x81; // QR=1, Opcode=0, AA=1, TC=0, RD=1
    dnsBuffer[3] = 0x80; // RA=1, Z=0, RCODE=0
    // ANCOUNT = 1
    dnsBuffer[6] = 0x00;
    dnsBuffer[7] = 0x01;

    // Find end of question section (skip QNAME + QTYPE + QCLASS)
    int qnameEnd = 12;
    while (qnameEnd < len && dnsBuffer[qnameEnd] != 0) {
        qnameEnd += dnsBuffer[qnameEnd] + 1;
    }
    qnameEnd++; // skip null terminator
    int questionEnd = qnameEnd + 4; // QTYPE(2) + QCLASS(2)

    // Append answer: pointer to QNAME, type A, class IN, TTL 60, RDLENGTH 4, IP
    int answerStart = questionEnd;
    dnsBuffer[answerStart] = 0xC0;     // pointer to offset 12 (QNAME)
    dnsBuffer[answerStart + 1] = 0x0C;
    dnsBuffer[answerStart + 2] = 0x00; // TYPE A
    dnsBuffer[answerStart + 3] = 0x01;
    dnsBuffer[answerStart + 4] = 0x00; // CLASS IN
    dnsBuffer[answerStart + 5] = 0x01;
    dnsBuffer[answerStart + 6] = 0x00; // TTL = 60
    dnsBuffer[answerStart + 7] = 0x00;
    dnsBuffer[answerStart + 8] = 0x00;
    dnsBuffer[answerStart + 9] = 0x3C;
    dnsBuffer[answerStart + 10] = 0x00; // RDLENGTH = 4
    dnsBuffer[answerStart + 11] = 0x04;
    dnsBuffer[answerStart + 12] = AP_IP[0];
    dnsBuffer[answerStart + 13] = AP_IP[1];
    dnsBuffer[answerStart + 14] = AP_IP[2];
    dnsBuffer[answerStart + 15] = AP_IP[3];

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(dnsBuffer, answerStart + 16);
    udp.endPacket();
}

String getMimeType(const String &path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".mjs")) return "application/javascript";
    if (path.endsWith(".js")) return "application/javascript";
    if (path.endsWith(".bin")) return "application/octet-stream";
    if (path.endsWith(".elf")) return "application/octet-stream";
    if (path.endsWith(".cache")) return "text/cache-manifest";
    if (path.endsWith(".ttf")) return "font/ttf";
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".json")) return "application/json";
    return "text/plain";
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n[PS4 JB] Starting...");

    // Mount LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("[PS4 JB] LittleFS mount failed!");
        return;
    }
    Serial.println("[PS4 JB] LittleFS mounted");

    // WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(SSID, PASSWORD);
    Serial.printf("[PS4 JB] AP started: %s @ %s\n", SSID, WiFi.softAPIP().toString().c_str());

    // DNS server
    udp.begin(53);
    Serial.println("[PS4 JB] DNS server started on port 53");

    // HTTP: serve static files from LittleFS root
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://10.1.1.1/index.html");
    });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://10.1.1.1/index.html");
    });
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://10.1.1.1/index.html");
    });

    // Serve all static files with correct MIME types
    server.onNotFound([](AsyncWebServerRequest *request) {
        String path = request->url();
        if (path == "/") path = "/index.html";

        if (LittleFS.exists(path)) {
            String mime = getMimeType(path);
            request->send(LittleFS, path, mime);
        } else {
            // Captive portal: redirect unknown paths to index
            request->redirect("http://10.1.1.1/index.html");
        }
    });

    server.begin();
    Serial.println("[PS4 JB] HTTP server started on port 80");
    Serial.println("[PS4 JB] Ready! Connect PS4 to WiFi 'jbps4' then open browser.");
}

void loop() {
    handleDNS();
}
