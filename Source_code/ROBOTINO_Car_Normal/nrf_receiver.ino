//Dùng cho xe bánh bình thường

#include <SPI.h>
#include <RF24.h>
#include <stdint.h>

// Define pins (change if you use different ones)
#define CE_PIN   4
#define CSN_PIN  5

#define IN1 8
#define IN2 7
#define ENA 9  // Chân cấp PWM cho bánh trái (Motor A)

#define IN3 5
#define IN4 4
#define ENB 10 // Chân cấp PWM cho bánh phải (Motor B)

#define PWM_FREQ 5000
#define PWM_RES 8
#define PWM_CHANNEL_A 11
#define PWM_CHANNEL_B 1
#define DEADZONE 15

RF24 radio(CE_PIN, CSN_PIN);  // Create RF24 object

struct receive_dat{
  int8_t x_coor; //Có thể sẽ có giá trị được map từ -128 đến 127
  int8_t y_coor; //int8_t là kiểu dữ liệu có dấu và uint8_t là ko dấu (-128;127) 
};
receive_dat data;

int pwm_x = 0;
int pwm_y = 0;
const uint64_t address = 0x1234567890LL;

unsigned long last_receive_time = 0; //So nguyen khong dau (32bit) 

/*Create module receive data (including failsafe)*/

void receivedata(){
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    last_receive_time = millis(); // Cập nhật thời gian khi nhận được lệnh
  }
}
void check_failsafe(){
  unsigned long current_time = millis(); 
  if(current_time - last_receive_time >500){
    data.x_coor = 0;
    data.y_coor = 0;
  }
}

//Note: IN1,2 là điều khiển chiều của động cơ trái; IN3,4 là động cơ phải 
//Chân ENA là xuất xung tín hiệu PWM để điều khiển tốc độ A 
//Lay data receive duoc de lam input. Va output la gia tri da duoc chinh sua

//Nếu mà IN1, IN2 là 01 hay 10 thì chạy còn 11 sẽ brake!!! --> Nếu x,y thuộc deadzone thì đặt trạng thái brake cho xe không bị trôi
//Depend on the input, it will define that the motor want to go forward or backward

void apply_motor(int speed, int pwmPin, int in1, int in2)
{
  if(abs(speed)<DEADZONE){
    digitalWrite(in1, HIGH);
    digitalWrite(in2, HIGH);
    ledcWrite(pwmPin, 0);
  }
  else if (speed >DEADZONE){
    // Chạy tiến 10
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, abs(speed));
  }
  else{
    // Chạy lùi: 01
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(pwmPin, abs(speed));
  }
}

//Dựa vào giá trị x, y của data.x và data.y để set tốc độ
void drive_motor(int x, int y){
  //Nếu đi thẳng thì x=0, y=y -->Lái đặt PWM ra cả 2 bên bánh --> Có chiều như nào đó thì ở đây không quyết định.
  //ENA bên trái ENB bên phải --> Ở đây phải xác định đuiowjc bên trái là tốc độ bao nhiêu theo x,y và bên phải cũng vậy, sau đó chỉ cần set speedA và speedB là xong
  //quẹo bên đâu thì trừ vận tốc bên đó cho nó không ảnh hưởng đến speed bên kia
   
  // Thuật toán Mixing (Trộn kênh):
  // y đóng vai trò tiến lùi, x đóng vai trò rẽ
  int leftSpeed = y + x;
  int rightSpeed = y - x;

  // Giới hạn giá trị trong khoảng chuẩn của PWM (-255 đến 255)
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Xuất xung và chiều cho Motor Trái (A) và Phải (B)
  apply_motor(leftSpeed, ENA, IN1, IN2);
  apply_motor(rightSpeed, ENB, IN3, IN4);
}



//Module SETUP PWM 
void setupMotorPins() {
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); 
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  // Cấu hình PWM phần cứng cho ESP32
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);
}

void setup() {
  Serial.begin(115200);
   if (!radio.begin()) {
    Serial.println("Radio hardware not responding!");
    delay(1000);
  }
    radio.begin();
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_1MBPS);
    radio.setChannel(67);
    radio.openReadingPipe(0, address);
    radio.startListening();
  }

void loop() {

//Receive data 
//setup pwm, output pin (setup_motor)
//Check fail safe
//Xuat tin hieu ra cac chan (cu the la driveMotor)'
  receivedata();

  int pwm_x = map(data.x_coor, -128, 127, -255, 255); 
  int pwm_y = map(data.y_coor, -128, 127, -255, 255); 
  setupMotorPins();
  check_failsafe();
  drive_motor(pwm_x, pwm_y);
}