// Inclut la bibliothèque WiFiNINA pour gérer le WiFi sur cartes compatibles
#include <WiFiNINA.h>
// Inclut aWOT, un micro-framework pour créer des routes HTTP et répondre aux requêtes
#include <aWOT.h>
// Inclut notre fournisseur de données JSON (variables partagées et getDataJSON())
#include "data_provider.h"
// Contenu web embarqué (login/dashboard/style)
#include "web/web_content.h"

// includ spi .

// Nom du point d'accès (SSID) que la carte créera
char ssid[] = "Fasaboat_02";
// Mot de passe du point d'accès
char pass[] = "1234567890";

// Variable qui contient l'état actuel du module WiFi
int status = WL_IDLE_STATUS;

// Objet serveur TCP qui écoute sur le port 80 (HTTP)
WiFiServer server(80);
// Instance de l'application aWOT pour enregistrer des routes et traiter les requêtes
Application app;

// Handler pour la route racine "/" : reçoit la requête et remplit la réponse
void index(Request &req, Response &res) {
  // Log sur le port série pour debugging
  Serial.println("answer to index =)");
  // Écrit le corps de la réponse (ici un petit message HTML) ; aWOT
  // enverra ces données sur le client
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

// Handler pour l'endpoint "/data" : renvoie le JSON construit par getDataJSON()
void dataHandler(Request &req, Response &res) {
  // Indique dans le moniteur série qu'on sert /data
  Serial.println("answer to /data");
  // On écrit manuellement l'entête HTTP avec le Content-Type JSON afin de
  // rester compatible avec différentes versions de aWOT.
  res.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
  // Écrit le corps JSON récupéré depuis data_provider
  res.print(getDataJSON());
}

// Parse a query string like "vref=12.3&heading=45" and update remote setpoints
void setHandler(Request &req, Response &res) {
  Serial.println("answer to /set");
  String path = req.path();
  // path peut être "/set?vref=...&heading=..." ; chercher '?' et prendre la query
  int qpos = path.indexOf('?');
  bool updated = false;
  // token must be provided as a query param 'token' or request is unauthorized
  String providedToken = "";
  if (qpos >= 0) {
    String query = path.substring(qpos + 1);
    // Séparer les paires key=value
    int start = 0;
    while (start < query.length()) {
      int amp = query.indexOf('&', start);
      if (amp == -1) amp = query.length();
      String pair = query.substring(start, amp);
      int eq = pair.indexOf('=');
      if (eq > 0) {
        String key = pair.substring(0, eq);
        String val = pair.substring(eq + 1);
        // Convertir et appliquer
        if (key == "token") {
          providedToken = val;
        }
        if (key == "vref") {
          double v = val.toDouble();
          remote_vref = v;
          updated = true;
        } else if (key == "heading" || key == "cap") {
          float a = val.toFloat();
          remote_targetAngle = a;
          updated = true;
        } else if (key == "enable") {
          if (val == "1" || val == "true") remoteSetpointsEnabled = true;
          else remoteSetpointsEnabled = false;
        }
      }
      start = amp + 1;
    }
  }

  // Validate token
  if (remoteAuthToken.length() == 0 || providedToken != remoteAuthToken) {
    res.print("HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n");
    res.print("{\"error\":\"unauthorized\"}");
    return;
  }

  // Répondre avec l'état actuel
  res.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
  String out = "{";
  out += "\"remoteSetpointsEnabled\":" + String(remoteSetpointsEnabled ? 1 : 0) + ",";
  out += "\"remote_vref\":" + String(remote_vref, 4) + ",";
  out += "\"remote_targetAngle\":" + String(remote_targetAngle, 2);
  out += "}";
  res.print(out);
}

// Hardcoded credentials (change these before déploiement)
const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "boatpass";

// Simple login handler: expects ?user=...&pass=... and returns a small page
// that stores token in fragment and redirects to /dashboard
void loginHandler(Request &req, Response &res) {
  String path = req.path();
  int qpos = path.indexOf('?');
  // If no query string, serve the login page HTML
  if (qpos == -1) {
    // Serve the login page from web_content.h
    res.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    res.print(login_page);
    return;
  }

  // Otherwise process credentials from query string
  String user="", pass="";
  String query = path.substring(qpos+1);
  int start = 0;
  while (start < query.length()) {
    int amp = query.indexOf('&', start);
    if (amp == -1) amp = query.length();
    String pair = query.substring(start, amp);
    int eq = pair.indexOf('=');
    if (eq>0) {
      String k = pair.substring(0, eq);
      String v = pair.substring(eq+1);
      if (k=="user") user = v;
      if (k=="pass") pass = v;
    }
    start = amp+1;
  }

  bool ok = (user == String(ADMIN_USER) && pass == String(ADMIN_PASS));
  if (ok) {
    // generate token and return a page that redirects with token in fragment
    remoteAuthToken = generateToken();
    String body = "<html><body>";
    body += "<script>location.href='/dashboard.html#token=" + remoteAuthToken + "';</script>";
    body += "</body></html>";
    res.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    res.print(body);
  } else {
    res.print("HTTP/1.1 401 Unauthorized\r\nContent-Type: text/plain\r\n\r\n");
    res.print("Unauthorized");
  }
}

// Serve minimal style (you have a full style.css in /web but this is embedded)
void styleHandler(Request &req, Response &res) {
  // Inlined minimal CSS; for full design use the file in the web/ folder
  res.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
  res.print(style_css);
}

// Dashboard page (minimal embedded version). The richer static files are in /web
void dashboardHandler(Request &req, Response &res) {
  // Serve the dashboard page from web_content.h
  res.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  res.print(dashboard_page);
}

// Return the editable table as JSON
void tableHandler(Request &req, Response &res) {
  res.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
  String out = "{";
  out += "\"vref\":" + String(table_vref, 4) + ",";
  out += "\"targetAngle\":" + String(table_targetAngle, 2) + ",";
  out += "\"omega\":" + String(table_omega, 4) + ",";
  out += "\"temps\":" + String(table_temps, 4) + ",";
  out += "\"commande\":" + String(table_commande, 4) + ",";
  out += "\"gpsLat\":" + String(table_gpsLat, 6) + ",";
  out += "\"gpsLon\":" + String(table_gpsLon, 6) + ",";
  out += "\"motorTemp\":" + String(table_motorTemp, 2) + ",";
  out += "\"use\":" + String(useTableValues ? 1 : 0);
  out += "}";
  res.print(out);
}

// Set values in the editable table (protected by token)
void tableSetHandler(Request &req, Response &res) {
  String path = req.path();
  int qpos = path.indexOf('?');
  String providedToken = "";
  if (qpos >= 0) {
    String query = path.substring(qpos+1);
    int start = 0;
    while (start < query.length()) {
      int amp = query.indexOf('&', start);
      if (amp == -1) amp = query.length();
      String pair = query.substring(start, amp);
      int eq = pair.indexOf('=');
      if (eq>0) {
        String key = pair.substring(0, eq);
        String val = pair.substring(eq+1);
  if (key == "token") providedToken = val;
        else if (key == "vref") table_vref = val.toDouble();
        else if (key == "targetAngle") table_targetAngle = val.toFloat();
        else if (key == "omega") table_omega = val.toDouble();
        else if (key == "temps") table_temps = val.toDouble();
        else if (key == "commande") table_commande = val.toDouble();
        else if (key == "gpsLat") table_gpsLat = val.toDouble();
        else if (key == "gpsLon") table_gpsLon = val.toDouble();
        else if (key == "motorTemp") table_motorTemp = val.toFloat();
  else if (key == "use") useTableValues = (val == "1" || val == "true");
      }
      start = amp+1;
    }
  }
  if (remoteAuthToken.length() == 0 || providedToken != remoteAuthToken) {
    res.print("HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"unauthorized\"}");
    return;
  }
  // respond with updated table
  tableHandler(req, res);
}


// Fonction d'initialisation exécutée une fois au démarrage
void setup() {
  // Initialise la communication série (moniteur série)
  Serial.begin(9600);
  // Attendre que le port série soit prêt (utile sur certaines cartes)
  while (!Serial) {
    ; // attente active
  }

  // Message d'information dans le moniteur série
  Serial.println("Access Point Web Server");

  // Vérifie que le module WiFi est présent
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // Bloque ici car sans module WiFi le sketch ne peut pas continuer
    while (true);
  }

  // Vérifie la version du firmware du module WiFi
  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // Configure une IP locale fixe pour le point d'accès (ici 10.0.0.1)
  WiFi.config(IPAddress(10, 0, 0, 1));

  // Affiche le nom du point d'accès que l'on va créer
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Crée le point d'accès WiFi (mode AP) avec SSID et mot de passe
  status = WiFi.beginAP(ssid, pass);
  // Vérifie si la création a réussi (mode écoute)
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // Bloque si échec
    while (true);
  }

  // Petite attente pour laisser le temps aux clients de se connecter
  delay(10000);

  // Enregistre la route racine et l'endpoint /data
  app.get("/", &index);
  app.get("/data", &dataHandler);
  // Endpoint pour définir des consignes à distance (vref, heading)
  app.get("/set", &setHandler);
  // Login endpoint (authentification) et pages
  app.get("/login", &loginHandler);
  app.get("/login.html", &loginHandler);
  app.get("/dashboard.html", &dashboardHandler);
  app.get("/style.css", &styleHandler);
  app.get("/editor.html", [](Request &req, Response &res){ res.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"); res.print(editor_page); });
  app.get("/table", &tableHandler);
  app.get("/table/set", &tableSetHandler);

  // Route de secours pour les chemins non trouvés (404)
  // Attention : la signature et l'API exacte de aWOT peuvent varier selon
  // la version. Ici on utilise une lambda qui écrit une réponse 404 simple.
  app.notFound([](Request &req, Response &res){
    // Log du chemin non trouvé
    Serial.println("404 for: " + req.path());
    // Écrit manuellement l'entête 404 et un corps texte
    res.print("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n");
    res.print("Not Found");
  });

  // Démarre le serveur TCP
  server.begin();

  // Affiche les informations réseau (SSID, IP, etc.)
  printWiFiStatus();

}

// Boucle principale répétée en continu
void loop() {

  // Si l'état WiFi a changé, on en informe le moniteur série
  if (status != WiFi.status()) {
    // Met à jour la variable d'état
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // Un appareil s'est connecté au point d'accès
      Serial.println("Device connected to AP");
    } else {
      // Un appareil s'est déconnecté
      Serial.println("Device disconnected from AP");
    }
  }

  // Récupère un client TCP si quelqu'un a fait une requête
  WiFiClient client = server.available();

  // Si le client est bien connecté, on délègue le traitement à aWOT
  if (client.connected()) {
    // Log utile pour debug
    Serial.println("Serving connected client : "  + String(client));

    // aWOT va lire la requête, appeler le handler adapté et écrire la réponse
    app.process(&client);
  }
}


// Affiche des informations réseau utiles dans le moniteur série
void printWiFiStatus() {
  // Affiche le SSID du point d'accès
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Récupère et affiche l'adresse IP locale
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Indique à l'utilisateur quelle adresse ouvrir dans un navigateur
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}
