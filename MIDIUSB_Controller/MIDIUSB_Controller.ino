#include <MIDIUSB.h>
#include <Wire.h>

//音源の数
#define OCILATOR_VALUE 3
//音源のMIDIチャンネルとI2Cアドレスの対応
byte channelByID[OCILATOR_VALUE] = {8, 9, 10};

//発音信号を受信したとき呼ばれる
//チャンネル、音階、音量
void noteOn(byte channel, byte pitch, byte velocity) {
  Serial.println("noteOn");
  if(channel > OCILATOR_VALUE) return; //チャンネルが音源の最大数より上なら何もしない
  Wire.beginTransmission(channelByID[channel - 1]); //I2C通信開始
  Wire.write(0b1001); //1001 = MIDIの発音命令
  Wire.write(channel);
  Wire.write(pitch);
  Wire.write(velocity);
  Wire.endTransmission(); //I2C通信終了
}

//発音停止信号を受信したとき呼ばれる
//チャンネル、音階、音量
void noteOff(byte channel, byte pitch, byte velocity) {
  Serial.println("noteOff");
  if(channel > OCILATOR_VALUE) return;
  Wire.beginTransmission(channelByID[channel - 1]);
  Wire.write(0b1000); //1001 = MIDIの発音停止命令
  Wire.write(channel);
  Wire.write(pitch);
  Wire.write(velocity);
  Wire.endTransmission();
}

//コントロールチェンジ信号を受信したとき呼ばれる
//チャンネル、操作項目、パラメーター
void controlChange(byte channel, byte control, byte value) {
  Serial.println("controlChange");
  Wire.beginTransmission(channelByID[channel - 1]);
  Wire.write(0b1011); //1011 = MIDIのコントロールチェンジ命令
  Wire.write(channel);
  Wire.write(control);
  Wire.write(value);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200); //デバッグ用にPCとシリアル通信する
  Wire.begin(); //I2C通信の開始
}



void loop() {
  midiEventPacket_t rx;
  do {

    rx = MidiUSB.read(); //通信を読み取る
    if (rx.header != 0) {
      //デバッグ用表示
      Serial.print("Received: ");
      Serial.print(rx.header, BIN);
      Serial.print("-");
      Serial.print(rx.byte1, BIN);
      Serial.print("-");
      Serial.print(rx.byte2, BIN);
      Serial.print("-");
      Serial.println(rx.byte3, BIN);
      if ((rx.byte1 & 0xf0) == 0x90) { //受け取ったのが発音信号なら
        if (rx.byte3 != 0) { //音量が0でなければ発音
          if ((rx.byte1 & 0x0f) < OCILATOR_VALUE) {
            noteOn((rx.byte1 & 0x0f) + 1, rx.byte2, rx.byte3);
          }
        } else { //0なら発音停止
          if ((rx.byte1 & 0x0f) < OCILATOR_VALUE) {
            noteOff((rx.byte1 & 0x0f) + 1, rx.byte2, rx.byte3);
          }
        }
      } else if ((rx.byte1 & 0xf0) == 0x80) { //受け取ったのが発音停止信号なら
        if ((rx.byte1 & 0x0f) < OCILATOR_VALUE) {
          noteOff((rx.byte1 & 0x0f) + 1, rx.byte2, rx.byte3);
        }
      } else if((rx.byte1 & 0xf0) == 0xB0) { //受け取ったのがコントロールチェンジ信号なら
        controlChange((rx.byte1 & 0x0f) + 1, rx.byte2, rx.byte3);
      }
    }
  } while (rx.header != 0);
}
