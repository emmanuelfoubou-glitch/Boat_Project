#include <WiFiNINA.h>
#include <aWOT.h>
#include "data_provider.h"

// includ spi .

char ssid[] = "Fasaboat_02";        // your network SSID (name)
char pass[] = "1234567890";    // your network password

int status = WL_IDLE_STATUS;

WiFiServer server(80);
Application app;

void index(Request &req, Response &res) {
  Serial.println("answer to index =)");
  res.print("Hello World!<br><h1>" + String(millis()));
}


// Returns a small JSON string with sample data. You can expand this to include
// real sensor readings or other telemetry.
String getDataJSON() {
  unsigned long t = millis();
  // Example sensor/sample value. Replace or remove analogRead if your board
  // doesn't support it or you don't need it.
  int sample = 0;
#ifdef A0
  sample = analogRead(A0);
#endif

  String json = "{";
  json += "\"time\":" + String(t) + ",";
  json += "\"sample\":" + String(sample);
  json += "}";
  return json;
}

// Handler for the /data endpoint: returns the JSON produced by getDataJSON().
void dataHandler(Request &req, Response &res) {
  Serial.println("answer to /data");
  // Send headers (some aWOT versions expect the body only, others need full
  // headers). Writing headers manually is the most compatible approach.
  res.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
  res.print(getDataJSON());
}


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");


  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // local IP address of will be 10.0.0.1
   WiFi.config(IPAddress(10, 0, 0, 1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);


  //start web server
  app.get("/", &index);
  // register /data endpoint
  app.get("/data", &dataHandler);
  // optional: fallback 404 route
  app.notFound([](Request &req, Response &res){
    Serial.println("404 for: " + req.path());
    res.print("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n");
    res.print("Not Found");
  });
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

}

void loop() {


  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }




  WiFiClient client = server.available();

  if (client.connected()) {
    Serial.println("Serving connected client : "  + String(client));

    app.process(&client);
  }
}


void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}
