#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN   4
#define CSN_PIN  5

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "00001";

int counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== NRF24L01 Transmitter - Auto ACK Disabled (Clone Test) ===");

  if (!radio.begin()) {
    Serial.println("Radio not responding!");
    while (1) {}
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
radio.setChannel(100);
radio.openWritingPipe(address);
radio.openReadingPipe(0, address);   // <-- add this line
  radio.stopListening();

  // === Key line for clone test ===
  radio.setAutoAck(false);     // Disable Auto ACK
  radio.enableDynamicPayloads();

  Serial.println("Transmitter ready (Auto ACK OFF). Sending every 1s...");
  radio.printDetails();        // Print config for debugging
}

void loop() {
  char text[32];
  snprintf(text, sizeof(text), "Hello ESP32 #%d", counter++);


int len = strlen(text) + 1;
radio.write(&text, len);
  
    Serial.println(text);

  delay(1000);
}