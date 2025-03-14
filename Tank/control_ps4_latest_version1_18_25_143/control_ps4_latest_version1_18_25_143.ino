#include <Arduino.h>
#include <PS4Controller.h>
#include <ESP32Servo.h>

// Right motor
int enableRightMotor = 22;
int rightMotorPin1 = 15;
int rightMotorPin2 = 21;

// Left motor
int enableLeftMotor = 23;
int leftMotorPin1 = 18;
int leftMotorPin2 = 19;

// Servos
Servo servo1; // Servo motor for up/down
Servo servo2; // Servo motor for left/right

const int led1Pin = 32; // Pin for LED 1 (controlled by L1)
const int led2Pin = 33; // Pin for LED 2 (controlled by R1)

// LED states and debounce variables
bool led1State = false;         // Current state of LED 1
bool lastL1ButtonState = false; // Previous state of the L1 button
bool led2State = false;         // Current state of LED 2
bool lastR1ButtonState = false; // Previous state of the R1 button


const int relayPin = 4; // Pin connected to the relay

// Servo pins
int servo1Pin = 26;
int servo2Pin = 27;

// Buzzer
int buzzerPin = 25;

// Maximum speed value (255 for HIGH or analogWrite equivalent)
const int MAX_SPEED = 255;

// Buzzer state and debounce variables
bool buzzerState = false;       // Current state of the buzzer
bool lastSquareButtonState = false; // Previous state of the Square button
unsigned long lastDebounceTime = 0; // Timestamp for debouncing
const unsigned long debounceDelay = 50; // Debounce delay in milliseconds

// Function to control motors
void rotateMotor(int rightMotorSpeed, int leftMotorSpeed) {
  // Right motor control
  if (rightMotorSpeed < 0) {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, HIGH);
    analogWrite(enableRightMotor, abs(rightMotorSpeed));
  } else if (rightMotorSpeed > 0) {
    digitalWrite(rightMotorPin1, HIGH);
    digitalWrite(rightMotorPin2, LOW);
    analogWrite(enableRightMotor, abs(rightMotorSpeed));
  } else {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, LOW);
    analogWrite(enableRightMotor, 0);
  }

  // Left motor control
  if (leftMotorSpeed < 0) {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, HIGH);
    analogWrite(enableLeftMotor, abs(leftMotorSpeed));
  } else if (leftMotorSpeed > 0) {
    digitalWrite(leftMotorPin1, HIGH);
    digitalWrite(leftMotorPin2, LOW);
    analogWrite(enableLeftMotor, abs(leftMotorSpeed));
  } else {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, LOW);
    analogWrite(enableLeftMotor, 0);
  }
}

// Set up pin modes
void setUpPinModes() {
  pinMode(enableRightMotor, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(enableLeftMotor, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT); // Set relay pin as output
  digitalWrite(relayPin, HIGH); // Initialize relay to OFF state

  pinMode(led1Pin, OUTPUT); // Set LED 1 pin as output
  digitalWrite(led1Pin, LOW); // Initialize LED 1 to OFF state
  pinMode(led2Pin, OUTPUT); // Set LED 2 pin as output
  digitalWrite(led2Pin, LOW); // Initialize LED 2 to OFF state


  // Attach servos
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
  // Set initial servo positions
  servo1.write(40);
  servo2.write(90);
  // Stop motors
  rotateMotor(0, 0);
}

// Notify function for PS4 controller inputs
void notify() {
  // Motor control with joysticks
  int rightMotorSpeed = map(PS4.RStickY(), -127, 127, -MAX_SPEED, MAX_SPEED);
  int leftMotorSpeed = map(PS4.LStickY(), -127, 127, -MAX_SPEED, MAX_SPEED);

  rightMotorSpeed = constrain(rightMotorSpeed, -MAX_SPEED, MAX_SPEED);
  leftMotorSpeed = constrain(leftMotorSpeed, -MAX_SPEED, MAX_SPEED);

  rotateMotor(rightMotorSpeed, leftMotorSpeed);


  if (PS4.Up()) {
    servo1.write(constrain(servo1.read() + 5, 0, 50)); // Move servo1 up
    Serial.println("Servo 1 moving up");
    }
  if (PS4.Down()) {
    servo1.write(constrain(servo1.read() - 5, 0, 35)); // Move servo1 down
    Serial.println("Servo 1 moving down");
    }
  if (PS4.Left()) {
    servo2.write(constrain(servo2.read() - 5, 0, 180)); // Move servo2 left
    Serial.println("Servo 2 moving left");
    }
  if (PS4.Right()) {
    servo2.write(constrain(servo2.read() + 5, 0, 180)); // Move servo2 right
    Serial.println("Servo 2 moving right");
    }
  

  // Buzzer control with Square button (with debouncing)
  bool currentSquareButtonState = PS4.Square(); // Read the current state of the Square button

  // Check if the button state has changed
  if (currentSquareButtonState != lastSquareButtonState) {
    lastDebounceTime = millis(); // Reset the debounce timer
  }

  // If the button state has been stable for the debounce delay, update the buzzer state
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentSquareButtonState != buzzerState) {
      buzzerState = currentSquareButtonState; // Update the buzzer state
      digitalWrite(buzzerPin, buzzerState ? HIGH : LOW); // Turn the buzzer on or off
      Serial.println(buzzerState ? "Buzzer ON" : "Buzzer OFF");
    }
  }

  lastSquareButtonState = currentSquareButtonState; // Save the current button state for the next loop

  // LED 1 control with L1 button (with debouncing)
  bool currentL1ButtonState = PS4.L1(); // Read the current state of the L1 button

  // Check if the button state has changed
  if (currentL1ButtonState != lastL1ButtonState) {
    lastDebounceTime = millis(); // Reset the debounce timer
  }
  // If the button state has been stable for the debounce delay, update the LED 1 state
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentL1ButtonState != led1State) {
      led1State = currentL1ButtonState; // Update the LED 1 state
      digitalWrite(led1Pin, led1State ? HIGH : LOW); // Turn the LED 1 on or off
      Serial.println(led1State ? "LED 1 ON" : "LED 1 OFF");
    }
  }
  lastL1ButtonState = currentL1ButtonState; // Save the current button state for the next loop

  // LED 2 control with R1 button (with debouncing)
  bool currentR1ButtonState = PS4.R1(); // Read the current state of the R1 button

  // Check if the button state has changed
  if (currentR1ButtonState != lastR1ButtonState) {
    lastDebounceTime = millis(); // Reset the debounce timer
  }
  // If the button state has been stable for the debounce delay, update the LED 2 state
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentR1ButtonState != led2State) {
      led2State = currentR1ButtonState; // Update the LED 2 state
      digitalWrite(led2Pin, led2State ? HIGH : LOW); // Turn the LED 2 on or off
      Serial.println(led2State ? "LED 2 ON" : "LED 2 OFF");
    }
  }
  lastR1ButtonState = currentR1ButtonState; // Save the current button state for the next loop


  // *Switch to Manual Mode (Disable ESP32 CAM)*
  if (PS4.Circle()) {
    digitalWrite(relayPin, LOW); // Turn relay ON
    Serial.println("Relay ON");
  }

  // *Return to Auto Mode (Enable ESP32 CAM)*
  if (PS4.Cross()) {
    digitalWrite(relayPin, HIGH); // Turn relay OFF
    Serial.println("Relay OFF");
  }
}

// On PS4 connect
void onConnect() {
  Serial.println("PS4 Controller Connected!");
}

// On PS4 disconnect
void onDisConnect() {
  rotateMotor(0, 0);
  Serial.println("PS4 Controller Disconnected!");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200); // Initialize UART communication (TX2/RX2 pins)

  // Initialize the buzzer pin to LOW (off)
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  setUpPinModes();

  // Initialize PS4 controller
  PS4.attach(notify);
  PS4.attachOnConnect(onConnect);
  PS4.attachOnDisconnect(onDisConnect);
  PS4.begin();

  Serial.println("Tank Ready!");
}

void loop() {
  // No additional code needed here
}