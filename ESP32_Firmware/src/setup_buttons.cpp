#include "setup_buttons.h"
#include "audio_setup.h"

const int button1Pin = 1; //yellow button
const int button2Pin = 0; //red button
const int button3Pin = 2; //blue button

void setupButtons(){
    pinMode(button1Pin, INPUT_PULLUP);
    pinMode(button2Pin, INPUT_PULLUP);
    pinMode(button3Pin, INPUT_PULLUP);
}
 
void waitForButton(int ledGreen,int ledBlue, const char* message) {
  Serial.println(message);
  digitalWrite(ledGreen, HIGH);
  digitalWrite(ledBlue, HIGH);

  while (true) {
    if (digitalRead(button1Pin) == LOW) {
      digitalWrite(ledGreen, LOW);
      digitalWrite(ledBlue, LOW);
      delay(300);
      return;
    }
    if (digitalRead(button3Pin) == LOW) {
      delay(300);
      clearRootFiles();
      playAudio("/keep/clear.wav");
    }
    delay(50);
    yield();
  }


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
