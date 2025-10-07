// data_provider.h
// Declares shared telemetry variables and the JSON provider function.
#ifndef DATA_PROVIDER_H
#define DATA_PROVIDER_H

#include <Arduino.h>

// Telemetry values provided by controle_cap_vitesse.ino
extern volatile double omega;
extern volatile double temps;
extern volatile double commande;
extern volatile double vref;

// Angles computed in the control sketch (made public)
extern float public_goodAngle;
extern float public_targetAngle;

// GPS and temperature placeholders (to be set by your GPS/temperature code)
extern double gpsLat;
extern double gpsLon;
extern float motorTemp;

// When true, the control sketch should use vref and public_targetAngle set
// remotely via the web server instead of local potentiometers.
extern volatile bool remoteSetpointsEnabled;

// Returns a JSON string with the current telemetry
String getDataJSON();

#endif // DATA_PROVIDER_H
