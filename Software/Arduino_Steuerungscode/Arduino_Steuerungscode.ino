#include <WiFi.h>
#include <WiFiUdp.h>
#include <SCServo.h>

#define S_RXD 14
#define S_TXD 27

// -------------------- WLAN --------------------
const char* WIFI_SSID = "XXX"; //Hotsot-Name
const char* WIFI_PASS = "XXX"; //Hotspot-Passwort

WiFiUDP Udp;
const uint16_t RX_PORT = 4210;

// -------------------- Protokoll --------------------
const uint8_t MAGIC   = 0xA7;
const uint8_t VERSION = 1;
const uint8_t TANK_ID = 1;

// -------------------- Failsafe --------------------
const uint32_t FAILSAFE_MS = 300;
uint32_t lastPacketMs = 0;

// -------------------- Servo --------------------
SMS_STS st;
byte ID[2];

// -------------------- Stepper --------------------
int IN1 = 25;
int IN2 = 33;
int IN3 = 32;
int IN4 = 26;

// -------------------- Empfangenes Steuerkommando --------------------
int16_t axisY  = 0;   // vor/zurück
int16_t axisX  = 0;   // links/rechts fahren
int16_t axisRX = 0;   // Turm

#pragma pack(push, 1)
struct CtrlPacket {
  uint8_t magic;
  uint8_t version;
  uint8_t tank_id;
  uint8_t seq;
  int16_t axisY_be;
  int16_t axisX_be;
  int16_t axisRX_be;
};
#pragma pack(pop)

static_assert(sizeof(CtrlPacket) == 10, "Packet size mismatch");

// ------------------------------------------------------------
// Big-Endian -> ESP32-Format umwandeln
// ------------------------------------------------------------
int16_t swap16(int16_t v) {
  uint16_t u = (uint16_t)v;
  u = (u >> 8) | (u << 8);
  return (int16_t)u;
}

// ------------------------------------------------------------
// Hilfsfunktion: alles stoppen
// ------------------------------------------------------------
void stopAll() {
  // Stepper aus
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  // Servos stoppen
  st.WriteSpe(1, 0);
  st.WriteSpe(2, 0);
}

// ------------------------------------------------------------
// UDP-Paket lesen
// ------------------------------------------------------------
bool readUdpPacket() {
  int len = Udp.parsePacket();
  if (len <= 0) return false;

  if (len != sizeof(CtrlPacket)) {
    while (Udp.available()) {
      Udp.read();
    }
    return false;
  }

  CtrlPacket pkt;
  int r = Udp.read((uint8_t*)&pkt, sizeof(pkt));
  if (r != sizeof(pkt)) return false;

  if (pkt.magic != MAGIC)   return false;
  if (pkt.version != VERSION) return false;
  if (pkt.tank_id != TANK_ID) return false;

  // Wichtig: Python sendet Big Endian ("!")
  axisY  = swap16(pkt.axisY_be);
  axisX  = swap16(pkt.axisX_be);
  axisRX = swap16(pkt.axisRX_be);

  lastPacketMs = millis();
  return true;
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {
  Serial.begin(1000000);
  Serial.println("ESP32 startet...");

  // Stepper Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Servo Setup
  Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
  st.pSerial = &Serial1;

  ID[0] = 1;
  ID[1] = 2;

  st.WheelMode(1);
  st.WheelMode(2);

  stopAll();

  // WLAN
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Verbinde mit WLAN");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WLAN verbunden");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // UDP starten
  Udp.begin(RX_PORT);
  Serial.print("UDP lauscht auf Port ");
  Serial.println(RX_PORT);
}

// ------------------------------------------------------------
// Loop
// ------------------------------------------------------------
void loop() {
  readUdpPacket();

  // Failsafe: wenn keine aktuellen Pakete mehr kommen -> stoppen
  if (millis() - lastPacketMs > FAILSAFE_MS) {
    stopAll();
    delay(2);
    return;
  }

  // Debug-Ausgabe
  Serial.print("axisX: ");
  Serial.print(axisX);
  Serial.print("   axisY: ");
  Serial.print(axisY);
  Serial.print("   axisRX: ");
  Serial.println(axisRX);

  // ----------------------------------------------------------
  // Stepper-Steuerung für Turm
  // ----------------------------------------------------------
  if (axisRX > 100) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
    delay(2);
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
    delay(2);
    digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    delay(2);
    digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
    delay(2);
  }
  else if (axisRX < -100) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
    delay(2);
    digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
    delay(2);
    digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    delay(2);
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
    delay(2);
  }
  else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  // ----------------------------------------------------------
  // Servo-Steuerung Fahren
  // axisY = vor/zurück
  // axisX = links/rechts
  //st.WrireSpe(1) ist rechtes Rad mit >0 für vorwärts
  //st.WrireSpe(2) ist linkes Rad mit <0 für vorwärts
  // ----------------------------------------------------------
  if (axisY < -250) {
      if (axisX < 450 && axisX > -450){
        st.WriteSpe(1, 3400);
        st.WriteSpe(2, -3400);//vorwärts
        }
      else if (axisX < -450){
        st.WriteSpe(1, 3400);
        st.WriteSpe(2, -1700);//vorwärts-links
      }
      else if (axisX > 450){
        st.WriteSpe(1, 1700);
        st.WriteSpe(2, -3400);//vorwärts-rechts
      }
  }

    else if (axisY > 250){
      if (axisX < 450 && axisX > -450){
        st.WriteSpe(1, -3400);
        st.WriteSpe(2, 3400);//rückwärts
      }
      else if (axisX < -450){
        st.WriteSpe(1, -3400);
        st.WriteSpe(2, 1700);//rückwärts-links
      }
      else if (axisX > 450){
        st.WriteSpe(1, -1700);
        st.WriteSpe(2, 3400);//rückwärts-rechts
      }
    }
    else if (axisX > 500 && axisY < 250 && axisY > -250){
      st.WriteSpe(1, -1700);
      st.WriteSpe(2, -1700);//drehen rechts
      }
    else if (axisX < -500 && axisY < 250 && axisY > -250){
      st.WriteSpe(1, 1700);
      st.WriteSpe(2, 1700); //drehen links
      }
  else{
    st.WriteSpe(1, 0);
    st.WriteSpe(2, 0);
  }

  delay(2);
}

