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

    function updateTelemetry(){
      fetch('/data')
        .then(r=>r.json())
        .then(j=>{
          telemetry.textContent = JSON.stringify(j, null, 2);
          // update gauge (simple width)
          const pct = Math.min(1, j.vref / 30);
          speedGauge.style.width = Math.max(4, Math.round(180 * pct)) + 'px';
          drawCompass(j.targetAngle || 0);
        })
        .catch(e=>{ telemetry.textContent = 'Erreur: '+e; });
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
)rawliteral";

#endif // WEB_CONTENT_H
