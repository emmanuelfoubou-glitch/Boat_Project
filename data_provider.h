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
// Simple auth token (store generated token here)
extern String remoteAuthToken;

// Simple editable table stored on the device (backend values editable via /table/set)
extern volatile double table_vref;
extern volatile float table_targetAngle;
extern volatile double table_omega;
extern volatile double table_temps;
extern volatile double table_commande;
extern volatile double table_gpsLat;
extern volatile double table_gpsLon;
extern volatile float table_motorTemp;
// When true, /data will return the editable table values instead of live sensors
extern volatile bool useTableValues;

// Returns a JSON string with the current telemetry
String getDataJSON();

#endif // DATA_PROVIDER_H
