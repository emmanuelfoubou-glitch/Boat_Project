#include <FlexiTimer2.h>
#include <digitalWriteFast.h>
#include <Servo.h>  // <--- Ajout pour le servo
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <QMC5883LCompass.h>
#include "data_provider.h"

//DC MOTOR DRIVER
int motorMoins = 6;
int motorPlus = 8;
int enablePin = 5;

Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
QMC5883LCompass compass;

float angleZ;
// === Pins ===
const int pinPotentiometre = A2;

float Kpp = 1.0;
// POTENTIOMETRES
#define POT_VITESSE A0   // Potentiomètre pour vitesse moteur DC
//#define POT_SERVO   A2   // Potentiomètre pour position servo

// SERVO
//Servo monServo;
//#define SERVO_PIN 9

// TIMER
#define CADENCE_MS 10
volatile double dt = CADENCE_MS / 1000.;
volatile double temps = -CADENCE_MS / 1000.;

// ENCODEUR
#define codeurInterruptionA 0
#define codeurInterruptionB 1
#define codeurPinA 2
#define codeurPinB 3
volatile long tick_codeur = 0;

// VARIABLES
volatile double omega;
volatile double theta_precedent = 0;
volatile double theta;
volatile double commande = 0.;

volatile double vref = 30;

// PID
volatile double Kp = 4.0;
volatile double P_x = 0.;
volatile double Ki = 0.0;
volatile double Kd = 0.0;
volatile double integral = 0.;
volatile double derivative = 0.;
volatile double previous_error = 0.;
volatile double ecart = 0.;

void setup() {
  pinMode(motorMoins, OUTPUT);
  pinMode(motorPlus, OUTPUT);
  pinMode(enablePin, OUTPUT);

  pinMode(codeurPinA, INPUT);
  pinMode(codeurPinB, INPUT);
  digitalWrite(codeurPinA, HIGH);
  digitalWrite(codeurPinB, HIGH);
  attachInterrupt(0, updateEncoderA, CHANGE);
  attachInterrupt(1, updateEncoderB, CHANGE);

  Serial.begin(9600);
  Wire.begin(); // Initialize I2C for both LCD and compass
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initialisation...");
  
  delay(1000);
  lcd.clear();
  lcd.print("Pret a fonctionner");
  delay(1000);
  compass.init();

  servo.attach(9);

  // Initialize shared telemetry values
  public_goodAngle = 0;
  public_targetAngle = 0;
  // gpsLat/gpsLon/motorTemp can be set here if you have sensors
  gpsLat = 0.0;
  gpsLon = 0.0;
  motorTemp = 0.0;

  // Timer
  tick_codeur = 0;
  FlexiTimer2::set(CADENCE_MS, 1 / 1000., calculs);
  FlexiTimer2::start();
}

void loop() {
  // Lire le potentiomètre pour la vitesse du moteur DC (vref)
  int potValue = analogRead(POT_VITESSE); // [0, 1023]
  vref = (potValue / 1023.0) * 30.0;       // Remap à [0, 10 rad/s]

  int x, y, z;
   compass.read();

  x = compass.getX();
  y = compass.getY();
  z = compass.getZ();


  int potValueReference = analogRead(pinPotentiometre);


  angleZ = -(atan2(y, x) * 180 / PI);

  float targetAngle = map(potValueReference, 0, 1023, 0, 180);

  float error = targetAngle - angleZ;
  error = fmod((error + 360), 360);

  float correctionKp = Kpp * error;
  float goodAngle = angleZ + correctionKp;

  int servoAngle = map(error, 0, 360, 0, 180);
  servo.write(servoAngle);

  

  lcd.setCursor(0, 0);
  lcd.print("Target: ");
  lcd.print(targetAngle);

  lcd.setCursor(0, 1);
  lcd.print("Heading: ");
  lcd.print(goodAngle); // Display raw compass for debugging
  
  delay(100);
  // Lire le potentiomètre pour le servo
 // int potServo = analogRead(POT_SERVO);   // [0, 1023]
 // int angle = map(potServo, 0, 1023, 0, 180);
 // monServo.write(angle);

  Serial.println(String(omega) + "  " + String(temps) + "  " + String(commande) + "  vref:" + String(vref) + "  servo:" + String(goodAngle) + "target:" + String(targetAngle));

  // Update shared telemetry so the web server can read current values
  public_goodAngle = goodAngle;
  public_targetAngle = targetAngle;
  // motorTemp and GPS values should be updated here when sensors are added
  // gpsLat = ...; gpsLon = ...; motorTemp = ...;
}

void commandeMoteur(double commande) {
  digitalWrite(motorPlus, HIGH);
  digitalWrite(motorMoins, LOW);
  if (commande > 255) commande = 255;
  else if (commande < 0) commande = 0;
  analogWrite(enablePin, commande);
}

void calculs() {
  int codeurDeltaPos = tick_codeur;
  tick_codeur = 0;

  omega = ((2. * 3.141592 * ((double)codeurDeltaPos)) / 828.) / dt;
  theta = theta_precedent + omega * dt;
  theta_precedent = theta;

  ecart = vref - omega;
  P_x = Kp * ecart;
  integral = Ki * ecart * dt;
  derivative = Kd * (ecart - previous_error) / dt;

  commande = P_x + integral + derivative;
  commande = commande * 255 / 10;

  commandeMoteur(commande);

  temps += dt;
  previous_error = ecart;

  // copy volatile values into shared variables for the web JSON provider
  ::omega = omega;
  ::temps = temps;
  ::commande = commande;
  ::vref = vref;
}

void updateEncoderA() {
  if (digitalReadFast2(codeurPinA) == digitalReadFast2(codeurPinB)) tick_codeur--;
  else tick_codeur++;
}

void updateEncoderB() {
  if (digitalReadFast2(codeurPinA) == digitalReadFast2(codeurPinB)) tick_codeur++;
  else tick_codeur++;
}