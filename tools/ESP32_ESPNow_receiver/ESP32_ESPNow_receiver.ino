// simple ESP-Now receiver sketch to receive WIZmote packages for WLED remote testing by @DedeHai

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#define LED_PIN 8  // C3 supermini onboard LED

uint32_t received = 0;

// struct for the received message
typedef struct remote_message_struct {
  uint8_t program;  // 0x91 for ON button, 0x81 for all others
  uint8_t seq[4];   // Incremental sequence number, 32-bit unsigned integer LSB first
  uint8_t byte5;    // Unknown
  uint8_t button;   // button code sent by the remote
  uint8_t byte8;    // Unknown, but always 0x01
  uint8_t byte9;    // Unknown, but always 0x64
  uint8_t byte10;   // Unknown, maybe checksum
  uint8_t byte11;   // Unknown, maybe checksum
  uint8_t byte12;   // Unknown, maybe checksum
  uint8_t byte13;   // Unknown, maybe checksum
} remote_message_struct;

void blink(int times, int blinktime) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(blinktime);
    digitalWrite(LED_PIN, HIGH);
    if(times > 1) // skip low delay if only blinking once
      delay(blinktime);
  }
}
// callback function to handle received ESP-NOW messages
void onDataReceive(const uint8_t *macAddr, const uint8_t *data, int len) {
  // Serial.println("Received a message via ESP-NOW!");
  static uint32_t sequence = 1;  // first sequence is set to 1 on sender
  static uint32_t missedMessages = 0;

  // Print MAC address of the sender
  char macStr[18];
  /*
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  Serial.print("From MAC: ");
  Serial.println(macStr);*/

  if (len == sizeof(remote_message_struct)) {
    remote_message_struct msg;
    memcpy(&msg, data, sizeof(remote_message_struct));  // Copy received data into struct
    uint32_t seqNum = msg.seq[0] | (msg.seq[1] << 8) | (msg.seq[2] << 16) | (msg.seq[3] << 24);
    if (sequence == seqNum)  // same message
      return;    
    if (seqNum - sequence > 1) missedMessages += seqNum - sequence;
    received++;
    sequence = seqNum;  //update to new sequence value
    uint8_t primarychannel;
    wifi_second_chan_t secondchannel;
    esp_wifi_get_channel(&primarychannel, &secondchannel);
    Serial.print("wifi channel: ");
    Serial.print(primarychannel);

    // Print the message contents
    Serial.print(" Message:");
    //Serial.printf("  Program: 0x%02X\n", msg.program);
    ESP_ERR_ESPNOW_NOT_INIT;
    Serial.printf("  Sequence: %u ", seqNum);  // print message sequence
    //Serial.printf("  Byte 5: %u\n", msg.byte5);
    Serial.printf("  Button: %u", msg.button);  // print received button function number
    //Serial.printf("  Byte 8: %u\n", msg.byte8);
    //Serial.printf("  Byte 9: %u\n", msg.byte9);
    //Serial.printf("  Byte 10: 0x%02X\n", msg.byte10);
    //Serial.printf("  Byte 11: 0x%02X\n", msg.byte11);
    //Serial.printf("  Byte 12: 0x%02X\n", msg.byte12);
    //Serial.printf("  Byte 13: 0x%02X\n", msg.byte13);
    Serial.printf(" missed: %u\n", missedMessages);

  } else {
    Serial.println("Received data size does not match the expected struct size.");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // disable LED

  delay(1000);  // wait for usb/serial to connect (is reconnected on reboot)
  Serial.println("************************************************************");
  Serial.println("************* EPS-Now Receiver Tester for WLED *************");
  Serial.println("************************************************************");
  Serial.println(" ");

  // Initialize Wi-Fi in station mode
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW initialized.");

  // Register callback to handle received messages
  esp_now_register_recv_cb(onDataReceive);
  esp_wifi_set_channel(2, WIFI_SECOND_CHAN_NONE); // use wifi channel 2, change if this channel is clogged up or use channel hopping (see main loop)
  blink(5, 50);  // indicate ready
}

void loop() {
  // blink when receiving a message
  if (received) {
    blink(1, 30);
    received--;
  }

  // uncomment to use channel hopping on the receiver
  /*
  for (int i = 1; i < 14; i++) {
    if (esp_wifi_set_channel(i, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
      Serial.println("Failed to set channel");
    }
    unsigned time = millis();
    while (millis() - time < 3000) { // change channel every 3 seconds
      if (received) {
        blink(1, 30);
        received--;
      }
      delay(5);
    }
  }*/
}
