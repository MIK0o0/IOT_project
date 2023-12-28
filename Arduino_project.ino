#include <Servo.h>

//-----Declare servos and variables
Servo recoil_servo;
Servo pan_servo;
Servo tilt_servo;

const byte pan_limit_1 = 0;
const byte pan_limit_2 = 180;
const byte tilt_limit_1 = 65;
const byte tilt_limit_2 = 180;
const byte reload_rest = 180;    
const byte recoil_pushed = 125;  

//-----Variables related to serial data handling
byte byte_from_app;
const byte buffSize = 30;
byte recived_data[buffSize];
const byte startMarker = 255;
const byte endMarker = 254;
byte bytesRecvd = 0;
boolean data_received = false;

//-----Variable related to motor timing and firing
bool is_shooting =  false;
bool can_shoot =  false;
bool reloading = false;

unsigned long shoot_start_time = 0;
unsigned long shoot_current_time = 0;
const long shoot_time = 150;

unsigned long reload_start_time = 0;
unsigned long reload_current_time = 0;
const long reload_time = 2 * shoot_time;

const byte motor_pin =  12;
boolean motors_ON = false;

// Buzzer and distance snsor pins
const int trigPin = 7;
const int echoPin = 6;
const int buzzerPin = 4;

//-----Variable related to distance sensor
long duration;
int distance;
boolean is_too_close = false;
static long previousMillis = 0;
static long currentMillis = 0;
static int previous_distance = 100;
const long interval = 3000;
const long distance_check_interval = 500; 
static long last_distance_check = 0;


void setup() {
  initializeMotor();
  initializeServos();
  initializeSequence();
  initializeDistanceSensor();
  Serial.begin(9600);
}

void loop() {
  getBluetooth();
  check_distance();
  start_motor();
  if (data_received) {
    move_servo();
    check_reload();
    start_motor();
  }
  shoot();
}

void initializeMotor() {
  pinMode(motor_pin, OUTPUT);
  digitalWrite(motor_pin, LOW);
}

void initializeServos() {
  recoil_servo.attach(9);
  pan_servo.attach(10);
  tilt_servo.attach(11);
}

void initializeSequence() {
  recoil_servo.write(reload_rest);
  pan_servo.write(90);
  delay(1000);
  tilt_servo.write(105);
}

void initializeDistanceSensor() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  delay(2000);
}


void getBluetooth() {
  if (Serial.available()) {
    byte_from_app = Serial.read();

    if (byte_from_app == 255) {
      bytesRecvd = 0;
      data_received = false;
    } else if (byte_from_app == 254) {
      data_received = true;
    } else {
      recived_data
    [bytesRecvd++] = byte_from_app;
      bytesRecvd = min(bytesRecvd, buffSize - 1);
    }
  }
}

void move_servo() {
  pan_servo.write(map(recived_data[0], 0, 253, pan_limit_2, pan_limit_1));
  tilt_servo.write(map(recived_data[1], 0, 253, tilt_limit_2, tilt_limit_1));
}

void check_reload() {
  can_shoot = (recived_data[3] == 1 || is_too_close) && (!is_shooting && !reloading);
}

void start_motor() {
  motors_ON = (recived_data[2] == 1 || is_too_close);
  digitalWrite(motor_pin, motors_ON ? HIGH : LOW);
}

void shoot() {
  if (can_shoot && !is_shooting && motors_ON) {
    shoot_start_time = reload_start_time = millis();
    is_shooting = true;
  }

  shoot_current_time = reload_current_time = millis();

  if (is_shooting) {
    if (shoot_current_time - shoot_start_time < shoot_time) {
      recoil_servo.write(recoil_pushed);
    } else if (reload_current_time - reload_start_time < reload_time) {
      recoil_servo.write(reload_rest);
    } else {
      is_shooting = false;
    }
  }
}

void check_distance() {
  if (millis() - last_distance_check >= distance_check_interval) {
    last_distance_check = millis();
    measureDistance();

    if (previous_distance > 30 && distance <= 30) {
      previousMillis = millis();
    }

    if (distance <= 30) {
      handleCloseDistance();
    } else {
      handleNormalDistance();
    }

    previous_distance = distance;
  }
}

void measureDistance() {
  digitalWrite(trigPin, LOW);
  digitalWrite(trigPin, HIGH);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  currentMillis = millis();
}

void handleCloseDistance() {
  digitalWrite(buzzerPin, HIGH);
  if (currentMillis - previousMillis >= interval) {
    is_too_close = true;
    can_shoot = true;
    digitalWrite(motor_pin, HIGH);
    motors_ON = true;
    data_received = true;
  }
}

void handleNormalDistance() {
  digitalWrite(buzzerPin, LOW);
  is_too_close = false;
  data_received = false;
}