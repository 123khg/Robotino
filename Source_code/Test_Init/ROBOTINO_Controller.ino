//Include 
#include <SPI.h>
#include <RF24.h>

#define PIN_NRF_CE   27
#define PIN_NRF_CSN  25
#define PIN_NRF_SCK  14
#define PIN_NRF_MISO 12
#define PIN_NRF_MOSI 13
SPIClass *spi = new SPIClass(HSPI);
RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);

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
const byte address[5] = {'0','0','0','0','1'};

void setup() {
  Serial.begin(115200);
  spi->begin(PIN_NRF_SCK, PIN_NRF_MISO, PIN_NRF_MOSI, -1); //tuong tu voi (*spi).begin();
  delay(1000);
  Serial.println();
  Serial.println(F("=== Kiem tra ket noi NRF24L01 voi ESP32 ==="));

  // Khoi tao radio
  while (!radio.begin(spi)) {
    Serial.println(F("Loi: Khong khoi tao duoc NRF24L01 (RX)."));
    delay(1000);
  }

  Serial.println(F("OK: NRF24L01 da san sang!"));
  
  // Tùy chọn: in thông tin cau hinh chip ra Serial (debug)
  radio.printDetails();

  radio.setChannel(108);         // Phai trung voi TX
  radio.setPALevel(RF24_PA_LOW); // Cong suat giong TX
  radio.setDataRate(RF24_1MBPS); // Toc do giong TX

  // Mo pipe 1 o che do doc (so 1 la index pipe)
  radio.openReadingPipe(1, address);

  // Bat che do nghe
  radio.startListening();
}

void loop() {
  // Tam thoi chua gui/nhan gi, chi de do
}

//Để test thì có thể copy paste sang 2 file khác nhau để test 
//Khoi transmit 
//Dựa vào datasheet thì ta cần khởi tạo mode của con NRF --> Chọn là Receive hoặc transmit
// Ngoài ra phải lựa tần số hoạt động, tiếp theo là chọn pipeline. Chọn payload hợp lý để truyền tối ưu nhất.
//Hơn nữa do board mình có PA và LNA nên mình cũng cso thể setup nó 
//Tối ưu thì cũng có thể thêm Auto Acknowledge