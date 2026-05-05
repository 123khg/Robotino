#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(25, 5); // CE, CSN
const byte address[6] = "00001";
const int ledPin = 2;

void setup()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  if (!radio.begin())
  {
    Serial.println("nRF24L01 không hoạt động!");
    delay(1000);
  }
  Serial.print("Success");
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
  radio.openReadingPipe(1, address);

  // Mẹo: Ép địa chỉ Pipe 0 giống Pipe 1 để hỗ trợ ACK tốt hơn
  radio.openReadingPipe(0, address);

  // Kiểm tra lại CONFIG sau khi startListening
  Serial.print("Config Register: ");
  radio.startListening();

  radio.printDetails();
}

void loop()
{
  if (radio.available())
  {
    int receivedData = 0;
    radio.read(&receivedData, sizeof(receivedData));
    Serial.print("Da nhan: ");
    Serial.println(receivedData);

    // Nháy LED khi nhận thành công
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
  }
}