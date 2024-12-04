#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "Workshop-08";
const char* password = "admin.admin";

const char* mqtt_server = "iotmqtt.online";
const char* mqtt_username = "public";
const char* mqtt_password = "1";

const int gsrPin = 32;
const int relay = 26;
const int pwmPin = 25;
int gsrValue = 0;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long therapyStartTime = 0;
const unsigned long therapyDuration = 60 * 1000;
bool therapyActive = false;

int voltageLevel = 0;
const int maxVoltage = 20;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Pesan diterima dari topik [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, "gsr/data") == 0) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Data Terhubung!");
    display.setCursor(0, 30);
    display.print("Pesan: ");
    for (int i = 0; i < length; i++) {
      display.print((char)payload[i]);
    }
    display.display();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke broker MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Terhubung");
      client.subscribe("gsr/data");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" mencoba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setVoltage(int level) {
  if (level >= 0 && level <= 100) {
    int voltageInVolts = map(level, 0, 100, 0, 24);
    if (voltageInVolts > maxVoltage) {
      Serial.println("Tegangan melebihi batas! Mematikan power supply.");
      digitalWrite(relay, HIGH);
      therapyActive = false;
    } else {
      voltageLevel = level;
      int dutyCycle = map(voltageLevel, 0, 100, 0, 255);
      ledcWrite(pwmPin, dutyCycle);
      
      // Perbarui tampilan OLED
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("Tegangan: ");
      display.print(voltageInVolts);
      display.println(" V");
      display.setCursor(0, 20);
      display.print("Level: ");
      display.print(level);
      display.println("%");
      display.display();
      
      Serial.print("Tegangan diatur ke: ");
      Serial.print(voltageInVolts);
      Serial.println("V");
    }
  } else {
    Serial.println("Nilai tegangan tidak valid (0-100).");
  }
}

void startTherapy() {
  therapyStartTime = millis();
  therapyActive = true;
  digitalWrite(relay, LOW);
  Serial.println("Terapi dimulai.");
}

void stopTherapy() {
  therapyActive = false;
  digitalWrite(relay, HIGH);
  Serial.println("Terapi selesai.");
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal diinisialisasi"));
    for (;;);
  }

  display.clearDisplay();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);

  ledcAttach(pwmPin, 5000, 8);

  pinMode(gsrPin, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  gsrValue = analogRead(gsrPin);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("GSR: ");
  display.print(gsrValue);
  display.setCursor(0, 20);
  display.print("Tegangan: ");
  display.print(map(voltageLevel, 0, 100, 0, 24));
  display.print(" V");
  display.setCursor(0, 40);
  display.print("Level: ");
  display.print(voltageLevel);
  display.print("%");
  display.display();

  String gsrStr = String(gsrValue);
  client.publish("gsr/data", gsrStr.c_str());

  if (therapyActive) {
    if (millis() - therapyStartTime >= therapyDuration) {
      stopTherapy();
    }
  }

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("start")) {
      if (!therapyActive) {
        startTherapy();
      } else {
        Serial.println("Terapi sedang berjalan.");
      }
    } else if (command.equalsIgnoreCase("stop")) {
      if (therapyActive) {
        stopTherapy();
      } else {
        Serial.println("Tidak ada terapi yang berjalan.");
      }
    } else if (command.startsWith("set ")) {
      String voltageStr = command.substring(4);
      int voltage = voltageStr.toInt();
      setVoltage(voltage);
    } else {
      Serial.println("Perintah tidak dikenali. Gunakan 'start', 'stop', atau 'set xx'.");
    }
  }

  delay(500);
}