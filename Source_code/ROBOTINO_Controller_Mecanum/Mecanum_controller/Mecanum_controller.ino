//Dùng cho xe bánh bình thường
//I finished the simple code with puzzy ahh structure but forgot to optimize it...
#include <SPI.h>
#include <RF24.h>

// Define pins
#define CE_PIN   4
#define CSN_PIN  5

#define JOY_X 2
#define JOY_Y 1
#define JOY_X1 3

// Địa chỉ pipe phải trùng với bên nhận
const uint64_t address = 0x123456789LL;
//
RF24 radio(CE_PIN, CSN_PIN);  // Create RF24 object

struct send_dat{
  int8_t x_coor; //Có thể sẽ có giá trị được map từ -128 đến 127
  int8_t y_coor; //int8_t là kiểu dữ liệu có dấu và uint8_t là ko dấu (-128;127) 
  int8_t x_coor1;
};
send_dat data;

void send_data(){
  if (radio.available()) {
    radio.write(&data, sizeof(data));
  }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(JOY_X, INPUT);
    pinMode(JOY_Y, INPUT);
    pinMode(JOY_X1,INPUT); 

    if (!radio.begin()) {
        Serial.println("RF24 không khởi động được!");
        while(1) delay(100);
    }

    radio.setPALevel(RF24_PA_LOW);      // Thấp để tiết kiệm pin
    radio.setDataRate(RF24_1MBPS);
    radio.setChannel(67);
    radio.openWritingPipe(address);
    radio.stopListening();              // Chế độ Transmitter

    Serial.println("Transmitter sẵn sàng!");
}
//get data --> pack data --> send data
void loop() {
    // Đọc joystick
    uint16_t x_raw = analogRead(JOY_X);
    uint16_t y_raw = analogRead(JOY_Y);
    uint16_t x_raw1 = analogRead(JOY_X1);

    // Map sang -128 ~ 127
    data.x_coor = map(x_raw, 0, 4095, -128, 127);
    data.y_coor = map(y_raw, 0, 4095, -128, 127);
    data.x_coor1 = map(x_raw1, 0, 4095, -128, 127);

    // Gửi dữ liệu
    bool success = radio.write(&data, sizeof(data));

    // Debug
    // static uint32_t lastDebug = 0;
    // if (millis() - lastDebug > 200) {
    //     Serial.printf("Raw X:%4d Y:%4d | Sent X:%4d Y:%4d | %s\n", 
    //                   x_raw, y_raw, data.x_coor, data.y_coor,
    //                   success ? "OK" : "Fail");
    //     lastDebug = millis();
    // }

    delay(20);  // Tần số gửi ~50Hz
}