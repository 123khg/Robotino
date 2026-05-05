#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ESP32 pin connections (standard VSPI pins)
#define CE_PIN 4
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "00001";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32 NRF24L01 Receiver Starting...");
  
  // Initialize radio
  if (!radio.begin()) {
    Serial.println("Radio hardware not responding!");
    while (1);
  }
  
  Serial.println("Radio initialized successfully");
  
  // Settings that work well with clone modules
  radio.setPALevel(RF24_PA_LOW);     // Start with LOW power
  radio.setChannel(100);              // Avoid WiFi interference on channels 1-13
  radio.openReadingPipe(0, address);
  radio.setAutoAck(false);
  radio.enableDynamicPayloads();

  radio.startListening();
  
  radio.printDetails();  // This shows what chip the library detected
}

void loop() {
  if (radio.available()) {
    char text[32];
    radio.read(&text, sizeof(text));
    Serial.print("Received: ");
    Serial.println(text);
  }
}