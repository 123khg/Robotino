//Dùng cho xe bánh bình thường

#include <SPI.h>
#include <RF24.h>

// Define pins (change if you use different ones)
#define CE_PIN   4
#define CSN_PIN  5

#define Joystick_x 2
#define Joystick_y 1

// Địa chỉ pipe phải trùng với bên nhận
const uint64_t address = 0x1234567890LL;
uint16_t x;
uint16_t y;
RF24 radio(CE_PIN, CSN_PIN);  // Create RF24 object

struct send_dat{
  int8_t x_coor; //Có thể sẽ có giá trị được map từ -128 đến 127
  int8_t y_coor; //int8_t là kiểu dữ liệu có dấu và uint8_t là ko dấu (-128;127) 
};
send_dat data;

void send_data(){
  if (radio.available()) {
    radio.write(&data, sizeof(data));
  }
}

//Input data.x va data.y
//Output integer x and y
void pack_data (uint16_t x_raw, uint16_t y_raw){
  data.x_coor = map(x_raw, 0, 4096, -128, 127); 
  data.y_coor = map(y_raw, 0, 4096, -128, 127);
}

void get_data(){
  x = analogRead(Joystick_x);
  y = analogRead(Joystick_y);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}  // Wait for Serial (optional)

  if (!radio.begin()) {
    Serial.println("Radio hardware not responding!");
    delay(1000);
  }
    radio.begin();
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_1MBPS);
    radio.setChannel(67);
    radio.openWritingPipe(address);
    radio.stopListening();
  }
//Do i need to get the acknowledge on every module to know that it runs well? 
void loop() {

//get data --> pack data --> send data
get_data();
pack_data(x,y);
send_data();
delay(20);

}