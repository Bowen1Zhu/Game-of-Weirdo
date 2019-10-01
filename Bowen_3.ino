//comment will be added
#include <Servo.h>
#include "pitches.h"

const int LEDPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
const int buttonPins[8] = {22, 24, 26, 28, 30, 38, 34, 36};
const int total_number = 8;
const int potentiometerPin = A0;
const int chance_LEDs[3] = {10, 11, 12};
const int buzzerPin = 31;
int start_melody[] = {NOTE_C4, NOTE_C4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5};
long start_duration[] = {100, 100, 100, 300, 100, 300};
const int start_notes_number = 6;
int win_melody[] = {NOTE_C4, NOTE_G4, NOTE_C5};
long win_duration[] = {100, 100, 300};
const int win_notes_number = 3;
int lose_melody[] = {NOTE_G3, NOTE_C3, NOTE_G3, NOTE_C3};
long lose_duration[] = {250, 250, 250, 250};
const int lose_notes_number = 4;
const int trigPin = 33;
const int echoPin = 35;
long last_detection;
float interval;
int chance = 3;
int arr[8];
int weirdo = 0;
int prev_weirdo = 0;
Servo servo;

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A1));                                           //ensure randomness
  for (int i = 0; i < total_number; i ++) {
    pinMode(LEDPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  for (int i = 0; i < chance; i ++) {    
    pinMode(chance_LEDs[i], OUTPUT);
  }
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  servo.attach(37);
  servo.write(86);
  weirdo = 0;
  prev_weirdo = 0;
}

void loop() {
  while (!detect_distance()) {}

  while (!check_button_press()) {
    play_sequence();
    if (millis() - last_detection > 5000) {
      all_LEDs_off();
      while (!detect_distance()) {}
    }
  }
  
  play_melody(start_melody, start_duration, start_notes_number);
  delay(500);

  chance = 3;
  for (int i = 0; i < chance; i ++) {    
    digitalWrite(chance_LEDs[i], HIGH);
  }
  while (chance) {
    randomize_array();
    randomize_weirdo();

//    print_arr();
//    delay(10000);
    
    light_LEDs(255);

    if (!parse_input()) {
      digitalWrite(chance_LEDs[chance - 1], LOW);
      chance --;
    }
    all_LEDs_off();
//    delay(5000);
  }
  play_melody(lose_melody, lose_duration, lose_notes_number);
  
  prev_weirdo = weirdo;
  weirdo = 0;
  move_servo();
  
  for (int i = 0; i < 3; i ++) {
    play_sequence();
  }
}

void all_LEDs_off() {
  for (int i = 0; i < total_number; i ++) {
    digitalWrite(LEDPins[i], LOW);
  }
}

void light_LEDs(int brightness) {
  for (int i = 0; i < total_number; i ++) {
    if (arr[i] > 0) {
      analogWrite(LEDPins[i], brightness);
    }
  }
}

void blink_LEDs() {
  long prev_time = millis();
  for (long i = millis(); i < prev_time + 1000; i = millis()) {
    for (int j = 0; j < total_number; j ++) {
      if (arr[j] == -1 || arr[j] == 2) {
        digitalWrite(LEDPins[j], i % 300 < 150);
      }
    }
    digitalWrite(chance_LEDs[chance - 1], i % 300 < 150);
  }
}

bool check_button_press() {
  for (int i = 0; i < total_number; i ++) {
    if (!digitalRead(buttonPins[i])) {
      return true;
    }
  }
  return false;
}

void play_sequence() {
    for (int i = 0; i < total_number; i ++) {
      digitalWrite(LEDPins[i], HIGH);
      detect_distance();
      digitalWrite(LEDPins[i], LOW);
    }
//  for (int j = 0; j < 3; j ++) {
//    for (int i = 0; i < total_number; i ++) {
//      digitalWrite(LEDPins[i], HIGH);
//    }
//    delay(150);
//    for (int i = 0; i < total_number; i ++) {
//      digitalWrite(LEDPins[i], LOW);
//    }
//    delay(150);
//  }
}

void randomize_array() {
  bool all_zero = true;
  while (all_zero) {
    for (int i = 0; i < total_number / 2; i ++) {
      if (random(25) / 10) {
        if (all_zero) {
          all_zero = false;
        }
        int r = random(2);
        arr[i + 4 * r] = 1;
        arr[i + 4 * (1 - r)] = -1;
      } else {
        arr[i] = 0;
        arr[i + 4] = 0;
      }
    }
  }
}


void randomize_weirdo() {
  if (true) {
    prev_weirdo = weirdo;
    weirdo = random(8);
  }
  
//  servo.write(weirdo * 45);
//  delay(50);

  Serial.println(weirdo);
  delay(1000);
  move_servo();
  
  if (weirdo < total_number / 2) {
    arr[weirdo + 4] *= 2;
  } else {
    arr[weirdo - 4] *= 2;
  }
  arr[weirdo] *= 2;
}

void move_servo() {
  if (weirdo > prev_weirdo) {
    for (int i = 0; i < weirdo - prev_weirdo; i ++) {
      servo.write(103);
      delay(148);
      servo.write(86);
      delay(100);
    }
  } else if (weirdo < prev_weirdo) {
    for (int i = 0; i < weirdo - prev_weirdo + 8; i ++) {
      servo.write(103);
      delay(148);
      servo.write(86);
      delay(100);
    }
  }
}

bool check_passed() {
  for (int i = 0; i < total_number; i ++) {
    if (arr[i] == -1 || arr[i] == 2) {
      return false;
    }
  }
  return true;
}

bool parse_input() {
  interval = map(analogRead(potentiometerPin), 0, 1023, 2500, 5000);
  long prev_time = millis();
  int brightness = 250;
  while (millis() - prev_time < interval) {
//    if (millis() % 2000 == 0) {
//      print_arr();
//    }
    interval = map(analogRead(potentiometerPin), 0, 1023, 2500, 5000);
    brightness = 250 * pow(1 - (millis() - prev_time) / interval, 2);
    light_LEDs(brightness);
    for (int i = 0; i < total_number; i ++) {
      if ((!arr[i]) || arr[i] == 1 || arr[i] == -2) {
        if (!digitalRead(buttonPins[i])) {
          if (i == 5) {
            bool error = false;
            long pt = millis();
            for (long tm = millis(); tm < pt + 100; tm = millis()) {
              if (digitalRead(buttonPins[5] == 1)) {
                error = true;
              }
            }
            if (error) {
              break;
            }
          }
//          Serial.print(i);
//          Serial.print(" first wrong \n arr[5] ");
//          Serial.println(arr[5]);
//          Serial.println(!digitalRead(buttonPins[i]));
          delay(100);
          all_LEDs_off();
          prev_time = millis();
          for (long j = millis(); j < prev_time + 1000; j = millis()) {
            for (int k = 0; k < total_number; k ++) {
              if (((!arr[k]) || arr[k] == 1 || arr[k] == -2) && (!digitalRead(buttonPins[k]))) {
//                Serial.print(k);
//                Serial.print(" wrong \n arr[5] ");
//                Serial.println(arr[5]);
//                Serial.println(!digitalRead(buttonPins[k]));
                if (arr[k] == -2) {
                  if (k < total_number / 2) {
                    digitalWrite(LEDPins[k + 4], HIGH);
                  } else {
                    digitalWrite(LEDPins[k - 4], HIGH);
                  }
                }
                arr[k] = -4;
              }
              if (arr[k] == -4) {
                digitalWrite(LEDPins[k], j % 300 < 150);
              }
            }
            digitalWrite(chance_LEDs[chance - 1], j % 300 < 150);
            if (j >  prev_time + 100 && j < prev_time + 200) {
              tone(buzzerPin, NOTE_G3, 300);
            } else if (j >  prev_time + 400 && j < prev_time + 500) {
              tone(buzzerPin, NOTE_C3, 300);
            }
          }
          return false;
        }
      } else if (arr[i] == -1 && !digitalRead(buttonPins[i])) {
        tone(buzzerPin, NOTE_C5, 200);
        arr[i] = -3;
      } else if (arr[i] == 2 && !digitalRead(buttonPins[i])) {
        bool error = false;
        if (i == 5) {
          long pt = millis();
          for (long tm = millis(); tm < pt + 50; tm = millis()) {
            if (digitalRead(buttonPins[5] == 0)) {
              error = true;
            }
          }
        }
        if (error) {
          break;
        } else {
          tone(buzzerPin, NOTE_C5, 200);
          arr[i] = 3;
        }
      }
      if (check_passed()) {
        play_melody(win_melody, win_duration, win_notes_number);
        delay(300);
        return true;
      }
    }
  }
  for (int i = 0; i < total_number / 2; i ++) {
    if (arr[i] == -1) {
      digitalWrite(LEDPins[i + 4], HIGH);
    }
  }
  for (int i = 4; i < total_number; i ++) {
    if (arr[i] == -1) {
      digitalWrite(LEDPins[i - 4], HIGH);
    }
  }
  blink_LEDs();
  tone(buzzerPin, NOTE_G3, 300);
  delay(250);
  tone(buzzerPin, NOTE_C3, 300);
  delay(250);
  return false;
}

void print_arr() {
  for (int i = 0; i < total_number; i ++) {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(arr[i]);
  }
  Serial.println();
}

bool detect_distance()
{
  float echoTime;                   //variable to store the time it takes for a ping to bounce off an object
  float calculatedDistance;         //variable to store the distance calculated from the echo time
  
  //send out an ultrasonic pulse that's 10ms long
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPin, LOW);

  echoTime = pulseIn(echoPin, HIGH);

  calculatedDistance = echoTime / 148.0;
//  Serial.println(calculatedDistance);
  delay(50);
  
  if (calculatedDistance < 100) {
    last_detection = millis();
    return true;
  } else {
    return false;
  }
}

void play_melody(int notes[], long duration[], int number) {
  for (int i = 0; i < number; i ++) {
    tone(buzzerPin, notes[i], duration[i]);
    delay(duration[i] * 1.1);
  }
}
