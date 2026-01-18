#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <math.h>
#include <PZEM004Tv30.h>

// ✅ Receiver MAC address
uint8_t receiverAddress[] = {0x94, 0xB9, 0x7E, 0xDA, 0x18, 0x8C};

// ✅ Unified Struct
typedef struct {
  uint8_t type; // 1=temp, 2=rpm, 3=vibration, 4=power
  float val1;
  float val2;
  float val3;
  float val4;
  float val5;
  float val6;
} struct_message;

struct_message myData;

// ✅ ESP-NOW management
int workingChannel = -1;
int failedAttempts = 0;
const int MAX_FAILED_ATTEMPTS = 3;
bool ackReceived = false;

// ==================== VIBRATION SENSOR (MPU6050) ====================
const int SAMPLES = 512;
const int SAMPLE_DELAY = 2;
const float VIBRATION_FREQUENCY = 50.0;
MPU6050 mpu(Wire);
float accelZ_offset = 0.0;
float accelSamples[SAMPLES];

float convertAccelToVelocity(float accel_rms, float freq) {
  if (freq < 1.0) freq = 20.0;
  float velocity_m_s = accel_rms / (2 * PI * freq);
  return velocity_m_s * 1000.0; // mm/s
}

float calculateRMS(float* data, int size) {
  float sum_sq = 0;
  for (int i = 0; i < size; i++) sum_sq += data[i] * data[i];
  return sqrt(sum_sq / size);
}

void calibrateAccelOffset() {
  const int calibrationSamples = 2000;
  float sum = 0.0;
  Serial.println("Calibrating MPU6050... Keep still.");
  for (int i = 0; i < calibrationSamples; i++) {
    mpu.update();
    sum += mpu.getAccZ() * 9.81;
    delay(2);
  }
  accelZ_offset = sum / calibrationSamples;
  Serial.print("Calibration complete. Offset: ");
  Serial.println(accelZ_offset, 6);
}

void collectAccelerationData() {
  for (int i = 0; i < SAMPLES; i++) {
    mpu.update();
    accelSamples[i] = (mpu.getAccZ() * 9.81) - accelZ_offset;
    delay(SAMPLE_DELAY);
  }
}

// ==================== POWER SENSOR (PZEM-004T) ====================
#define RXD2 16
#define TXD2 17
PZEM004Tv30 pzem(Serial0, RXD2, TXD2);

// ==================== ESP-NOW CORE FUNCTIONS ====================
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status){
  ackReceived = (status == ESP_NOW_SEND_SUCCESS);
}

bool tryChannel(int channel) {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(10);

  if (esp_now_init() != ESP_OK) {
    return false;
  }
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      esp_now_deinit();
      return false;
    }
  }

  ackReceived = false;
  struct_message dummy;
  dummy.type = 99;
  dummy.val1 = 0;
  esp_now_send(receiverAddress, (uint8_t *)&dummy, sizeof(dummy));
  delay(200);

  esp_now_deinit();
  return ackReceived;
}

bool findReceiverChannel() {
  Serial.println("🔍 Scanning channels 1–13...");
  for (int ch = 1; ch <= 13; ch++) {
    if (tryChannel(ch)) {
      workingChannel = ch;
      Serial.print("✅ Receiver found on channel ");
      Serial.println(ch);
      return true;
    }
  }
  workingChannel = -1;
  return false;
}

bool setupESPNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(workingChannel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;
  }
  Serial.print("📡 Locked to Channel: ");
  Serial.println(workingChannel);
  return true;
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  // MPU6050 init
  if (mpu.begin() != 0) {
    Serial.println("❌ MPU6050 not connected!");
    while (1) delay(1000);
  }
  Serial.println("✅ MPU6050 connected");
  calibrateAccelOffset();

  Serial.println("⚡ ESP32 + MPU6050 + PZEM-004T + ESP-NOW");

  // Find & lock ESP-NOW channel
  if (!findReceiverChannel()) {
    Serial.println("❌ No receiver found. Halting.");
    while (true);
  }
  if (!setupESPNow()) {
    Serial.println("❌ Failed to setup ESP-NOW. Halting.");
    while (true);
  }
}

// ==================== LOOP ====================
void loop() {
  bool loopSuccess = false; // Track if at least one send works

  // ----- Vibration Measurement -----
  collectAccelerationData();
  float accel_rms = calculateRMS(accelSamples, SAMPLES);
  float velocity_mm_s = convertAccelToVelocity(accel_rms, VIBRATION_FREQUENCY);

  Serial.printf("Velocity (mm/s RMS): %.2f\n", velocity_mm_s);
  if (velocity_mm_s < 0.5) Serial.println("Machine is OFF");
  else if (40 > velocity_mm_s > 0.5) Serial.println("Machine is working properly");
  else Serial.println("Machine is vibrating abnormally");
  myData.type = 3; // Vibration
  myData.val1 = velocity_mm_s;
  myData.val2 = myData.val3 = myData.val4 = myData.val5 = myData.val6 = 0;

  ackReceived = false;
  esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));
  delay(200);
  if (ackReceived) loopSuccess = true;

  // ----- Power Measurement -----
  float v = pzem.voltage();
  float c = pzem.current();
  float p = pzem.power();
  float e = pzem.energy();
  float f = pzem.frequency();
  float pf = pzem.pf();

  if (isnan(v)) {
    Serial.println("❌ Failed to read from PZEM! Sending zeros");
    v = c = p = e = f = pf = 0;
  }

  Serial.printf("V=%.2fV, I=%.2fA, P=%.2fW, E=%.2fkWh, F=%.2fHz, PF=%.2f\n",
                v, c, p, e, f, pf);

  myData.type = 4; // Power
  myData.val1 = v;
  myData.val2 = c;
  myData.val3 = p;
  myData.val4 = e;
  myData.val5 = f;
  myData.val6 = pf;

  ackReceived = false;
  esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));
  delay(200);
  if (ackReceived) loopSuccess = true;

  // ----- Failure handling -----
  if (!loopSuccess) failedAttempts++;
  else failedAttempts = 0;

  if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
    Serial.println("🔄 Too many failures. Re-scanning channels...");
    esp_now_del_peer(receiverAddress);
    esp_now_deinit();
    while (true) {
      if (findReceiverChannel() && setupESPNow()) {
        Serial.println("✅ Reconnected successfully!");
        failedAttempts = 0;
        break;
      }
      Serial.println("🔁 Receiver not found. Retrying in 5 seconds...");
      delay(5000);
    }
  }

  delay(2000); // Repeat every 2s
}
