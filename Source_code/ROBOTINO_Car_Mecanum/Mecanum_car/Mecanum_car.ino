#include <SPI.h>
#include <RF24.h>
#include <stdint.h>

//Dabble debug
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>


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
  double theta_send; 
  double power_send; 
  int16_t omega_send;
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
    data.power_send = 0;
    data.theta_send = 0;
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

//RECEIVED: pOWER, THETA AND OMEGA --> wE HAVE TO OUTPUT THE PWM SIGNALS SATISFY LIMIT AND RIGHT CHOICE
// Nhận vào:
// - power: Công suất tổng (0 đến 255)
// - theta: Góc di chuyển tính bằng Radian
// - turn: Tốc độ xoay (-255 đến 255, âm là xoay trái, dương là xoay phải)
void driveMecanum(double power, double theta, double turn) {
    
    // Đổi tên biến để tránh xung đột với hàm sin(), cos() của C++
    double cos_val = cos(theta - PI/4.0);
    double sin_val = sin(theta - PI/4.0);
    
    // Tìm giá trị lớn nhất giữa sin và cos để giữ power luôn đạt đỉnh
    double max_val = max(abs(cos_val), abs(sin_val));

    // Tính toán tốc độ thô cho 4 bánh
    double v_fl = (power * cos_val / max_val) + turn; // Trước - Trái
    double v_fr = (power * sin_val / max_val) - turn; // Trước - Phải
    double v_bl = (power * sin_val / max_val) + turn; // Sau - Trái
    double v_br = (power * cos_val / max_val) - turn; // Sau - Phải

    // Normalization (Chuẩn hóa tỷ lệ)
    // Nếu tổng lực vọt qua 255, ta phải giảm tốc độ cả 4 bánh xuống theo cùng 1 tỷ lệ
    // để xe không bị méo quỹ đạo (đi chéo mà bị thành đi thẳng)
    double max_speed = max(max(abs(v_fl), abs(v_fr)), max(abs(v_bl), abs(v_br)));
    if (max_speed > 255.0) {
        v_fl = (v_fl / max_speed) * 255.0;
        v_fr = (v_fr / max_speed) * 255.0;
        v_bl = (v_bl / max_speed) * 255.0;
        v_br = (v_br / max_speed) * 255.0;
    }

    // Xuất tín hiệu ra 4 bánh bằng hàm applyMotor đã cấu hình
    applyMotor((int)v_fl, EN2L, IN2L);
    applyMotor((int)v_fr, EN2R, IN2R);
    applyMotor((int)v_bl, EN4L, IN4L);
    applyMotor((int)v_br, EN4R, IN4R);
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
        Serial.printf("Gui thanh cong");
    }

    // Failsafe
    if (millis() - last_receive_time > 500) {
        data.power_send = 0;
        data.theta_send = 0;
        data.omega_send = 0;
    }
    
    driveMecanum(data.power_send, data.theta_send, data.omega_send);
    // Debug (tắt khi chạy thật)
    // static uint32_t lastDebug = 0;
    // if (millis() - lastDebug > 200) {
    //     Serial.printf("X:%d Y:%d | L:%d R:%d\n", data.x_coor, data.y_coor, pwmX, pwmY);
    //     lastDebug = millis();
    // }
}