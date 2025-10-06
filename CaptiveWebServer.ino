#include <WiFiNINA.h>
#include <aWOT.h>

// includ spi .

char ssid[] = "Group07";        // your network SSID (name)
char pass[] = "1234567890";    // your network password

int status = WL_IDLE_STATUS;

WiFiServer server(80);
Application app;

void index(Request &req, Response &res) {
  Serial.println("answer to index =)");
  res.print("Hello World!<br><h1>" + String(millis()));
// Bibliothèque pour gérer le module Wi‑Fi (fonctions de connexion, AP, clients, etc.)
#include <WiFiNINA.h>
// Bibliothèque aWOT pour créer un serveur HTTP léger et définir des routes (Request/Response)
#include <aWOT.h>

// Commentaire: si vous utilisez SPI explicitement, vous pouvez inclure <SPI.h> ici.
// includ spi .

// Nom du réseau Wi‑Fi (SSID) que l'appareil va créer en mode point d'accès (AP)
char ssid[] = "Group07";        // your network SSID (name)
// Mot de passe du point d'accès (attention: en clair dans le code)
char pass[] = "1234567890";    // your network password

// Variable pour stocker l'état courant du module Wi‑Fi (initialisée à l'état idle)
int status = WL_IDLE_STATUS;

// Crée un serveur TCP qui écoutera sur le port 80 (HTTP)
WiFiServer server(80);
// Instance de l'application aWOT qui gère les routes et les réponses HTTP
Application app;

// Handler pour la route racine "/" : reçoit la requête et remplit la réponse
void index(Request &req, Response &res) {
  // Affiche un message sur le moniteur série pour le debug quand / est demandé
  Serial.println("answer to index =)");
  // Écrit dans la réponse HTTP la chaîne "Hello World!" et le temps écoulé (millis)
  res.print("Hello World!<br><h1>" + String(millis()));
}


void setup() {
  // Initialise la communication série à 9600 bauds pour debug
  Serial.begin(9600);
  // Sur certaines cartes USB natives, il faut attendre la connexion du moniteur série
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Message indiquant le démarrage du serveur AP
  Serial.println("Access Point Web Server");


  // Vérifie que le module Wi‑Fi est présent et communicant
  if (WiFi.status() == WL_NO_MODULE) {
    // Si aucun module détecté, affiche l'erreur
    Serial.println("Communication with WiFi module failed!");
    // Arrête le programme ici (boucle infinie) car sans module le réseau ne fonctionnera pas
    while (true);
  }

  // Récupère la version du firmware du module Wi‑Fi
  String fv = WiFi.firmwareVersion();
  // Si la version est inférieure à 1.0.0, prévient que la mise à jour est recommandée
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // Configure une adresse IP locale fixe pour l'interface (ici 10.0.0.1)
  // Utile en mode AP pour que les clients puissent accéder à cette IP
   WiFi.config(IPAddress(10, 0, 0, 1));

  // Affiche le SSID que l'on va créer
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Démarre le point d'accès (AP) avec le SSID et le mot de passe fournis
  // Renvoie un statut qu'on stocke dans la variable `status`
  status = WiFi.beginAP(ssid, pass);
  // Vérifie que l'AP a bien démarré et est en écoute
  if (status != WL_AP_LISTENING) {
    // Si échec, affiche un message et bloque l'exécution
    Serial.println("Creating access point failed");
    while (true);
  }

  // Attente 10 secondes pour laisser le temps aux clients de se connecter
  delay(10000);


  // Enregistre le handler `index` pour la route GET "/"
  app.get("/", &index);
  // Démarre le serveur TCP pour accepter les connexions HTTP
  server.begin();

  // Affiche des informations sur le Wi‑Fi (SSID, IP, URL à ouvrir)
  printWiFiStatus();

}

void loop() {


  // Compare l'état Wi‑Fi précédent (stocké dans `status`) avec l'état actuel
  if (status != WiFi.status()) {
    // Si changement, met à jour `status`
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // Un appareil s'est connecté au point d'accès
      Serial.println("Device connected to AP");
    } else {
      // Un appareil s'est déconnecté (ou autre état), on est en écoute
      Serial.println("Device disconnected from AP");
    }
  }



  // Vérifie s'il y a un client TCP disponible (connexion entrante)
  WiFiClient client = server.available();

  // Si le client est connecté, on le sert
  if (client.connected()) {
    // Log: sert un client connecté (String(client) n'affiche pas forcément l'IP lisible)
    Serial.println("Serving connected client : "  + String(client));

    // Passe le client à aWOT pour qu'il traite la requête HTTP et envoie la réponse
    app.process(&client);
  }
}


// Fonction utilitaire qui affiche des informations réseau sur le port série
void printWiFiStatus() {
  // Affiche le SSID du réseau (AP ou réseau connecté)
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Récupère et affiche l'adresse IP locale
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Affiche l'URL à ouvrir dans un navigateur (http://<ip>)
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}
