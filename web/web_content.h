// web_content.h
// Contient le HTML/CSS embarqué pour le serveur (généré à partir de web/)
#ifndef WEB_CONTENT_H
#define WEB_CONTENT_H

static const char login_page[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Connexion - Dashboard bateau</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="login-card">
    <h2>Connexion</h2>
    <form id="loginForm">
      <label>Nom d'utilisateur</label>
      <input type="text" id="user" required>
      <label>Mot de passe</label>
      <input type="password" id="pass" required>
      <button type="submit">Se connecter</button>
    </form>
    <div id="msg"></div>
  </div>
  <script>
    document.getElementById('loginForm').addEventListener('submit', function(e){
      e.preventDefault();
      const user = document.getElementById('user').value;
      const pass = document.getElementById('pass').value;
      // On utilise GET simple sur /login pour obtenir un token
      fetch('/login?user='+encodeURIComponent(user)+'&pass='+encodeURIComponent(pass))
        .then(r => r.text())
        .then(t => {
          // la réponse JavaScript/HTML va stocker le token et rediriger
          document.open();
          document.write(t);
          document.close();
        })
        .catch(err => { document.getElementById('msg').innerText = 'Erreur réseau'; });
    });
  </script>
</body>
</html>)rawliteral";

static const char dashboard_page[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Dashboard bateau</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="container">
    <header>
      <h1>Tableau de bord</h1>
      <div id="userArea"></div>
    </header>
    <!-- Status bar: serveur et capteurs -->
    <div class="status-bar">
      <span id="serverStatus" class="status red">Serveur: inconnu</span>
      <span id="gpsStatus" class="status red">GPS: inconnu</span>
      <span id="motorStatus" class="status red">Moteur: inconnu</span>
      <span id="compassStatus" class="status red">Boussole: inconnu</span>
    </div>
    <main>
      <section class="row">
        <div class="card">
          <h3>Consignes</h3>
          <label>Vitesse désirée (vref)</label>
          <input id="vrefInput" type="range" min="0" max="30" step="0.1">
          <div class="gauge" id="speedGauge"></div>

          <label>Cap désiré</label>
          <input id="headingInput" type="range" min="0" max="360" step="1">
          <canvas id="compassCanvas" width="200" height="200"></canvas>

          <button id="applyBtn">Appliquer</button>
        </div>

        <div class="card">
          <h3>Informations capteurs</h3>
          <pre id="telemetry">Chargement...</pre>
        </div>
      </section>

    </main>
  </div>

  <script>
    // Récupération du token depuis l'URL (écrit lors du login)
    function getTokenFromFragment(){
      const hash = location.hash.substring(1);
      if(!hash) return null;
      try{
        const params = new URLSearchParams(hash);
        return params.get('token');
      }catch(e){return null}
    }

    const token = getTokenFromFragment();
    if(!token){
      // pas de token : redirection vers login
      location.href = '/login.html';
    }

    document.getElementById('userArea').innerText = 'Connecté';

    const vrefInput = document.getElementById('vrefInput');
    const headingInput = document.getElementById('headingInput');
    const telemetry = document.getElementById('telemetry');
    const speedGauge = document.getElementById('speedGauge');
    const compass = document.getElementById('compassCanvas');
    const ctx = compass.getContext('2d');

  function drawCompass(angle){
      ctx.clearRect(0,0,200,200);
      ctx.save();
      ctx.translate(100,100);
      ctx.rotate((angle-90) * Math.PI/180);
      ctx.beginPath();
      ctx.moveTo(0,-70);
      ctx.lineTo(10,0);
      ctx.lineTo(-10,0);
      ctx.closePath();
      ctx.fillStyle = 'crimson';
      ctx.fill();
      ctx.restore();
    }

    const serverStatus = document.getElementById('serverStatus');
    const gpsStatus = document.getElementById('gpsStatus');
    const motorStatus = document.getElementById('motorStatus');
    const compassStatus = document.getElementById('compassStatus');

    function setOk(el, txt){ el.className = 'status green'; el.textContent = txt; }
    function setFail(el, txt){ el.className = 'status red'; el.textContent = txt; }

    function updateTelemetry(){
      fetch('/data')
        .then(r=>{
          if(!r.ok) throw new Error('HTTP '+r.status);
          return r.json();
        })
        .then(j=>{
          // serveur reachable
          setOk(serverStatus, 'Serveur: connecté');
          telemetry.textContent = JSON.stringify(j, null, 2);
          // update gauge (simple width)
          const pct = Math.min(1, j.vref / 30);
          speedGauge.style.width = Math.max(4, Math.round(180 * pct)) + 'px';
          drawCompass(j.targetAngle || 0);

          // GPS: considéré connecté si lat/lon != 0
          if (j.gps && (j.gps.lat != 0 || j.gps.lon != 0)) setOk(gpsStatus, 'GPS: connecté');
          else setFail(gpsStatus, 'GPS: déconnecté');

          // Moteur: connecté si omega ou commande non nuls
          if ((j.omega && Math.abs(j.omega) > 0.0001) || (j.commande && Math.abs(j.commande) > 0.0001)) setOk(motorStatus, 'Moteur: actif');
          else setFail(motorStatus, 'Moteur: inactif');

          // Boussole: connecté si targetAngle / goodAngle présents
          if ((j.targetAngle && Math.abs(j.targetAngle) >= 0) || (j.goodAngle && Math.abs(j.goodAngle) >= 0)) setOk(compassStatus, 'Boussole: ok');
          else setFail(compassStatus, 'Boussole: déconnectée');
        })
        .catch(e=>{
          // serveur non reachable
          setFail(serverStatus, 'Serveur: déconnecté');
          telemetry.textContent = 'Aucune donnée (serveur non accessible)';
          setFail(gpsStatus, 'GPS: inconnu');
          setFail(motorStatus, 'Moteur: inconnu');
          setFail(compassStatus, 'Boussole: inconnu');
        });
    }

    document.getElementById('applyBtn').addEventListener('click', ()=>{
      const v = vrefInput.value;
      const h = headingInput.value;
      const token = getTokenFromFragment();
      let url = '/set?vref='+encodeURIComponent(v)+'&heading='+encodeURIComponent(h)+'&enable=1';
      if(token) url += '&token='+encodeURIComponent(token);
      fetch(url)
        .then(r=>r.json())
        .then(j=>{ console.log('set', j); })
        .catch(e=>console.error(e));
    });

    // Refresh périodiquement
    updateTelemetry();
    setInterval(updateTelemetry, 2000);

  </script>
</body>
</html>)rawliteral";

static const char style_css[] PROGMEM = R"rawliteral(body{font-family:Arial,Helvetica,sans-serif;background:#eef2f5;margin:0;padding:0}
.container{max-width:1000px;margin:20px auto;padding:10px}
header{display:flex;justify-content:space-between;align-items:center}
.card{background:#fff;border-radius:8px;padding:12px;box-shadow:0 2px 6px rgba(0,0,0,0.08);margin:8px}
.row{display:flex;gap:12px}
.gauge{height:20px;background:#ddd;border-radius:10px;width:40px;margin:8px 0}
.login-card{max-width:320px;margin:80px auto;padding:20px;background:#fff;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.12)}
input[type=range]{width:100%}
button{background:#0066cc;color:#fff;padding:8px 12px;border:none;border-radius:4px;cursor:pointer}
pre{white-space:pre-wrap}

.status-bar{display:flex;gap:8px;padding:8px}
.status{padding:6px 10px;border-radius:6px;color:#fff;font-weight:600}
.status.green{background:#28a745}
.status.red{background:#dc3545}
)rawliteral";

static const char editor_page[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Editeur de valeurs - Backend table</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="container">
    <h1>Éditeur de valeurs (backend table)</h1>
    <p>Utilisez ce formulaire pour insérer manuellement les valeurs envoyées au dashboard.</p>
    <form id="tableForm">
      <label>Vitesse (vref)</label>
      <input id="vref" name="vref" type="number" step="0.01">

      <label>Cap (targetAngle)</label>
      <input id="targetAngle" name="targetAngle" type="number" step="0.1">

      <label>Omega</label>
      <input id="omega" name="omega" type="number" step="0.0001">

      <label>Temps</label>
      <input id="temps" name="temps" type="number" step="0.001">

      <label>Commande</label>
      <input id="commande" name="commande" type="number" step="0.01">

      <label>GPS lat</label>
      <input id="gpsLat" name="gpsLat" type="number" step="0.000001">
      <label>GPS lon</label>
      <input id="gpsLon" name="gpsLon" type="number" step="0.000001">

      <label>Motor Temp</label>
      <input id="motorTemp" name="motorTemp" type="number" step="0.1">

  <label><input id="useTable" name="use" type="checkbox"> Utiliser ces valeurs pour le dashboard</label>

      <button type="submit">Envoyer au serveur</button>
    </form>

    <div id="msg"></div>
  </div>

  <script>
    function getTokenFromFragment(){
      const hash = location.hash.substring(1);
      if(!hash) return null;
      try{ const params = new URLSearchParams(hash); return params.get('token'); }catch(e){return null}
    }
    const token = getTokenFromFragment();
    if(!token){ alert('Pas de token trouvé. Connectez-vous via /login.html'); }

    function loadTable(){
      fetch('/table')
        .then(r=>r.json())
        .then(j=>{
          document.getElementById('vref').value = j.vref || '';
          document.getElementById('targetAngle').value = j.targetAngle || '';
          document.getElementById('omega').value = j.omega || '';
          document.getElementById('temps').value = j.temps || '';
          document.getElementById('commande').value = j.commande || '';
          document.getElementById('gpsLat').value = j.gpsLat || '';
          document.getElementById('gpsLon').value = j.gpsLon || '';
          document.getElementById('motorTemp').value = j.motorTemp || '';
          // set the 'use' checkbox if provided
          try{ document.getElementById('useTable').checked = (j.use == 1 || j.use == true); }catch(e){}
        })
        .catch(e=>{ document.getElementById('msg').innerText = 'Impossible de charger la table: '+e; });
    }

    document.getElementById('tableForm').addEventListener('submit', function(e){
      e.preventDefault();
      const params = new URLSearchParams();
      ['vref','targetAngle','omega','temps','commande','gpsLat','gpsLon','motorTemp'].forEach(k=>{
        const v = document.getElementById(k).value;
        if(v !== '') params.append(k, v);
      });
      // include the use checkbox
      const useChecked = document.getElementById('useTable').checked;
      params.append('use', useChecked ? '1' : '0');
      if(token) params.append('token', token);
      fetch('/table/set?'+params.toString())
        .then(r=>r.json())
        .then(j=>{ document.getElementById('msg').innerText = 'Serveur: '+JSON.stringify(j); })
        .catch(e=>{ document.getElementById('msg').innerText = 'Erreur: '+e; });
    });

    loadTable();
  </script>
</body>
</html>)rawliteral";

#endif // WEB_CONTENT_H
