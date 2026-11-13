/*
Cualquier cosa me mandan un wasap 
si es muy urgente llamada al 311 367 7343
si estoy libre me hacen video llamada para ver cuando compitan
*/

// --- Configurar espacio ---

// Asegurar tener internet y no andar de chachuchas queriendo hacer todo esto el dia del RoboTiger

// Ir a Boards Manager, buscar "esp32" e instalar la opcion by Espressif Systems, NO la de Arduino
// Seleccionar puerto COMX y board "ESP32 Dev Module"


// Activator, false para calibrar y pruebas, true para competir
bool competitionMode = false;

// PID
// Constante proporcional, empezar en 0.01f, aumentar de 0.005f Ej: 0.01f -> 0.015f -> 0.02f
const float KP = 0.0f;
// Constante integral, siempre 0.0f
const float KI = 0.0f;
// Constante derivativa, 0.0f mientras se calibra KP, una vez KP es definitivo empezar en 0.001f, aumentar de 0.0005f Ej: 0.001f -> 0.0015f -> 0.002f
// tratar de pulir una vez encontrada un valor confiable Ej 0.003 -> 0.0029 -> 0.0028                                 
const float KD = 0.0f;

// Speed
// No tocar, fijo a 255
int maxSpeed = 255;

/* Velocidad del robot, depende del tipo de pista
Guia rapida de elejir baseSpeed
Para pistas dificiles (como la del CCLT) usar de 80 a 120
Para pistas faciles (pocas curvas, largas y rectas como le gustan a gabo) usar minimo 120 y aumentar de 10 en 10
si pueden mandarme foto, video o hacerme video llamada para que pueda recomendarles un valor mucho mejor */
int baseSpeed = 50;

// Calibrar el dia del evento, ya que la iluminacion ambintal puede llegar a alterar el resultado.
const int nonLineSum = 2000;


// Evitar mover el codigo apartir de aqui para evitar que la cajetien y valga bencha


// Pins
const uint8_t pwma = 4;
const uint8_t pwmb = 18;
const uint8_t bin2 = 19;
const uint8_t bin1 = 21;
const uint8_t ain1 = 22;
const uint8_t ain2 = 23;

const uint8_t numSensors = 8;
const uint8_t qtrPins[numSensors] = {34, 35, 32, 33, 25, 26, 27, 14};

const uint8_t smdLed = 2;

const int CH_A = 0;
const int CH_B = 1;

const int RDY = 16;
const int GO = 17;

// Constants
const uint16_t calibrationTime = 10000;
const unsigned long sampleTimeUs = 1000UL;

// Variables
uint16_t rawSensorValues[numSensors];
uint16_t fixedSensorValues[numSensors];
uint16_t maxSensorValues[numSensors];
uint16_t minSensorValues[numSensors];

int weight[numSensors] = {-3500, -2500, -1500, -500, 500, 1500, 2500, 3500};

int pos;
int lastPos = 0;
long sensorSignal = 0;
int sumB;

unsigned long lastLoopTimeUs = 0;

float lastError = 0.0f;
float integralTerm = 0.0f;

const float integralLimit = 20000.0f;

const float errorFilterAlpha = 0.35f;
float filteredError = 0.0f;
bool filterInitialized = false;

const int maxOutputStep = 35;
int lastOutput = 0;

// Debug
unsigned long debugCounter = 0;
bool debug = true;

// Functions
void waitTrigger();
void readSensorBar();
void calibrateSensors();
int getPos();
void moveMotors(int leftSpeed, int rightSpeed);
void stopMotors();

// Setup
void setup() {
    Serial.begin(115200);

    pinMode(pwma, OUTPUT);
    pinMode(pwmb, OUTPUT);
    pinMode(ain1, OUTPUT);
    pinMode(ain2, OUTPUT);
    pinMode(bin1, OUTPUT);
    pinMode(bin2, OUTPUT);

    pinMode(RDY, INPUT);
    pinMode(GO, INPUT);

    for (int i = 0; i < numSensors; i++) {
        analogSetPinAttenuation(qtrPins[i], ADC_11db);
    }

    // Retained ESP32 Core v2.x API
    ledcSetup(CH_A, 20000, 8);
    ledcSetup(CH_B, 20000, 8);
    ledcAttachPin(pwma, CH_A);
    ledcAttachPin(pwmb, CH_B);

    pinMode(smdLed, OUTPUT);

    stopMotors();

    calibrateSensors();

    Serial.println("=== Calibracion terminada ===");
    for (int i = 0; i < numSensors; i++) {
        Serial.print("Sensor "); Serial.print(i);
        Serial.print(" -> min: "); Serial.print(minSensorValues[i]);
        Serial.print(" | max: "); Serial.println(maxSensorValues[i]);
    }

    waitTrigger();

    lastLoopTimeUs = micros();
}

// Loop
void loop() {
    if (digitalRead(GO) == LOW && competitionMode) {
        stopMotors();
        return;
    } else {
            unsigned long nowUs = micros();

        if (nowUs - lastLoopTimeUs < sampleTimeUs) {
            return;
        }

        float dt = (nowUs - lastLoopTimeUs) / 1000000.0f;
        lastLoopTimeUs = nowUs;

        if (dt <= 0.0f) {
            return;
        }

        pos = getPos();

        if (!filterInitialized) {
            filteredError = pos;
            filterInitialized = true;
        } else {
            filteredError = errorFilterAlpha * pos +
                        (1.0f - errorFilterAlpha) * filteredError;
        }

        float error = filteredError;

        integralTerm += error * dt;

        if (integralTerm > integralLimit) integralTerm = integralLimit;
        if (integralTerm < -integralLimit) integralTerm = -integralLimit;

        float derivative = (error - lastError) / dt;
        lastError = error;

        float output = (KP * error) + (KI * integralTerm) + (KD * derivative);

        int desiredOutput = (int)output;

        if (desiredOutput - lastOutput > maxOutputStep) {
            desiredOutput = lastOutput + maxOutputStep;
        } else if (desiredOutput - lastOutput < -maxOutputStep) {
            desiredOutput = lastOutput - maxOutputStep;
        }

        lastOutput = desiredOutput;

        int requestedLeftSpeed  = baseSpeed + desiredOutput;
        int requestedRightSpeed = baseSpeed - desiredOutput;

        int leftSpeed  = constrain(requestedLeftSpeed, -maxSpeed, maxSpeed);
        int rightSpeed = constrain(requestedRightSpeed, -maxSpeed, maxSpeed);
        bool outputSaturated = leftSpeed != requestedLeftSpeed ||
                           rightSpeed != requestedRightSpeed;

        moveMotors(leftSpeed, rightSpeed);

        if (debug) {
            debugCounter++;

            if (debugCounter % 200 == 0) {
                for (int i = 0; i < numSensors; i++) {
                    Serial.print(fixedSensorValues[i]);
                    Serial.print("\t");
                }

                Serial.print("Pos: ");
                Serial.print(pos);
                Serial.print(" | Sum: ");
                Serial.println(sumB);
            }
        }
    }

}

// Trigger condition
void waitTrigger() {
    if (!competitionMode) {
        delay(2000);
    }
}

// Sensor
void readSensorBar() {
    for (int i = 0; i < numSensors; i++) {
        rawSensorValues[i] = analogRead(qtrPins[i]);

        if (maxSensorValues[i] <= minSensorValues[i] + 10) {
            fixedSensorValues[i] = 0;
        } else {
            fixedSensorValues[i] = constrain(
                map(rawSensorValues[i],
                    minSensorValues[i],
                    maxSensorValues[i],
                    0,
                    1000),
                0,
                1000
            );
        }
    }
}

// Calibration
void calibrateSensors() {
    digitalWrite(smdLed, HIGH);

    unsigned long start = millis();

    for (int i = 0; i < numSensors; i++) {
        minSensorValues[i] = 4095;
        maxSensorValues[i] = 0;
    }

    while (millis() - start < calibrationTime) {
        for (int i = 0; i < numSensors; i++) {
            rawSensorValues[i] = analogRead(qtrPins[i]);

            if (rawSensorValues[i] > maxSensorValues[i]) {
                maxSensorValues[i] = rawSensorValues[i];
            }

            if (rawSensorValues[i] < minSensorValues[i]) {
                minSensorValues[i] = rawSensorValues[i];
            }

            if (debug) {
                debugCounter++;

                if (debugCounter % 300 == 0) {
                    for (int i = 0; i < numSensors; i++) {
                        Serial.print(rawSensorValues[i]);
                        Serial.print("\t");
                    }
                    Serial.println("\t");
                }
            }
        }
    }

    digitalWrite(smdLed, LOW);
}

int getPos() {
    long sumA = 0;
    sumB = 0;

    readSensorBar();

    for (int i = 0; i < numSensors; i++) {
        sumA += (long)fixedSensorValues[i] * weight[i];
        sumB += fixedSensorValues[i];
    }

    sensorSignal = sumB;

    if (sumB < nonLineSum) {
        return lastPos;
    }

    lastPos = sumA / sumB;
    return lastPos;
}

// Motors
void moveMotors(int leftSpeed, int rightSpeed) {
    if (leftSpeed >= 0) {
        digitalWrite(ain1, HIGH);
        digitalWrite(ain2, LOW);
    } else {
        digitalWrite(ain1, LOW);
        digitalWrite(ain2, HIGH);
        leftSpeed = -leftSpeed;
    }

    ledcWrite(CH_A, leftSpeed);

    if (rightSpeed >= 0) {
        digitalWrite(bin1, HIGH);
        digitalWrite(bin2, LOW);
    } else {
        digitalWrite(bin1, LOW);
        digitalWrite(bin2, HIGH);
        rightSpeed = -rightSpeed;
    }

    ledcWrite(CH_B, rightSpeed);
}

void stopMotors() {
    digitalWrite(ain1, LOW);
    digitalWrite(ain2, LOW);
    digitalWrite(bin1, LOW);
    digitalWrite(bin2, LOW);
    ledcWrite(CH_A, 0);
    ledcWrite(CH_B, 0);
}
