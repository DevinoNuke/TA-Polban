#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Inisialisasi OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Ganti dengan informasi jaringan WiFi Anda
const char* ssid = "SII-Cikunir";
const char* password = "admin.admin";

// Informasi broker MQTT
const char* mqtt_server = "iotmqtt.online";
const char* mqtt_username = "public";
const char* mqtt_password = "public1234";

// Deklarasi pin GSR dan WiFi client
const int gsrPin = 34; // Pin sensor GSR pada ESP32
int gsrValue = 0;

WiFiClient espClient;
PubSubClient client(espClient);

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

  // Menampilkan pesan pada OLED jika menerima data dari aplikasi Kodular
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
  // Loop sampai terhubung kembali ke broker MQTT
  while (!client.connected()) {
    Serial.print("Menghubungkan ke broker MQTT...");
    // Koneksi ke broker MQTT dengan username dan password
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Terhubung");
      client.subscribe("gsr/data");  // Ganti sesuai topik yang ingin disubscribe
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" mencoba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal diinisialisasi"));
    for (;;);
  }

  // Bersihkan display OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Hubungkan ke WiFi
  setup_wifi();

  // Hubungkan ke broker MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Baca nilai dari sensor GSR
  gsrValue = analogRead(gsrPin);

  // Tampilkan data di OLED
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("GSR Value: ");
  display.print(gsrValue);
  display.display();

  // Kirim data GSR ke broker MQTT
  String gsrStr = String(gsrValue);
  client.publish("gsr/data", gsrStr.c_str());

  // Delay untuk pengiriman data (2 detik)
  delay(2000);
}
