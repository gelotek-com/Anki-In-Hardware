#include "setup_buttons.h"

const int button1Pin = 1; //yellow button
const int button2Pin = 0; //red button
const int button3Pin = 2; //blue button

void setupButtons(){
    pinMode(button1Pin, INPUT_PULLUP);
    pinMode(button2Pin, INPUT_PULLUP);
    pinMode(button3Pin, INPUT_PULLUP);
}
 
void waitForButton(int pin, int led, const char* message) {
  Serial.println(message);
  digitalWrite(led, HIGH);
  while (digitalRead(pin) == HIGH) {
    delay(50);
    yield();
  }
  digitalWrite(led, LOW);
  delay(300);
}

int waitForStudy(int pin, int pin2, int led, const char* message) {
  Serial.println(message);
  digitalWrite(led, HIGH);
  while (true) {
    if (digitalRead(pin) == LOW) { // yellow for 1
      digitalWrite(led, LOW);
      delay(300);
      return 1;
    }
    if (digitalRead(pin2) == LOW) { // red for 4
      digitalWrite(led, LOW);
      delay(300);
      return 4;
    }
    delay(50);
    yield();
  }
}
