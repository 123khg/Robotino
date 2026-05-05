#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(25, 5); // CE, CSN
const byte address[6] = "00001";
const int ledPin = 2; // LED có sẵn trên board ESP32
int counter = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  if (!radio.begin())
  {
    Serial.println("nRF24L01 không hoạt động!");
    while (1)
      ;
  }

  // 1. Ép công suất xuống thấp để tránh sụt áp và nhiễu (Dù có nguồn ngoài)
  radio.setPALevel(RF24_PA_LOW);

  // 2. Hạ tốc độ truyền xuống thấp nhất (Giúp chip clone chạy ổn định hơn)
  radio.setDataRate(RF24_250KBPS);

  // 3. Đổi kênh tần số để tránh trùng sóng WiFi 2.4GHz trong nhà
  radio.setChannel(110);

  // 4. Cố định độ dài gói tin để tránh lỗi Dynamic Payload trên hàng clone
  radio.enableDynamicPayloads(); // Cho phép gói tin dài ngắn tùy ý (từ 1 đến 32 byte)
  radio.setAutoAck(true);        // Tính năng này bắt buộc phải bật đi kèm với Dynamic Payload
  // 5. In toàn bộ thông số ra để kiểm tra địa chỉ và kết nối SPI
  radio.printDetails();
  radio.openWritingPipe(address);
  radio.stopListening(); // Đảm bảo module ở chế độ phát
  Serial.println("TX Initialized");

  // Các lệnh openWritingPipe hoặc openReadingPipe đặt sau các cấu hình trên
}

void loop()
{
  bool success = radio.write(&counter, sizeof(counter)); // Gửi dữ liệu

  if (success)
  {
    Serial.print("Gui thanh cong: ");
    Serial.println(counter);

    // Nháy LED khi truyền thành công
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);

    counter++;
  }
  else
  {
    Serial.println("Gui thất bại - Kiem tra nguồn/khoảng cách");
  }

  delay(1000); // Đợi 1 giây rồi gửi tiếp
}