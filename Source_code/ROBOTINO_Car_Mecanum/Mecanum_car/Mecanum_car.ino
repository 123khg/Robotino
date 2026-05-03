//Dùng cho xe bánh bình thường

#include <SPI.h>
#include <RF24.h>
#include <stdint.h>

// ================== PIN CONFIG ==================
#define CE_PIN   9
#define CSN_PIN  10

// Motor Left (A)
#define IN1  8
#define IN2  7
#define ENA  6   // PWM

// Motor Right (B)
#define IN3  5
#define IN4  4
#define ENB  3   // PWM

#define PWM_FREQ 5000
#define PWM_RES 8

#define DEADZONE 15

//RF24

RF24 radio(CE_PIN, CSN_PIN);
const uint64_t address = 0x1234567890LL;

struct DataPacket {
    int8_t x_coor;
    int8_t y_coor;
};
DataPacket data;

unsigned long last_receive_time = 0;

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
// ================== MOTOR CONTROL ==================
void setupMotors() {
    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
    
    // PWM cho ESP32
    ledcAttach(ENA, PWM_FREQ, PWM_RES);
    ledcAttach(ENB, PWM_FREQ, PWM_RES);
}

void applyMotor(int speed, uint8_t pwmPin, uint8_t in1, uint8_t in2) {
    if (abs(speed) < DEADZONE) {
        // Brake
        digitalWrite(in1, HIGH);
        digitalWrite(in2, HIGH);
        ledcWrite(pwmPin, 0);
    } 
    else if (speed > 0) {
        // Forward
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        ledcWrite(pwmPin, speed);
    } 
    else {
        // Backward
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        ledcWrite(pwmPin, -speed);  // abs(speed)
    }
}

void drive(int x, int y) {
    int leftSpeed  = y + x;
    int rightSpeed = y - x;

    leftSpeed  = constrain(leftSpeed,  -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    applyMotor(leftSpeed,  ENA, IN1, IN2);
    applyMotor(rightSpeed, ENB, IN3, IN4);
}

// ================== SETUP ==================
void setup() {
    Serial.begin(115200);
    
    setupMotors();
    
    if (!radio.begin()) {
        Serial.println("RF24 không khởi động được!");
        while(1) delay(100);  // Dừng nếu lỗi
    }
    
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_1MBPS);
    radio.setChannel(67);
    radio.openReadingPipe(0, address);
    radio.startListening();
    
    Serial.println("Xe sẵn sàng!");
}

//Receive data 
//setup pwm, output pin (setup_motor)
//Check fail safe
//Xuat tin hieu ra cac chan
// ================== LOOP ==================
void loop() {
    // Nhận dữ liệu
    if (radio.available()) {
        radio.read(&data, sizeof(data));
        last_receive_time = millis();
    }

    // Failsafe
    if (millis() - last_receive_time > 500) {
        data.x_coor = 0;
        data.y_coor = 0;
    }

    // Map giá trị
    int pwmX = map(data.x_coor, -128, 127, -255, 255);
    int pwmY = map(data.y_coor, -128, 127, -255, 255);

    drive(pwmX, pwmY);

    // Debug (tắt khi chạy thật)
    // static uint32_t lastDebug = 0;
    // if (millis() - lastDebug > 200) {
    //     Serial.printf("X:%d Y:%d | L:%d R:%d\n", data.x_coor, data.y_coor, pwmX, pwmY);
    //     lastDebug = millis();
    // }
}