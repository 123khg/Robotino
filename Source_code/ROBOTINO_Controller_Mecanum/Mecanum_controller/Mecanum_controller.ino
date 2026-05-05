//Dùng cho xe bánh bình thường
//I finished the simple code with puzzy ahh structure but forgot to optimize it...
#include <SPI.h>
#include <RF24.h>
#include <math.h>

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
  double theta_send; //Có thể sẽ có giá trị được map từ -128 đến 127
  double power_send; //int8_t là kiểu dữ liệu có dấu và uint8_t là ko dấu (-128;127) 
  int16_t omega_send;
};
send_dat data;


void setup() {
    Serial.begin(115200);
    
    pinMode(JOY_X, INPUT);
    pinMode(JOY_Y, INPUT);
    pinMode(JOY_X1,INPUT); 

    if (!radio.begin()) {
        Serial.println("RF24 không khởi động được!");
        while(1) delay(100);
    }

    radio.setPALevel(RF24_PA_LOW);      
    radio.setDataRate(RF24_1MBPS);
    radio.setChannel(67);
    radio.openWritingPipe(address);
    radio.stopListening();              

    Serial.println("Transmitter sẵn sàng!");
}
//get data --> pack data --> send data
void loop() {
    // Đọc joystick
    uint16_t x_raw = analogRead(JOY_X);
    uint16_t y_raw = analogRead(JOY_Y);
    uint16_t x_raw1 = analogRead(JOY_X1);
    int16_t dx = x_raw - 2048;
    int16_t dy = y_raw - 2048;

    //Ta sử dụng hẳn giá trị read được để tính theta và vận tốc luôn. và thêm deadzone vào
    data.theta_send = atan2((double)dy, (double)dx);           // -π ~ +π
    // ÁP DỤNG THUẬT TOÁN "MAP HÌNH VUÔNG THÀNH HÌNH TRÒN" vì Joystick sẽ khá là ehhhh, khoảng giá trị nó là hình vuông 
    double x_norm = dx / 2048.0;
    double y_norm = dy / 2048.0;
    double x_circular = x_norm * sqrt(1.0 - (y_norm * y_norm / 2.0));
    double y_circular = y_norm * sqrt(1.0 - (x_norm * x_norm / 2.0));
    data.power_send = map(2048*hypot((double)x_circular, (double)y_circular),0,2048,0,255);
    data.omega_send = map(x_raw1, 0, 4095, -255, 255);

    // Gửi dữ liệu
    bool success = radio.write(&data, sizeof(data));

    // static uint32_t lastDebug = 0;
    // if (millis() - lastDebug > 200) {
    //     Serial.printf("success ? "OK" : "Fail");
    //     lastDebug = millis();
    // }
}