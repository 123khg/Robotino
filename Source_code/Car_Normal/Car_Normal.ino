//Dùng cho xe bánh bình thường

#include <SPI.h>
#include <RF24.h>
#include <stdint.h>

// ================== PIN CONFIG ==================
#define CE_PIN   9
#define CSN_PIN  10

// Motor Left (A)
#define IN2L  8
#define IN2R  7
#define EN2L  6   // PWM cua Enable 2L
#define EN2R  12

// Motor Right (B)
#define IN4R  5
#define IN4L  4
#define EN4R  3   // PWM
#define EN4L  11

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

//IN2L điều khiển bánh trên trái , IN4L dưới trái. IN2R trên phải, IN4R dưới phải.
// Ta có 4 chân điều khiển tốc độ. E2L: trên trái, E4L: dưới trái. E2R: trên phải, E4R: dưới phải.
//Lay data receive duoc de lam input. Va output la gia tri da duoc chinh sua

//Nếu mà IN1, IN2 là 01 hay 10 thì chạy còn 11 sẽ brake!!! --> Nếu x,y thuộc deadzone thì đặt trạng thái brake cho xe không bị trôi
//Depend on the input, it will define that the motor want to go forward or backward
// ================== MOTOR CONTROL ==================
void setupMotors() {
    pinMode(IN2L, OUTPUT); pinMode(IN2R, OUTPUT);
    pinMode(IN4L, OUTPUT); pinMode(IN4R, OUTPUT);
    
    // PWM cho ESP32
    ledcAttach(EN2L, PWM_FREQ, PWM_RES);
    ledcAttach(EN4R, PWM_FREQ, PWM_RES);
    ledcAttach(EN2R, PWM_FREQ, PWM_RES);
    ledcAttach(EN4L, PWM_FREQ, PWM_RES);

}
//Khi này brake sẽ là IN2L = IN2R = 0
void applyMotor(int speed, uint8_t pwmPin, uint8_t dirPin) {
    if (abs(speed) < DEADZONE) {
        // Brake: Bắt buộc phanh mềm (tắt PWM) vì dùng cổng NOT
        ledcWrite(pwmPin, 0);
    } 
    else if (speed > 0) {
        // Forward: Cấp mức HIGH (Cổng NOT sẽ tự đảo LOW cho chân IN còn lại)
        digitalWrite(dirPin, HIGH);
        ledcWrite(pwmPin, speed);
    } 
    else {
        // Backward: Cấp mức LOW
        digitalWrite(dirPin, LOW);
        ledcWrite(pwmPin, abs(speed));  // Dùng hàm abs() cho an toàn
    }
}

void drive(int x, int y) {
    int leftSpeed  = y + x;
    int rightSpeed = y - x;

    leftSpeed  = constrain(leftSpeed,  -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    // Xuất tín hiệu cho 2 bánh BÊN TRÁI
    applyMotor(leftSpeed,  EN2L, IN2L); // Bánh trên trái
    applyMotor(leftSpeed,  EN4L, IN4L); // Bánh dưới trái
    
    // Xuất tín hiệu cho 2 bánh BÊN PHẢI
    applyMotor(rightSpeed, EN2R, IN2R); // Bánh trên phải
    applyMotor(rightSpeed, EN4R, IN4R); // Bánh dưới phải
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