//
//  Author: sorasuke(Twitter: @sora_suke_mc)
//
//  Runnable Arduino: Uno (ATmega328P)
//
//

#include <SoftwareSerial.h>
#include <MIDI.h>
#include <MozziGuts.h>
#include <mozzi_midi.h>
#include <Oscil.h>
#include <Ead.h>
#include <ADSR.h>
#include <tables/sin2048_int8.h>
#include <tables/square_analogue512_int8.h>
#include <tables/saw1024_int8.h>

#define CONTROL_RATE 128 //Mozziのコントロールレート

/*//正弦波のオシレーター
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> ocil1(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> ocil2(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> ocil3(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> ocil4(SIN2048_DATA);
*/
//矩形波のオシレーター
  Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> ocil1(SQUARE_ANALOGUE512_DATA);
  Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> ocil2(SQUARE_ANALOGUE512_DATA);
  Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> ocil3(SQUARE_ANALOGUE512_DATA);
  Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> ocil4(SQUARE_ANALOGUE512_DATA);


/*//ノコギリ波のオシレーター
  Oscil <SAW1024_NUM_CELLS, AUDIO_RATE> ocil1(SAW1024_DATA);
  Oscil <SAW1024_NUM_CELLS, AUDIO_RATE> ocil2(SAW1024_DATA);
  Oscil <SAW1024_NUM_CELLS, AUDIO_RATE> ocil3(SAW1024_DATA);
  Oscil <SAW1024_NUM_CELLS, AUDIO_RATE> ocil4(SAW1024_DATA);
*/

//エンベロープ
ADSR<AUDIO_RATE, AUDIO_RATE> envelope1;
ADSR<AUDIO_RATE, AUDIO_RATE> envelope4;
ADSR<AUDIO_RATE, AUDIO_RATE> envelope2;
ADSR<AUDIO_RATE, AUDIO_RATE> envelope3;

byte MIDI_CHANNEL = 0;

#define SOUNDS_VALUE 4 //同時発音可能数(オシレーターの数)
byte notesPitch[4] = {0, 0, 0, 0}; //4つのオシレーターの音階
byte notesVolume[4] = {0, 0, 0, 0}; //4つのオシレーターの音量
bool notesEnable[4] = {false, false, false, false}; //4つのオシレーターが有効かどうか

#define SOUNDS_VALUE 4

#define IS_DEBUG_MODE false

//全体の音量
byte allVolume = 255;

//ADSR
int atk = 50;
int dec = 100;
int sus = 10000;
int rel = 500;
byte atk_vol = 255;
byte dec_vol = 64;

//ADSRが有効か
bool adsrOn = true;

//仮のピンアサイン
//SoftwareSerial MIDISerial(3, 4); //RX TX
//MIDI_CREATE_INSTANCE(SoftwareSerial, MIDISerial, sMIDI); //sMIDIがMIDI.beginとかやる
MIDI_CREATE_DEFAULT_INSTANCE();

//オシレーターのリセットスイッチのピン
#define BTN_RST_OCL 2

//リセットスイッチの以前の状態
byte BTN_OCL_PRE = 0;

//発音信号を受け取ったとき呼ばれる
//チャンネル、音階、音量
void noteOn(byte channel, byte pitch, byte velocity)
{
  debugPrint("noteOn");
  for (int i = 0; i < SOUNDS_VALUE; i++) //未使用のオシレーターを探す
  {
    if (notesEnable[i] == false) //i番のオシレーターが未使用なら
    {
      //受け取った値を代入しオシレーターが有効に設定する
      notesPitch[i] = pitch;
      notesVolume[i] = velocity;
      notesEnable[i] = true;
      //エンベロープを開始する
      switch (i)
      {
        case 0:
          envelope1.noteOn();
          break;
        case 1:
          envelope2.noteOn();
          break;
        case 2:
          envelope3.noteOn();
          break;
        case 3:
          envelope4.noteOn();
          break;
      }

      break;
    }
  }
}

//発音停止信号を受け取ったとき呼ばれる
//チャンネル、音階、音量
void noteOff(byte channel, byte pitch, byte velocity)
{
  debugPrint("noteOff");
  for (int i = 0; i < SOUNDS_VALUE; i++)
  {
    if (notesPitch[i] == pitch)
    {
      if (!adsrOn) //ADSRがオンだと余韻を残すため音量を0にはしない
        notesVolume[i] = 0;
      notesEnable[i] = false;
      //余韻の開始
      switch (i)
      {
        case 0:
          envelope1.noteOff();
          break;
        case 1:
          envelope2.noteOff();
          break;
        case 2:
          envelope3.noteOff();
          break;
        case 3:
          envelope4.noteOff();
          break;
      }
      break;
    }
  }
}

//コントロールチェンジ
void controlChange(byte channel, byte number, byte value) {
  debugPrint("controlChange");
  if (number == 0b0111) { //0111 = 全体の音量について
    allVolume = value * 2;
  } else if (number == 21) { // エンベロープの有効か無効か
    adsrOn = value > 63;
  } else if (number == 22) { //エンベロープのAttack時間
    atk = value * 10;
  } else if (number == 23) { //エンベロープのDecay時間
    dec = value * 10;
  } else if (number == 24) { //エンベロープのSustain時間
    sus = value * 100;
  } else if (number == 25) { //エンベロープのRelease時間
    rel = value * 100;
  } else if (number == 26) { //エンベロープAttackの音量
    atk_vol = value * 2;
  } else if (number == 27) { //エンベロープDecayの音量
    dec_vol = value * 2;
  }
  setEnvelopes();
}

//オシレーターをすべてリセット
void clearOcilators() {
  for (int i = 0; i < SOUNDS_VALUE; i++)
  {
    notesVolume[i] = 0;
    notesEnable[i] = false;
  }
  debugPrint("RESET OCILATORS");
}

void setEnvelopes() {
  //エンベロープの設定
  envelope1.setADLevels(atk_vol, dec_vol);
  envelope1.setTimes(atk, dec, sus, rel);
  envelope2.setADLevels(atk_vol, dec_vol);
  envelope2.setTimes(atk, dec, sus, rel);
  envelope3.setADLevels(atk_vol, dec_vol);
  envelope3.setTimes(atk, dec, sus, rel);
  envelope4.setADLevels(atk_vol, dec_vol);
  envelope4.setTimes(atk, dec, sus, rel);
}

//MozziにてUpdateの代わりにここに記述
void updateControl()
{

  MIDI.read();





  //オシレーターの周波数を設定
  //mtof関数はMIDIの音階番号を周波数にする
  ocil1.setFreq(mtof(notesPitch[0]));
  ocil2.setFreq(mtof(notesPitch[1]));
  ocil3.setFreq(mtof(notesPitch[2]));
  ocil4.setFreq(mtof(notesPitch[3]));
  /*if (Serial.available())
  {
    char sel = Serial.read();
    debugPrint(sel);
    if (sel == 'r') { //シリアルで'r'を受信したらオシレーターのリセット
      clearOcilators();
    }

  }*/

  //ボタンが押されたらオシレーターとI2Cのリセット
  if (digitalRead(BTN_RST_OCL) && !BTN_OCL_PRE) {
    clearOcilators();
  }
  //最後に今のボタンの状態を以前のボタンの状態の変数に記録させる
  //そうすることで前のループで押されていたかを記憶し、押された瞬間を知ることができる
  BTN_OCL_PRE = digitalRead(BTN_RST_OCL);
}

//Mozziの音声についてのアップデート
int updateAudio()
{

  //エンベロープを更新
  envelope1.update();
  envelope2.update();
  envelope3.update();
  envelope4.update();
  //瞬間の波形の高さを入れる変数
  int ret;


  ret += (int) //最終的にintにする
         (((
             (adsrOn ? envelope1.next() : 255) * ocil1.next()
           ) >> 8)
          * notesVolume[0]) >> 7;
  //以下同様

  ret += (int)((((adsrOn ? envelope2.next() : 255) * ocil2.next()) >> 8) * notesVolume[1]) >> 7;
  ret += (int)((((adsrOn ? envelope3.next() : 255) * ocil3.next()) >> 8) * notesVolume[2]) >> 7;
  ret += (int)((((adsrOn ? envelope4.next() : 255) * ocil4.next()) >> 8) * notesVolume[3]) >> 7;
  //戻り値は瞬間の波形の高さ
  return (int)((ret >> 2) * allVolume) >> 8; //出力できる最大から溢れないようにする
}

void setup() {
  //Serial.begin(115200);

  pinMode(BTN_RST_OCL, INPUT_PULLUP);
  pinMode(13, OUTPUT);

  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);

  byte b4 = digitalRead(5) == LOW;
  byte b3 = digitalRead(6) == LOW;
  byte b2 = digitalRead(7) == LOW;
  byte b1 = digitalRead(8) == LOW;
  
  MIDI_CHANNEL = (b4 << 3) + (b3 << 2) + (b2 << 1) + b1;
  //Serial.println(MIDI_CHANNEL);

  MIDI.setHandleNoteOn(noteOn);
  MIDI.setHandleNoteOff(noteOff);
  MIDI.setHandleControlChange(controlChange);
  MIDI.begin(MIDI_CHANNEL);

  startMozzi(CONTROL_RATE); //Mozziを開始する
  setEnvelopes();
  debugPrint("begin");
}

void debugPrint(char* c) {
  if (IS_DEBUG_MODE)
    Serial.println(c);
}

void loop() {
  //音声処理を続けるためこれのみをloop内には記述する
  audioHook();
}
