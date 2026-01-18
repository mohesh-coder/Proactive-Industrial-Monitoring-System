#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>

// Wi-Fi credentials
const char* ssid = "ZTE_2.4G_qNuuqK";
const char* password = "d3Yma4KT";

// MQTT broker settings
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "/esp32/sensorData";  // Unified topic

WiFiClient espClient;
PubSubClient client(espClient);

// Define unique message structure (match sender side!)
typedef struct struct_message {
  uint8_t type;  // 1=temp, 2=rpm, 3=vibration, 4=power
  float val1;    // temperature or rpm or vibration or voltage
  float val2;    // current (power)
  float val3;    // power
  float val4;    // energy
  float val5;    // frequency
  float val6;    // power factor
} struct_message;

struct_message incomingData;

// Store latest values
float latestTemp = 0;
float latestRPM = 0;
float latestVibration = 0;
float voltage = 0, current = 0, power = 0, energy = 0, frequency = 0, pf = 0;

// Function to publish all data as JSON
void publishCombinedData() {
  char payload[256];
  snprintf(payload, sizeof(payload),
           "{"
           "\"temperature\": %.2f, "
           "\"rpm\": %.2f, "
           "\"vibration\": %.2f, "
           "\"voltage\": %.2f, "
           "\"current\": %.2f, "
           "\"power\": %.2f, "
           "\"energy\": %.2f, "
           "\"frequency\": %.2f, "
           "\"powerFactor\": %.2f"
           "}",
           latestTemp, latestRPM, latestVibration,
           voltage, current, power, energy, frequency, pf);

  client.publish(mqtt_topic, payload);
  Serial.println("📡 Sent to MQTT:");
  Serial.println(payload);
}

// Handle incoming ESP-NOW data
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int data_len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  Serial.print("📥 Received from: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  switch (incomingData.type) {
    case 1:  // Temperature
      latestTemp = incomingData.val1;
      Serial.print("🌡 Temp: ");
      Serial.println(latestTemp);
      break;

    case 2:  // RPM
      latestRPM = incomingData.val1;
      Serial.print("⚙️ RPM: ");
      Serial.println(latestRPM);
      break;

    case 3:  // Vibration
      latestVibration = incomingData.val1;
      Serial.print("🌀 Vibration: ");
      Serial.println(latestVibration);
      break;

    case 4:  // Power
      voltage   = incomingData.val1;
      current   = incomingData.val2;
      power     = incomingData.val3;
      energy    = incomingData.val4;
      frequency = incomingData.val5;
      pf        = incomingData.val6;
      Serial.println("⚡ Power Data:");
      Serial.printf("Voltage: %.2f V, Current: %.2f A, Power: %.2f W\n", voltage, current, power);
      Serial.printf("Energy: %.2f kWh, Freq: %.2f Hz, PF: %.2f\n", energy, frequency, pf);
      break;

    default:
      Serial.println("❓ Unknown type received");
      return;
  }

  // After every valid update, publish all data
  publishCombinedData();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.print("🔌 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }
  Serial.println("✅ ESP-NOW Initialized");
  esp_now_register_recv_cb(OnDataRecv);

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    while (!client.connected()) {
      Serial.print("🔁 Connecting to MQTT...");
      if (client.connect("ESP32UnifiedReceiver")) {
        Serial.println("✅ Connected to MQTT");
      } else {
        Serial.print("❌ Failed, rc=");
        Serial.println(client.state());
        delay(2000);
      }
    }
  }

  client.loop();  // Maintain MQTT connection
}