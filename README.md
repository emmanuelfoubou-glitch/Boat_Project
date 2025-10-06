# Boat_Project
1. création serveur (explication du code"CaptiveWenServer")
    Je vais d’abord donner un résumé bref, puis expliquer le flux d’exécution et les éléments importants (comportement, dépendances, limites et conseils de test).

## Résumé rapide
Ce sketch transforme la carte en point d’accès Wi‑Fi (AP), lance un petit serveur HTTP et sert la route racine "/" qui renvoie une page simple "Hello World!" contenant le temps écoulé (millis). Il utilise la bibliothèque WiFiNINA pour le Wi‑Fi et aWOT pour gérer la route HTTP.

## Comportement pas à pas
a. Initialisation (setup)
   - Initialise le port série pour le debug (Serial.begin).
   - Vérifie la présence du module Wi‑Fi (WiFi.status() vs WL_NO_MODULE). Si absent, le programme se bloque.
   - Récupère la version du firmware et affiche un message si elle est < "1.0.0".
   - Configure une adresse IP locale fixe (10.0.0.1) pour l'interface.
   - Démarre un point d’accès Wi‑Fi avec le SSID et le mot de passe définis (WiFi.beginAP).
   - Si la création de l’AP échoue, le programme se bloque.
   - Attend 10 secondes (delay) pour laisser le temps aux clients de se connecter.
   - Enregistre le handler de route "/" avec aWOT (app.get("/", &index)) et démarre le serveur TCP (server.begin()).
   - Affiche le SSID, l’adresse IP et l’URL à ouvrir via printWiFiStatus().

b. Handler de route (`index`)
   - Quand la route "/" est demandée, affiche un message sur le moniteur série.
   - Envoie au client une réponse contenant "Hello World!" et la valeur de millis() (temps depuis démarrage).

c. Boucle principale (loop)
   - Surveille les changements d’état Wi‑Fi (compare `status` et `WiFi.status()`).
     - Si un appareil se connecte au point d’accès, affiche "Device connected to AP".
     - Si un appareil se déconnecte, affiche "Device disconnected from AP".
   - Vérifie si un client TCP est disponible (server.available()).
     - Si le client est connecté, envoie le client à aWOT via `app.process(&client)` qui lit la requête et envoie la réponse HTTP (invoke `index` si c’est "/").

d. Utilitaires
   - `printWiFiStatus()` affiche SSID, adresse IP et l’URL à ouvrir dans le navigateur.

## Dépendances et matériel requis
- Bibliothèques : WiFiNINA (obligatoire), aWOT (pour routage HTTP).
- Matériel : un module/une carte compatible WiFiNINA (par ex. MKR WiFi 1010, Nano 33 IoT, ou shield Wi‑Fi NINA). Une Arduino Uno Rev2 seule n’a pas de Wi‑Fi intégré, donc sur matériel strictement Uno le sketch ne fonctionnera pas sans module Wi‑Fi.

## Comportements à connaître / limites
- AP mode : le code crée un point d’accès, donc les clients doivent se connecter au SSID défini pour accéder au serveur.
- Bloquant en cas d’erreur : en cas d’absence de module ou d’échec de création d’AP, le sketch entre dans une boucle infinie (`while(true);`) — pratique pour debug mais peu robuste pour production.
- delay(10000) : bloque la MCU 10 s ; empêche tout travail pendant cette période.
- Mot de passe en clair dans le code — pas sécurisé.
- Comparaison de version par chaîne : `if (fv < "1.0.0")` est une comparaison lexicographique de chaînes, suffisante pour des versions simples mais fragile pour des formats complexes (ex : "1.10.0").
- Log `String(client)` : peut ne pas donner une information lisible (IP) — il est préférable d’utiliser des méthodes dédiées si disponibles (remoteIP()).
- aWOT : si cette librairie n’est pas disponible sur l’environnement (ex. certains simulateurs), le routage ne fonctionnera ; on peut remplacer par un serveur HTTP manuel (lecture des headers et écriture de la réponse).

## Comment tester rapidement
- Sur matériel réel : téléverser sur une carte qui possède Wi‑FiNINA (MKR WiFi 1010 / Nano 33 IoT). Ouvrir le moniteur série pour lire les messages et l’IP. Se connecter au SSID créé depuis un PC/mobile et ouvrir http://10.0.0.1/.
- Sur simulateur (Wokwi) : préférer une carte MKR WiFi 1010 ou Nano 33 IoT; si aWOT n’est pas disponible, utiliser une version simplifiée (je peux fournir le code) qui n’utilise que WiFiNINA et WiFiServer.



