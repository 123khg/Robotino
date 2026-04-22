//Include 
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>


#define PIN_NRF_CE   4
#define PIN_NRF_CSN  5
#define PIN_NRF_SCK  18
#define PIN_NRF_MISO 19
#define PIN_NRF_MOSI 23

// #define Joystick_x 
// #define Joystick_y 
// //Khoi truyen du lieu NRF24L01
// //Khoi tao (gom co chip, chan sdung,kenh giao tiep, loai giao tiep (Receive or transmit) )
// //Receive du lieu danh cho Xe
// //Receive thi no se co phan code ve dieu khien motor
// //Transmit du lieu danh cho tay cam
// //Transmit thi se co phan xu ly ADC
// int x = analogRead(Joystick_x);
// int y = analogRead(Joystick_y);

//Input là giá trị analog từ joystick. Output là giá trị cần điều khiển bánh nào nhanh, chậm

RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);
const byte address[6] = "00001";


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("=== Kiem tra ket noi NRF24L01 voi ESP32 ==="));

  // Khoi tao radio
  if (!radio.begin()) {
    Serial.println(F("Loi: Khong tim thay NRF24L01. Kiem tra lai day noi va nguon!"));
    while (1) {
      delay(1000);
    }
  }

  Serial.println(F("OK: NRF24L01 da san sang!"));
  
  // Tùy chọn: in thông tin cau hinh chip ra Serial (debug)
  radio.printDetails();
}

void loop() {
  // Tam thoi chua gui/nhan gi, chi de do
}


// void setup(){
//   Serial.begin(115200);
//   radio.begin();
//   radio.openReadingPipe(0, address);
//   radio.setLNALevel(RF24_LA_MIN);
//   radio.startListening();
// }

// void loop(){
// if (radio.available()) {
//     char text[32] = "";
//     radio.read(&text, sizeof(text));
//     Serial.println(text);
//   }
// }

//Để test thì có thể copy paste sang 2 file khác nhau để test 
//Khoi transmit 
//Dựa vào datasheet thì ta cần khởi tạo mode của con NRF --> Chọn là Receive hoặc transmit
// Ngoài ra phải lựa tần số hoạt động, tiếp theo là chọn pipeline. Chọn payload hợp lý để truyền tối ưu nhất.
//Hơn nữa do board mình có PA và LNA nên mình cũng cso thể setup nó 
//Tối ưu thì cũng có thể thêm Auto Acknowledge



