#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

const char* ssid = "Darius's iPhone";     
const char* password = "ardusefu";  

#define PIN_RELEU 14      
#define PIN_DHT 26        
#define PIN_SOL 34        

// Setari Senzori
#define DHTTYPE DHT11     
int prag_uscat = 30;      // Daca umiditatea solului scade sub 30%, uda singur

// Variabile sistem
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);

float temperatura = 0;
float umiditate_aer = 0;
int umiditate_sol_procent = 0;
bool mod_auto = true;     // Pornim in mod AUTOMAT
String stare_pompa = "OPRITA";

String getHTML() {
  String ptr = "<!DOCTYPE html><html><head>";
  ptr += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  ptr += "<title>Sistem Irigare Smart</title>";
  ptr += "<style>body{font-family:Arial; text-align:center; margin-top:20px; background-color:#f4f4f4;}";
  ptr += "h2{color:#333;} .card{background:white; padding:20px; margin:10px auto; max-width:400px; border-radius:10px; box-shadow:0 4px 8px rgba(0,0,0,0.1);}";
  ptr += ".val{font-size:22px; font-weight:bold; color:#007bff;}";
  ptr += ".btn{display:inline-block; padding:12px 25px; font-size:16px; text-decoration:none; border-radius:5px; margin:5px; color:white; font-weight:bold;}";
  ptr += ".on{background-color:#28a745;} .off{background-color:#dc3545;} .auto{background-color:#ffc107; color:black;}";
  ptr += "</style></head><body>";

  ptr += "<h2>Monitor Irigare</h2>";
  
  // Card Senzori
  ptr += "<div class='card'>";
  ptr += "<p>Umiditate Sol: <span class='val'>" + String(umiditate_sol_procent) + "%</span></p>";
  ptr += "<p>Temperatura: <span class='val'>" + String(temperatura) + " &deg;C</span></p>";
  ptr += "<p>Umiditate Aer: <span class='val'>" + String(umiditate_aer) + "%</span></p>";
  ptr += "</div>";

  // Card Status
  ptr += "<div class='card'>";
  ptr += "<p>Pompa este: <b>" + stare_pompa + "</b></p>";
  ptr += "<p>Mod Actual: <b>" + String(mod_auto ? "AUTOMAT (Senzor)" : "MANUAL (Telefon)") + "</b></p>";
  ptr += "</div>";

  // Card Butoane
  ptr += "<div class='card'>";
  ptr += "<h3>Control Manual</h3>";
  ptr += "<a href=\"/pompa_on\" class=\"btn on\">PORNESTE</a>";
  ptr += "<a href=\"/pompa_off\" class=\"btn off\">OPRESTE</a>";
  ptr += "<hr>";
  ptr += "<h3>Setare Mod</h3>";
  ptr += "<a href=\"/auto_on\" class=\"btn auto\">Mod AUTOMAT</a>";
  ptr += "<a href=\"/auto_off\" class=\"btn off\" style='background-color:#6c757d;'>Mod MANUAL</a>";
  ptr += "</div>";
  
  ptr += "</body></html>";
  return ptr;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RELEU, OUTPUT);
  digitalWrite(PIN_RELEU, HIGH); // Pornim cu pompa oprita (HIGH = Oprit la relee)
  pinMode(PIN_SOL, INPUT);
  
  // Pornire DHT
  dht.begin();

  // Conectare Wi-Fi
  Serial.print("Ma conectez la Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectat! Intra pe adresa: ");
  Serial.println(WiFi.localIP()); 

  server.on("/", []() { server.send(200, "text/html", getHTML()); });
  
  server.on("/pompa_on", []() {
    mod_auto = false; // Trecem pe manual
    digitalWrite(PIN_RELEU, LOW); // LOW = Porneste Pompa
    stare_pompa = "PORNITA (Manual)";
    server.send(200, "text/html", getHTML());
    delay(100);
  });

  server.on("/pompa_off", []() {
    mod_auto = false; // Ramanem pe manual
    digitalWrite(PIN_RELEU, HIGH); // HIGH = Opreste Pompa
    stare_pompa = "OPRITA (Manual)";
    server.send(200, "text/html", getHTML());
    delay(100);
  });

  server.on("/auto_on", []() {
    mod_auto = true;
    server.send(200, "text/html", getHTML());
  });
  
  server.on("/auto_off", []() {
    mod_auto = false;
    stare_pompa = "ASTEPTARE (Manual)";
    digitalWrite(PIN_RELEU, HIGH); // Oprim pompa preventiv
    server.send(200, "text/html", getHTML());
  });

  server.begin();
}

void loop() {
  server.handleClient(); // Asculta comenzile de pe telefon

  // Citire Senzori
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  if (!isnan(t)) temperatura = t;
  if (!isnan(h)) umiditate_aer = h;
  
  // Citire Sol (Convertim din 4095-0 in 0-100%)
  int analogVal = analogRead(PIN_SOL);
  umiditate_sol_procent = map(analogVal, 4095, 1600, 0, 100); 
  // Limite 0-100%
  if(umiditate_sol_procent < 0) umiditate_sol_procent = 0;
  if(umiditate_sol_procent > 100) umiditate_sol_procent = 100;

  // Daca suntem pe AUTO, decide ESP-ul
  if (mod_auto == true) {
    if (umiditate_sol_procent < prag_uscat) {
      digitalWrite(PIN_RELEU, LOW); // Porneste Pompa
      stare_pompa = "UDARE (Auto)";
    } else {
      digitalWrite(PIN_RELEU, HIGH); // Opreste Pompa
      stare_pompa = "Standby (Umed)";
    }
  }

  delay(200); // Mica pauza
}