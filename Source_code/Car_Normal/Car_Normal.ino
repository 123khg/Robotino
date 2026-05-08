#include <SPI.h>
#include <RF24.h>
#include <stdint.h>

#include <DabbleESP32.h>
#define BLUETOOTH_NAME "Robotino_Normal"

// ================== STRUCT ==================
struct Counter_t {
  uint32_t interval; // millis/micros
  uint32_t time;
};
Counter_t RF24_counter = {200, 0}, dabble_counter = {50, 0}, motor_counter = {5, 0};

struct DataPacket_t {
  double theta; 
  double power; 
  int16_t omega;
};
DataPacket_t data0 = {0, 0, 0};

struct DataPacket_Normal_t{
  float speedTurn;
  float speedForward;
};

DataPacket_Normal_t data = {0,0};

struct DabbleInput_t {
  float radius;
  float angle;
  bool forward;
  bool backward;
  bool left;
  bool right;
};
DabbleInput_t dabbleInput;

// ================== PIN CONFIG ==================
// #define CE_PIN   17
// #define CSN_PIN  16
// #define HSPI_MISO   12
// #define HSPI_MOSI   13
// #define HSPI_SCLK   14
// #define HSPI_SS     15

// 4 là bánh trên, 2 là bánh dưới
// Motor Left (A)
#define IN4L  25
#define IN4R  14
#define EN4L  26   // PWM cua Enable 2L
#define EN4R  21

// Motor Right (B)
#define IN2R  27
#define IN2L  32
#define EN2L  33
#define EN2R  22  

#define PWM_FREQ 5000
#define PWM_RES 8

#define DEADZONE 15

#define SCALE_LEFT 1
#define SCALE_RIGHT 1
#define MAX_POWER 200


// // ================== RF24 CONFIG ==================
// RF24 radio(CE_PIN, CSN_PIN);
// const uint64_t address = 0xE7E7E7E7E7LL;

bool debug = false;

/*Create module receive data (including failsafe)*/
// void receivedata(){
//   if (radio.available()) {
//     radio.read(&data, sizeof(data));
//     last_receive_time = millis(); // Cập nhật thời gian khi nhận được lệnh
//   }
// }
// void check_failsafe(){
//   unsigned long current_time = millis(); 
//   if(current_time - last_receive_time >500){
//     data.power = 0;
//     data.theta = 0;
//   }
// }

//IN2L điều khiển bánh trên trái , IN4L dưới trái. IN2R trên phải, IN4R dưới phải.
// Ta có 4 chân điều khiển tốc độ. E2L: trên trái, E4L: dưới trái. E2R: trên phải, E4R: dưới phải.
//Lay data receive duoc de lam input. Va output la gia tri da duoc chinh sua

//Nếu mà IN1, IN2 là 01 hay 10 thì chạy còn 11 sẽ brake!!! --> Nếu x,y thuộc deadzone thì đặt trạng thái brake cho xe không bị trôi
//Depend on the input, it will define that the motor want to go forward or backward
// ================== MOTOR CONTROL ==================
void setupMotors() {
    pinMode(IN2L, OUTPUT); pinMode(IN2R, OUTPUT);
    digitalWrite(IN2L, LOW);
    digitalWrite(IN2R, LOW);
    pinMode(IN4L, OUTPUT); pinMode(IN4R, OUTPUT);
    digitalWrite(IN4L, LOW);
    digitalWrite(IN4R, LOW);

    pinMode(EN2L, OUTPUT);
    pinMode(EN4R, OUTPUT);
    pinMode(EN2R, OUTPUT);
    pinMode(EN4L, OUTPUT);
}

void applyMotor(int speed, uint8_t pwmPin, uint8_t dirPin) {
    if (abs(speed) < DEADZONE) {
        analogWrite(pwmPin, 0); 
    } 
    else if (speed > 0) {
        digitalWrite(dirPin, HIGH);
        analogWrite(pwmPin, speed); // Just like Arduino
    } 
    else {
        digitalWrite(dirPin, LOW);
        analogWrite(pwmPin, abs(speed));
    }
}

//RECEIVED: pOWER, THETA AND OMEGA --> wE HAVE TO OUTPUT THE PWM SIGNALS SATISFY LIMIT AND RIGHT CHOICE
// Nhận vào:
// - power: Công suất tổng (0 đến 255)
// - theta: Góc di chuyển tính bằng Radian
// - turn: Tốc độ xoay (-255 đến 255, âm là xoay trái, dương là xoay phải)
// void driveMecanum(double power, double theta, double turn) {
    
//     // Đổi tên biến để tránh xung đột với hàm sin(), cos() của C++
//     double sin_val = sin(theta + PI/4.0);
//     double cos_val = cos(theta + PI/4.0);
    
//     // Tìm giá trị lớn nhất giữa sin và cos để giữ power luôn đạt đỉnh
//     double max_val = max(abs(cos_val), abs(sin_val));

//     // Tính toán tốc độ thô cho 4 bánh
//     double v_fl = (power * cos_val / max_val) + turn; // Trước - Trái
//     double v_fr = (power * sin_val / max_val) - turn; // Trước - Phải
//     double v_bl = (power * sin_val / max_val) + turn; // Sau - Trái
//     double v_br = (power * cos_val / max_val) - turn; // Sau - Phải

//     // Normalization (Chuẩn hóa tỷ lệ)
//     // Nếu tổng lực vọt qua 255, ta phải giảm tốc độ cả 4 bánh xuống theo cùng 1 tỷ lệ
//     // để xe không bị méo quỹ đạo (đi chéo mà bị thành đi thẳng)
//     double max_speed = max(max(abs(v_fl), abs(v_fr)), max(abs(v_bl), abs(v_br)));
//     if (max_speed > 255.0) {
//         v_fl = (v_fl / max_speed) * 255.0;
//         v_fr = (v_fr / max_speed) * 255.0;
//         v_bl = (v_bl / max_speed) * 255.0;
//         v_br = (v_br / max_speed) * 255.0;
//     }

//     // Xuất tín hiệu ra 4 bánh bằng hàm applyMotor đã cấu hình
//     applyMotor((int)v_fl, EN2L, IN2L);
//     applyMotor((int)v_fr, EN2R, IN2R);
//     applyMotor((int)v_bl, EN4L, IN4L);
//     applyMotor((int)v_br, EN4R, IN4R);
// }

void driveNormal(float turn_speed, float forward_speed) {
    if (turn_speed > 0 && forward_speed == 0) turn_speed = turn_speed/abs(turn_speed) * 255; 
    int leftSpeed  = (forward_speed + turn_speed) * SCALE_LEFT;
    int rightSpeed = (forward_speed - turn_speed) * SCALE_RIGHT;

    leftSpeed  = constrain(leftSpeed,  -1*MAX_POWER, MAX_POWER);
    rightSpeed = constrain(rightSpeed, -1*MAX_POWER, MAX_POWER);

    // Xuất tín hiệu cho 2 bánh BÊN TRÁI
    applyMotor(leftSpeed,  EN2L, IN2L); // Bánh trên trái
    applyMotor(leftSpeed,  EN4L, IN4L); // Bánh dưới trái
    
    // Xuất tín hiệu cho 2 bánh BÊN PHẢI
    applyMotor(rightSpeed, EN2R, IN2R); // Bánh trên phải
    applyMotor(leftSpeed, EN4R, IN4R); // Bánh dưới phải
}

//-------------------------------------------------------
//                    INPUT AND UPDATE 
//-------------------------------------------------------
void getInput() {
  Dabble.processInput();
  data.speedTurn = 0; // Reset before checking buttons
  data.speedForward = 0;

  // Read all 10 buttons for game controls and settings
  // dabbleInput.radius = (float) GamePad.getRadius() / 7.0 * 255.0; // power
  // dabbleInput.angle = (float) GamePad.getAngle() / 180.0 * PI; // Radian 
  dabbleInput.left = GamePad.isSquarePressed();
  dabbleInput.right = GamePad.isCirclePressed();
  dabbleInput.forward = GamePad.isUpPressed();
  dabbleInput.backward = GamePad.isDownPressed();
  // dabbleInput.left = GamePad.isLeftPressed();
  // dabbleInput.right = GamePad.isRightPressed();
  // dabbleInput.start  = GamePad.isStartPressed();
  // dabbleInput.select = GamePad.isSelectPressed();

  // data.power = dabbleInput.radius;
  // data.theta = dabbleInput.angle;
  // data.omega = dabbleInput.left * -90 + dabbleInput.right * 90;
  if(dabbleInput.left) data.speedTurn = -150;
  if(dabbleInput.right) data.speedTurn = 150;
  if(dabbleInput.forward) data.speedForward = 255;
  if(dabbleInput.backward) data.speedForward = -255;
}

//-------------------------------------------------------
//                    DEBUG 
//-------------------------------------------------------
String readWord() {
  char c;
  String word = "";
  while (true) {
    if (Serial.available() > 0) {
      c = Serial.read();
      if (c == ' ' || c == '\n')
        break;
      word += c;
    }
    else
      return "";
  }
  return word;
}

bool isValidNumber(String str) {
    if (str.length() == 0) return false;
    
    for (int i = 0; i < str.length(); i++) {
        // Allow a minus sign only at the very beginning
        if (i == 0 && str[i] == '-') {
            if (str.length() == 1) return false; // Case where input is just "-"
            continue;
        }
        // If any character isn't a digit, it's not a valid number
        if (!isdigit(str[i])) return false;
    }
    return true;
}

void serialCommand() {
  if (Serial.available() > 0) {
    bool valid = true;
    String cmdSelect = readWord(); // "power", "theta", "turn", "run", "debug", "restart"
    String cmdMode;
    // String cmdState;
    int stateVal;
    // Special command
    if (cmdSelect == "restart") {
      // Flush serial buffer 
      while (Serial.available() > 0) 
        char temp = Serial.read();
      ESP.restart();
    }
    else if (cmdSelect == "")
      return;
    
    cmdMode = readWord();
    /* 
    power: Công suất tổng (0 đến 255)
    theta: Góc di chuyển tính bằng Độ, sẽ convert sang Radian sau (0 - 360)
    turn: Tốc độ xoay (-255 đến 255, âm là xoay trái, dương là xoay phải)
    */
    // check cmdMode for numbers
    if (!isValidNumber(cmdMode))
        valid = false;
    
    // Process command
    if (valid) {
        int index = cmdMode.toInt();
        // if (cmdSelect == "power") {
        //     if (index >= 0 && index <= 255) {
        //         data.power = index;
        //         Serial.println("Power set to " + String(data.power));
        //     }
        //     else
        //         valid = false;
        // }
        // else if (cmdSelect == "theta") {
        //     if (index >= 0 && index <= 255){
        //         data.theta = (float)index / 180.0 * PI;
        //         Serial.println("Theta set to " + String(data.theta));
        //     }
        //     else
        //         valid = false;
        // }
        // else if (cmdSelect == "turn") {
        //     if (index >= -255 && index <= 255) {
        //         data.omega = index;
        //         Serial.println("Turn set to " + String(data.omega));
        //     }
        //     else
        //         valid = false;
        // }
        // else if (cmdSelect == "run") {
        //     if (index >= 1) {
        //         driveMecanum(data.power, data.theta, data.omega);
        //         Serial.println("Car running at: " + String(data.omega) + " " + String(data.omega) + " " + String(data.omega));
        //     }
        //     else
        //         valid = false;
        // }
        // else if (cmdSelect == "debug") {
        //     if (index >= 1) {
        //       debug = true;
        //       Serial.println("Debug set to 1");
        //     }
        //     else {
        //       debug = false;
        //       Serial.println("Debug set to 0");
        //     }
        // }
    }

    if (!valid) {
      Serial.println("Command not found: " + cmdSelect + " " + cmdMode);
      // Flush serial buffer
      while (Serial.available() > 0) 
        char temp = Serial.read();
    }
  }
}

//-------------------------------------------------------
//                    SETUP 
//-------------------------------------------------------
void setup() {
  Serial.begin(115200);

  Dabble.begin(BLUETOOTH_NAME);

  Serial.println("Dabble ready. Waiting for connection...");

  setupMotors();
  
  // if (!radio.begin()) {
  //   Serial.println("RF24 không khởi động được!");
  //   // while(1) delay(100);  // Dừng nếu lỗi
  // }
  // else {    
  //   radio.setPALevel(RF24_PA_LOW);
  //   radio.setDataRate(RF24_1MBPS);
  //   radio.setChannel(67);
  //   radio.openReadingPipe(0, address);
  //   radio.startListening();
  // }
  
  
  Serial.println("Xe sẵn sàng!");
}

//-------------------------------------------------------
//                    LOOP 
//-------------------------------------------------------
void loop() {
  // Get data
  if (millis() - dabble_counter.time >= dabble_counter.interval) {
    serialCommand();
    getInput();
    dabble_counter.time = millis();
  }
  
  // Nhận dữ liệu
  // if (radio.available()) {
  //     radio.read(&data, sizeof(data));
  //     last_receive_time = millis();
  //     Serial.println("Car running at: " + String(data.omega) + " " + String(data.omega) + " " + String(data.omega));
  //     Serial.printf("Gui thanh cong");
  // }

  if (millis() - motor_counter.time >= motor_counter.interval) {
    driveNormal(data.speedTurn, data.speedForward);
      motor_counter.time = millis();
  }

  
  // Debug (tắt khi chạy thật)
  // static uint32_t lastDebug = 0;
  // if (millis() - lastDebug > 200) {
  //     Serial.printf("X:%d Y:%d | L:%d R:%d\n", data.x_coor, data.y_coor, pwmX, pwmY);
  //     lastDebug = millis();
  // }
}