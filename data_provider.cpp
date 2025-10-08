// data_provider.cpp
#include "data_provider.h"

// Define defaults in case controle_cap_vitesse doesn't set them yet
volatile double omega = 0;
volatile double temps = 0;
volatile double commande = 0;
volatile double vref = 0;

float public_goodAngle = 0;
float public_targetAngle = 0;

double gpsLat = 0.0;
double gpsLon = 0.0;
float motorTemp = 0.0;

// Remote setpoints and control flag
volatile double remote_vref = 0.0;
volatile float remote_targetAngle = 0.0;
volatile bool remoteSetpointsEnabled = false;

// Helper functions (optional) to set remote setpoints from other translation units
void setRemoteVref(double v) {
  remote_vref = v;
}
void setRemoteTargetAngle(float a) {
  remote_targetAngle = a;
}

// Auth token simple storage
String remoteAuthToken = "";

// Very small token generator (not cryptographically secure)
String generateToken() {
  unsigned long r = micros() ^ (unsigned long)remote_vref;
  return String(r, HEX);
}

// Editable backend table (defaults)
volatile double table_vref = 0;
volatile float table_targetAngle = 0;
volatile double table_omega = 0;
volatile double table_temps = 0;
volatile double table_commande = 0;
volatile double table_gpsLat = 0;
volatile double table_gpsLon = 0;
volatile float table_motorTemp = 0;

volatile bool useTableValues = false;

String getDataJSON() {
  // Build a compact JSON string. Keep memory usage small by using String
  // concatenation carefully.
  String s = "{";
  if (useTableValues) {
    s += "\"time\":" + String((unsigned long)table_temps) + ",";
    s += "\"omega\":" + String(table_omega, 4) + ",";
    s += "\"commande\":" + String(table_commande, 4) + ",";
    s += "\"vref\":" + String(table_vref, 4) + ",";
    s += "\"goodAngle\":" + String(table_targetAngle, 2) + ",";
    s += "\"targetAngle\":" + String(table_targetAngle, 2) + ",";
    s += "\"gps\":{\"lat\":" + String(table_gpsLat, 6) + ",\"lon\":" + String(table_gpsLon, 6) + "},";
    s += "\"motorTemp\":" + String(table_motorTemp, 2);
    s += "}";
    return s;
  }

  s += "\"time\":" + String((unsigned long)temps) + ",";
  s += "\"omega\":" + String(omega, 4) + ",";
  s += "\"commande\":" + String(commande, 4) + ",";
  s += "\"vref\":" + String(vref, 4) + ",";
  s += "\"goodAngle\":" + String(public_goodAngle, 2) + ",";
  s += "\"targetAngle\":" + String(public_targetAngle, 2) + ",";
  s += "\"gps\":{\"lat\":" + String(gpsLat, 6) + ",\"lon\":" + String(gpsLon, 6) + "},";
  s += "\"motorTemp\":" + String(motorTemp, 2);
  s += "}";
  return s;
}
