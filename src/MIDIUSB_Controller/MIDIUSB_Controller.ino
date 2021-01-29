#include <MIDIUSB.h>

void setup() {
  Serial.begin(115200); //デバッグ用にPCとシリアル通信する
  Serial1.begin(31250);
}



void loop() {
  midiEventPacket_t rx;
  do {

    rx = MidiUSB.read(); //通信を読み取る
    if (rx.header != 0) {
      //デバッグ用表示
      Serial.print("Received: ");
      //Serial.print(rx.header, BIN);
      //Serial.print("-");
      Serial.print(rx.byte1, BIN);
      Serial.print("-");
      Serial.print(rx.byte2, BIN);
      Serial.print("-");
      Serial.println(rx.byte3, BIN);
      Serial1.write(rx.byte1);
      Serial1.write(rx.byte2);
      Serial1.write(rx.byte3);
    }
  } while (rx.header != 0);
}
