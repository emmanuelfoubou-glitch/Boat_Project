// Bibliothèque pour minuterie flexible (callbacks périodiques)
#include <FlexiTimer2.h>
// Versions rapides de digitalWrite/digitalRead optimisées
#include <digitalWriteFast.h>
// Bibliothèque pour contrôler un servomoteur
#include <Servo.h>  // <--- Ajout pour le servo
// Bus I2C (utilisé par LCD et boussole)
#include <Wire.h>
// Écran LCD via I2C
#include <LiquidCrystal_I2C.h>
// Boussole QMC5883L
#include <QMC5883LCompass.h>
// Fichier fournisseur de données JSON (variables partagées)
#include "data_provider.h"

// --- Pilote moteur DC ---
// Broche pour sens négatif du moteur (IN1 par exemple)
int motorMoins = 6;
// Broche pour sens positif du moteur (IN2 par exemple)
int motorPlus = 8;
// Broche PWM pour réguler la vitesse (ENA)
int enablePin = 5;

// Instance du servo pour contrôler la gouverne
Servo servo;
// Instance de l'écran LCD (adresse 0x27, 16 colonnes, 2 lignes)
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Instance de la boussole QMC5883L
QMC5883LCompass compass;

// Angle mesuré autour de l'axe Z (heading) en degrés
float angleZ;
// === Broches ===
// Broche analogique utilisée pour lecture potentiomètre référence (cap)
const int pinPotentiometre = A2;

// Gain proportionnel pour correction cap
float Kpp = 1.0;
// Broche analogique pour régler la consigne de vitesse (vref)
#define POT_VITESSE A0   // Potentiomètre pour vitesse moteur DC
// #define POT_SERVO A2  // (désactivé) potentiomètre pour le servo

// --- Servo (si besoin d'une autre instance) ---
//Servo monServo; // exemple d'instance alternative
//#define SERVO_PIN 9

// --- Timer périodique ---
// Cadence d'exécution en millisecondes pour la fonction 'calculs'
#define CADENCE_MS 10
// Pas de temps en secondes (convertit ms -> s)
volatile double dt = CADENCE_MS / 1000.;
// 'temps' est partagé via data_provider (déclaré extern dans data_provider.h)

// --- Encodeur (capteur de position/rotation) ---
// Numéros d'interruptions (dépend de la carte, ex: 0 -> digital 2 sur UNO)
#define codeurInterruptionA 0
#define codeurInterruptionB 1
// Broches physiques liées à l'encodeur
#define codeurPinA 2
#define codeurPinB 3
// Compteur d'impulsions accumulées entre deux appels à calculs()
volatile long tick_codeur = 0;

// --- Variables dynamiques ---
// Les variables partagées 'omega', 'commande', 'vref' et 'temps' sont
// déclarées dans data_provider.h (extern) et définies dans data_provider.cpp.
// On conserve ici les variables locales liées au calcul d'angle.
// Angle précédent (pour intégration)
volatile double theta_precedent = 0;
// Angle courant
volatile double theta;

// --- Paramètres et variables du contrôleur PID ---
volatile double Kp = 4.0;          // gain proportionnel
volatile double P_x = 0.;          // terme proportionnel calculé
volatile double Ki = 0.0;          // gain intégral
volatile double Kd = 0.0;          // gain dérivé
volatile double integral = 0.;     // terme intégral
volatile double derivative = 0.;   // terme dérivé
volatile double previous_error = 0.;
volatile double ecart = 0.;        // erreur actuelle (vref - omega)

void setup() {
  // Configure les broches du pilote moteur en sortie
  pinMode(motorMoins, OUTPUT);
  pinMode(motorPlus, OUTPUT);
  pinMode(enablePin, OUTPUT);

  // Configure les broches de l'encodeur en entrée et active pull-up
  pinMode(codeurPinA, INPUT);
  pinMode(codeurPinB, INPUT);
  digitalWrite(codeurPinA, HIGH); // pull-up logiciel
  digitalWrite(codeurPinB, HIGH);
  // Attache les interruptions pour détecter les changements d'état
  attachInterrupt(0, updateEncoderA, CHANGE);
  attachInterrupt(1, updateEncoderB, CHANGE);

  // Initialise la liaison série pour le debug
  Serial.begin(9600);
  // Démarre le bus I2C (pour LCD et boussole)
  Wire.begin(); // Initialize I2C for both LCD and compass
  // Initialise l'écran LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initialisation...");
  
  delay(1000);
  lcd.clear();
  lcd.print("Pret a fonctionner");
  delay(1000);
  // Initialise la boussole QMC5883L
  compass.init();

  // Attache le servo sur la broche 9
  servo.attach(9);

  // Initialise les variables partagées (utilisées par le serveur web)
  public_goodAngle = 0;
  public_targetAngle = 0;
  // Valeurs GPS / température initiales (à remplacer si vous avez des capteurs)
  gpsLat = 0.0;
  gpsLon = 0.0;
  motorTemp = 0.0;

  // Initialise le compteur d'encodeur et démarre la minuterie FlexiTimer2
  tick_codeur = 0;
  // Définit un rappel périodique toutes les CADENCE_MS millisecondes
  FlexiTimer2::set(CADENCE_MS, 1 / 1000., calculs);
  FlexiTimer2::start();
}

void loop() {
  // Lecture du potentiomètre pour la consigne de vitesse
  int potValue = analogRead(POT_VITESSE); // plage [0, 1023]
  // Remise à l'échelle : ici on mappe 0..1023 -> 0..30 (valeur arbitraire)
  // Si la commande distante est activée, utiliser remote_vref à la place
  if (remoteSetpointsEnabled) {
    vref = remote_vref;
  } else {
    vref = (potValue / 1023.0) * 30.0;       // Remap à [0, 30]
  }

  // Lecture de la boussole (met à jour les registres internes)
  int x, y, z;
  compass.read();

  // Récupère les composantes x,y,z de la boussole
  x = compass.getX();
  y = compass.getY();
  z = compass.getZ();

  // Lecture du potentiomètre de référence (cap ou direction souhaitée)
  int potValueReference = analogRead(pinPotentiometre);

  // Calcul de l'angle autour de Z (heading) en degrés
  angleZ = -(atan2(y, x) * 180 / PI);

  // Convertit la lecture du potentiomètre de cap en angle cible [0..180]
  float targetAngle = map(potValueReference, 0, 1023, 0, 180);
  // Si la commande distante est activée, on prend la valeur envoyée par le serveur
  if (remoteSetpointsEnabled) {
    targetAngle = remote_targetAngle;
  }

  // Calcul de l'erreur entre angle cible et angle mesuré
  float error = targetAngle - angleZ;
  // Normalisation sur 0..360 (évite sauts brusques)
  error = fmod((error + 360), 360);

  // Application d'un gain proportionnel sur l'erreur de cap
  float correctionKp = Kpp * error;
  // Angle corrigé affiché (pour debug)
  float goodAngle = angleZ + correctionKp;

  // Calcul d'un angle servo à partir de l'erreur (exemple simple)
  int servoAngle = map(error, 0, 360, 0, 180);
  servo.write(servoAngle);

  // Affichage sur l'écran LCD : cible et cap actuel
  lcd.setCursor(0, 0);
  lcd.print("Target: ");
  lcd.print(targetAngle);

  lcd.setCursor(0, 1);
  lcd.print("Heading: ");
  lcd.print(goodAngle); // Affiche le cap corrigé pour debug
  
  delay(100);
  // Lecture potentiometre servo (si utilisé)
  // int potServo = analogRead(POT_SERVO);   // [0, 1023]
  // int angle = map(potServo, 0, 1023, 0, 180);
  // monServo.write(angle);

  // Log série avec valeurs importantes (omega, temps, commande...)
  Serial.println(String(omega) + "  " + String(temps) + "  " + String(commande) + "  vref:" + String(vref) + "  servo:" + String(goodAngle) + "target:" + String(targetAngle));

  // Met à jour les variables partagées pour que le serveur HTTP puisse les lire
  public_goodAngle = goodAngle;
  public_targetAngle = targetAngle;
  // motorTemp, gpsLat et gpsLon doivent être mis à jour ici si vous ajoutez
  // des capteurs physiques (ex: capteur de température ou module GPS)
}

// Envoie la commande PWM au moteur (sens fixe ici: positive)
void commandeMoteur(double commande) {
  // Définit le sens du moteur (ici sens positif)
  digitalWrite(motorPlus, HIGH);
  digitalWrite(motorMoins, LOW);
  // Contrainte de la commande entre 0 et 255 (valeur PWM)
  if (commande > 255) commande = 255;
  else if (commande < 0) commande = 0;
  // Écrit la valeur PWM sur la broche enable
  analogWrite(enablePin, commande);
}

// Fonction appelée périodiquement (par FlexiTimer2) pour calculs du contrôle
void calculs() {
  // Lit et remet à zéro le nombre d'impulsions comptées depuis le dernier
  // appel (delta de position de l'encodeur)
  int codeurDeltaPos = tick_codeur;
  tick_codeur = 0;

  // Convertit delta d'impulsions en vitesse angulaire (rad/s)
  // Constante 828. correspond au nombre d'impulsions par tour (exemple)
  omega = ((2. * 3.141592 * ((double)codeurDeltaPos)) / 828.) / dt;
  // Intégration pour obtenir l'angle (theta)
  theta = theta_precedent + omega * dt;
  theta_precedent = theta;

  // Calcul de l'erreur entre consigne et mesure
  ecart = vref - omega;
  P_x = Kp * ecart;
  integral = Ki * ecart * dt;
  derivative = Kd * (ecart - previous_error) / dt;

  // Somme des termes PID
  commande = P_x + integral + derivative;
  // Mise à l'échelle de la commande vers l'échelle PWM (0..255)
  commande = commande * 255 / 10;

  // Envoie la commande au moteur
  commandeMoteur(commande);

  // Avance le temps interne
  temps += dt;
  previous_error = ecart;

  // Copie des valeurs volatiles dans les variables partagées
  // Les :: forcent la recherche dans l'espace global (évite conflits)
  ::omega = omega;
  ::temps = temps;
  ::commande = commande;
  ::vref = vref;
}

// Interruption appelée quand la voie A de l'encodeur change
void updateEncoderA() {
  // Quadrature decoding basique : compare A et B pour déterminer le sens
  if (digitalReadFast2(codeurPinA) == digitalReadFast2(codeurPinB)) tick_codeur--;
  else tick_codeur++;
}

// Interruption appelée quand la voie B de l'encodeur change
void updateEncoderB() {
  if (digitalReadFast2(codeurPinA) == digitalReadFast2(codeurPinB)) tick_codeur++;
  else tick_codeur++;
}