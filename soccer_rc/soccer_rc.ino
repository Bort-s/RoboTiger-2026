// Codigo Robot Soccer

// --- Configurar espacio ---

// Asegurar tener internet y no andar de chachuchas queriendo hacer todo esto el dia del RoboTiger

// Ir a Boards Manager, buscar "esp32" e instalar la opcion by Espressif Systems, NO la de Arduino
// Seleccionar puerto COMX y board "ESP32 Dev Module"

// Ir a https://github.com/adafruit/Adafruit_NeoPixel/archive/refs/tags/1.15.5.zip y descargar
// Ir a https://github.com/madhephaestus/ESP32Servo/archive/refs/tags/3.2.1.zip y descargar
// Ir a https://github.com/Bort-s/RoboTiger-2026/archive/refs/tags/V1.67.zip y descargar
// IMPORTANTE: No descomprimir
// Para los 3 zips: Sketch -> Include library -> Add .ZIP labrary

#include <ps5Controller.h>
#include <ESP32Servo.h> 
#include <Adafruit_NeoPixel.h>

// true para Gabox, false para Bort
bool isGabox = false;

// Velocidad de giro
const int maxGyroSpeed = 80;
// Velocidad normal (adelante)
const int maxForwardSpeed = 200;
// Velocidad para atraz
const int maxBackwardSpeed = 180;


// Evitar mover el codigo apartir de aqui para evitar que la cajetien y valga bencha


// Pins
const int PWMA = 32;
const int AIN2 = 25;
const int AIN1 = 33;

const int BIN1 = 26;
const int BIN2 = 27;
const int PWMB = 14;

const int SMD_LED = 16;
const int SERVO_PIN = 13;
const int button_pin = 22;


// PWM channels
const int CH_A = 2;
const int CH_B = 3;


// Constants
const char* controllerMac[] = {
  "10:18:49:74:EE:8A",
  "AC:36:1B:D9:2E:E7"
};

const int deadzone = 20;


// Variables
bool isControlConnected = false;
bool isSquarePushed = false;
bool isCrossPushed = false;
bool isL2Pushed = false;
bool isR2Pushed = false;

int valueL2;
int valueR2;
int leftStickHorizontal;
int differential;

int forwardSpeed;
int backwardSpeed;
int gyroSpeed;
int VConstant;

char controlCase;


// Servo
bool launchBallCoroutine = false;
bool inBallCoroutine = false;
int launcherNormal = 55;
int launcherTarget;

unsigned long servoStartMillis = 0;
const unsigned long servoDuration = 150;

// Control loop timing
unsigned long previousControlMillis = 0;
const unsigned long controlInterval = 20;

int currentTime;


// Function declarations
void forward(int lspeed, int rspeed);
void backward(int lspeed, int rspeed);
void right(int speed);
void left(int speed);
void stop();
void readControl();
void caseSelecter();
void set_led(int r, int g, int b);

Servo servomotor;

Adafruit_NeoPixel smd_led = Adafruit_NeoPixel(1, SMD_LED, NEO_GRB + NEO_KHZ800);

// Setup
void setup() {
  Serial.begin(115200);

  smd_led.begin();
  smd_led.clear();
  set_led(255, 0, 0);
  smd_led.show();

  if (isGabox) ps5.begin(controllerMac[1]);
  else ps5.begin(controllerMac[0]);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  pinMode(SMD_LED, OUTPUT);


  servomotor.setPeriodHertz(50);
  servomotor.attach(SERVO_PIN, 500, 2400);
  servomotor.write(launcherNormal);


  ledcSetup(CH_A, 20000, 8);
  ledcSetup(CH_B, 20000, 8);

  ledcAttachPin(PWMA, CH_A);
  ledcAttachPin(PWMB, CH_B);

  launcherTarget = launcherNormal + 90;
}


// Loop
void loop() {
  isControlConnected = ps5.isConnected();

  if (!isControlConnected) {
    stop();
    set_led(255, 50, 0);
  } else {
    if (isGabox) set_led(0, 0, 255);
    else set_led(0, 255, 0);

    currentTime = millis();

    if (currentTime - previousControlMillis >= controlInterval) {
      previousControlMillis = currentTime;

      readControl();
      caseSelecter();

      switch (controlCase) {
        case 'F':
          forward(forwardSpeed, forwardSpeed);
          break;

        case 'B':
          backward(backwardSpeed, backwardSpeed);
          break;

        case 'L':
          left(gyroSpeed);
          break;

        case 'R':
          right(gyroSpeed);
          break;

        case 'H':
          // Left-Forward
          forward(VConstant, forwardSpeed);
          break;

        case 'G':
          // Right-Forward
          forward(forwardSpeed, VConstant);
          break;

        case 'I':
          // Right-Backward
          backward(backwardSpeed, VConstant);
          break;

        case 'J':
          // Left-Backward
          backward(VConstant, backwardSpeed);
          break;

        default:
          stop();
          break;
      }
    }

    // Launch ball
    if (ps5.Cross() && !inBallCoroutine) {
      inBallCoroutine = true;
      launchBallCoroutine = true;

      servoStartMillis = millis();

      servomotor.write(launcherTarget);
    }

    if (launchBallCoroutine && millis() - servoStartMillis >= servoDuration) {
      servomotor.write(launcherNormal);

      launchBallCoroutine = false;
      inBallCoroutine = false;
    }
  }
}


// Motor logic

void forward(int lspeed, int rspeed) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  ledcWrite(CH_A, lspeed);

  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  ledcWrite(CH_B, rspeed);
}


void backward(int lspeed, int rspeed) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  ledcWrite(CH_A, lspeed);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  ledcWrite(CH_B, rspeed);
}

void right(int speed) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  ledcWrite(CH_A, speed);

  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  ledcWrite(CH_B, speed);
}

void left(int speed) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  ledcWrite(CH_A, speed);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  ledcWrite(CH_B, speed);
}

void stop() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  ledcWrite(CH_A, 0);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
  ledcWrite(CH_B, 0);
}


void readControl() {
  isCrossPushed = ps5.Cross();
  isSquarePushed = ps5.Square();
  isL2Pushed = ps5.L2();
  isR2Pushed = ps5.R2();

  leftStickHorizontal = ps5.LStickX();

  valueL2 = ps5.L2Value();
  valueR2 = ps5.R2Value();

  differential = valueR2 - valueL2;

  if (differential > 0){
    isL2Pushed = false;
  }

  else if (differential < 0) {
    isR2Pushed = false;
  }

  forwardSpeed = map(abs(differential), 0, 255, 0, maxForwardSpeed);
  backwardSpeed = map(abs(differential), 0, 255, 0, maxBackwardSpeed);
  gyroSpeed = map(abs(leftStickHorizontal), 0, 128, 0, maxGyroSpeed);
  VConstant = map(abs(leftStickHorizontal), 0, 128, abs(differential), 0);
}


void caseSelecter() {
  if (isL2Pushed || isR2Pushed) {
    if (leftStickHorizontal < -deadzone) {
      if (isR2Pushed) {
        controlCase = 'G';
      }
      else {
        controlCase = 'J';
      }

    } else if (leftStickHorizontal > deadzone) {
      if (isR2Pushed) {
        controlCase = 'H';
      }
      else {
        controlCase = 'I';
      }
    } else {
      if (isR2Pushed) {
        controlCase = 'F';
      }
      else {
        controlCase = 'B';
      }
    }

  } else if (leftStickHorizontal < -deadzone) {
    controlCase = 'L';
  } else if (leftStickHorizontal > deadzone) {
    controlCase = 'R';
  } else {
    controlCase = 'S';
  }
}

void set_led(int r, int g, int b) {
  smd_led.setPixelColor(0, smd_led.Color(r, g, b));
  smd_led.show();
}
