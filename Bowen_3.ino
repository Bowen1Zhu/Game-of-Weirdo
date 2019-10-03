#include <Servo.h>
#include "pitches.h"

const int LEDPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};              //pin numbers of red1, yellow1, blue1, green1, red2, yellow2, blue2, green2, respectively
const int buttonPins[8] = {22, 24, 26, 28, 30, 38, 34, 36};   //pin numbers of red1, yellow1, blue1, green1, red2, yellow2, blue2, green2, respectively
const int total_number = 8;
const int potentiometerPin = A0;                              //speed controller
const int chance_LEDs[3] = {10, 11, 12};                      //display the number of lives left
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
const int trigPin = 33;                                       //trigger pin of the distance sensor
const int echoPin = 35;                                       //echo pin of the distance sensor
long last_detection;                                          //last time when player's presence is detected
float interval;                                               //time limit of each round
int chance = 3;                                               //number of lives
int arr[8];                                                   //status of each color
                                                              //0--LED does not light up; 1--normal LED lights up; -1--opposite normal LED lights up
                                                              //2--weirdo lights up; -2--opposite weirdo lights up; -3--button correctly pressed; -4--button mistouched
int weirdo = 0;                                               //current weird color
int prev_weirdo = 0;                                          //previous weird color
Servo servo;                                                  //use continuous rotation servo so that it can rotate 360°

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A1));                                 //ensure randomness
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
  servo.write(86);                                            //a value near 90 means no movement
  weirdo = 0;                                                 //initialize weirdo as red_1
  prev_weirdo = 0;
}

void loop() {
  //the device is off initially
  //keep detecting while no player is detected
  while (!detect_distance()) {}

  //if player is detected but button is not pressed yet, switch to standby mode and remain until button press is detected
  while (!check_button_press()) {
    play_sequence();
    //if player disappears for over 5 seconds, turn off the device and keep detecting again
    if (millis() - last_detection > 5000) {
      all_LEDs_off();
      while (!detect_distance()) {}
    }
  }

  //finally, player is present and button is pressed, play a tune before game starts
  play_melody(start_melody, start_duration, start_notes_number);
  delay(500);

  //display lives
  chance = 3;
  for (int i = 0; i < chance; i ++) {    
    digitalWrite(chance_LEDs[i], HIGH);
  }

  //continue the game until chance reduces to zero
  while (chance) {
    //randomization before each round
    randomize_array();
    randomize_weirdo();

//    //debugging purpose
//    print_arr();
//    delay(5000);

    //display LEDs with full brightness
    light_LEDs(255);

    //parse player's input
    //if mistakes are detected, decrement chance before continuing
    if (!parse_input()) {
      digitalWrite(chance_LEDs[chance - 1], LOW);
      chance --;
    }
    all_LEDs_off();
  }
  //if chance reduces to zero, jump out of the loop and play the game over sound
  play_melody(lose_melody, lose_duration, lose_notes_number);

  //reset weirdo to red_1
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
      if (arr[j] == -1 || arr[j] == 2) {                      //blink LEDs that the player missed
        digitalWrite(LEDPins[j], i % 300 < 150);
      }
    }
    digitalWrite(chance_LEDs[chance - 1], i % 300 < 150);     //blink one of the life_display_LEDs
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
      detect_distance();                                      //there is a 50ms delay here
      digitalWrite(LEDPins[i], LOW);
    }
//  //blink all LEDs
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
  while (all_zero) {                                          //avoid the case when no LEDs light up
    for (int i = 0; i < total_number / 2; i ++) {             //iterate through 4 colors
      //***change the probability of LEDs being on/off in the if condition***
      if (random(25) / 10) {
        if (all_zero) {
          all_zero = false;                                   //we are now sure that at least one LED will light up
        }
        int r = random(2);                                    //for each color randomly pick one of the two LEDs
        arr[i + 4 * r] = 1;                                   //1 means on--this LED will light up
        arr[i + 4 * (1 - r)] = -1;                            //-1 means the opposite is on--this pushbutton thus needs to be pressed
      } else {
        arr[i] = 0;                                           //0 means do nothing
        arr[i + 4] = 0;
      }
    }
  }
}


void randomize_weirdo() {
  //***set the frequency of rotating the pointer in the if condition***
  if (true) {
    prev_weirdo = weirdo;
    weirdo = random(8);
  }

//  //this would be the easier way if standard servo could rotate 360°
//  servo.write(weirdo * 45);
//  delay(50);

//  //debugging purpose
//  Serial.println(weirdo);
  delay(500);
  move_servo();

  //mark the weirdo in arr by replacing 1 by 2
  //2 means weirdo lights up--pressing is needed
  //-2 means the opposite weirdo lights up--do not press this
  if (weirdo < total_number / 2) {
    arr[weirdo + 4] *= 2;
  } else {
    arr[weirdo - 4] *= 2;
  }
  arr[weirdo] *= 2;
}

void move_servo() {
  //rotate the servo for some time according to the angle
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
    //-1: opposite normal LED is on
    // 2: this weird LED is on
    //if any of the above has not been pressed, test is not passed yet
    if (arr[i] == -1 || arr[i] == 2) {
      return false;
    }
  }
  //if no more button that requires pressing is left unpressed, test is passed
  return true;
}

bool parse_input() {
  //read input from the speed controller
  //interval is set between 2.5s to 5s
  interval = map(analogRead(potentiometerPin), 0, 1023, 2500, 5000);
  long prev_time = millis();
  int brightness = 250;                                       //full brightness in the beginning
  while (millis() - prev_time < interval) {
//    //debugging purpose
//    if (millis() % 2000 == 0) {
//      print_arr();
//    }

    //keep reading input from the speed controller
    interval = map(analogRead(potentiometerPin), 0, 1023, 2500, 5000);
    //let LEDs gradually fade
    brightness = 250 * pow(1 - (millis() - prev_time) / interval, 2);//based on the interval and time elapsed
    light_LEDs(brightness);

    //detect player's input
    for (int i = 0; i < total_number; i ++) {
      if ((!arr[i]) || arr[i] == 1 || arr[i] == -2) {         //these buttons should not be pressed
        if (!digitalRead(buttonPins[i])) {                    //if any of them is pressed
//          //button yellow_2 may have some problem, double check
//          if (i == 5) {
//            bool error = false;
//            long pt = millis();
//            for (long tm = millis(); tm < pt + 100; tm = millis()) {
//              if (digitalRead(buttonPins[5] == 1)) {
//                error = true;
//              }
//            }
//            if (error) {
//              break;
//            }
//          }
          delay(100);                                         //first wait for 100ms as there could be a time delay between different buttons are pressed
          all_LEDs_off();
          //then report player's mistake for one second--blink mistouched LEDs, blink life display LED, generate error sound
          prev_time = millis();
          for (long j = millis(); j < prev_time + 1000; j = millis()) {
            for (int k = 0; k < total_number; k ++) {
              if (((!arr[k]) || arr[k] == 1 || arr[k] == -2) && (!digitalRead(buttonPins[k]))) {//keep checking as player may make more than one mistake
                //if weirdo is not pressed correctly, set the weirdo LED to HIGH again to inform the player
                if (arr[k] == -2) {
                  if (k < total_number / 2) {
                    digitalWrite(LEDPins[k + 4], HIGH);
                  } else {
                    digitalWrite(LEDPins[k - 4], HIGH);
                  }
                }
                //set all the mistouched buttons to some other numbers so that they can blink to inform the player
                arr[k] = -4;
              }
              if (arr[k] == -4) {
                digitalWrite(LEDPins[k], j % 300 < 150);
              }
            }
            //blink the life display LED at the same time
            digitalWrite(chance_LEDs[chance - 1], j % 300 < 150);
            //generate error sound
            if (j >  prev_time + 100 && j < prev_time + 200) {
              tone(buzzerPin, NOTE_G3, 300);
            } else if (j >  prev_time + 400 && j < prev_time + 500) {
              tone(buzzerPin, NOTE_C3, 300);
            }
          }
          //after reporting errors, stop and return false
          return false;
        }
      } else if (arr[i] == -1 && !digitalRead(buttonPins[i])) {//if player correctly presses the button whose opposite LED is on
        //generate a tone
        tone(buzzerPin, NOTE_C5, 200);
        //and use -3 to mark this button as correctly pressed
        arr[i] = -3;
      } else if (arr[i] == 2 && !digitalRead(buttonPins[i])) {//if player correctly presses the weirdo
//        //button yellow_2 may have some problem, double check
//        bool error = false;
//        if (i == 5) {
//          long pt = millis();
//          for (long tm = millis(); tm < pt + 50; tm = millis()) {
//            if (digitalRead(buttonPins[5] == 0)) {
//              error = true;
//            }
//          }
//        }
//        if (error) {
//          break;
//        } else {
          //generate a tone
          tone(buzzerPin, NOTE_C5, 200);
          //and use -3 to mark this button as correctly pressed
          arr[i] = 3;
//        }
      }
      if (check_passed()) {
        //if test is passed, play win_melody and return true
        play_melody(win_melody, win_duration, win_notes_number);
        delay(300);
        return true;
      }
    }
  }
  //after the loop, now the time interval has passed, which means timeout
  //the lighted LEDs have fade away; set them to HIGH again to inform the player
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
  //blink the buttons that player failed to press as well as the life display LED
  blink_LEDs();
  //generate error sound
  tone(buzzerPin, NOTE_G3, 300);
  delay(250);
  tone(buzzerPin, NOTE_C3, 300);
  delay(250);
  //after reporting the timeout, return false
  return false;
}

bool detect_distance() {
  float echoTime;                   //variable to store the time it takes for a ping to bounce off an object
  float calculatedDistance;         //variable to store the distance calculated from the echo time
  
  //send out an ultrasonic pulse that's 10ms long
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPin, LOW);

  echoTime = pulseIn(echoPin, HIGH);

  calculatedDistance = echoTime / 148.0;
  delay(50);
  
  if (calculatedDistance < 100) {
    //player detected, update last detection time
    last_detection = millis();
    return true;
  } else {
    //player not detected
    return false;
  }
}

void play_melody(int notes[], long duration[], int number) {
  for (int i = 0; i < number; i ++) {
    tone(buzzerPin, notes[i], duration[i]);
    delay(duration[i] * 1.1);
  }
}

//for debugging purpose
void print_arr() {
  for (int i = 0; i < total_number; i ++) {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(arr[i]);
  }
  Serial.println();
}
