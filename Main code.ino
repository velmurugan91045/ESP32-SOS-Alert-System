#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

// ===== EDIT THESE =====
const char* SCHOOL_NAME = "xxx";
const char* TEAM_NAME   = "xxx";
const char* AP_SSID     = "LIFELINE_SOS";
const char* RESCUE_PIN  = "1234";   // CHANGE

#define BUZZER_PIN 25
#define LED_PIN    2
#define BUTTON_PIN 13
#define MAX_REPORTS 30

struct Report {
  bool used;
  bool isSOS;
  bool handled;
  bool cancelled;
  int  id;
  int  prio;
  unsigned long token;
  unsigned long t;
  String people, ages, gender, need, location, phone, notes;
};

Report reports[MAX_REPORTS];
int  nextSlot = 0;
int  nextId = 1;
int  totalReceived = 0;
bool muted = false;
unsigned long lastInputAt = 0;

uint32_t lastSosIp = 0;
unsigned long lastSosAt = 0;
int lastSosId = 0;

DNSServer dnsServer;
WebServer server(80);
IPAddress apIP(192, 168, 4, 1);

const char* STYLE =
  "*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}"
  "body{margin:0;min-height:100vh;font-family:-apple-system,Segoe UI,Roboto,Arial,sans-serif;color:#eaf0ff;"
  "background:#070b16;background-image:radial-gradient(600px 420px at 90% -10%,rgba(255,59,59,.28),transparent),"
  "radial-gradient(520px 420px at -10% 105%,rgba(47,128,255,.24),transparent);background-attachment:fixed}"
  ".wrap{max-width:480px;margin:0 auto;padding:18px 16px 30px}"
  ".hd{text-align:center;padding:8px 0 4px}"
  ".sc{font-size:11px;letter-spacing:2.5px;text-transform:uppercase;color:#7f93bd;font-weight:600}"
  ".lg{display:flex;align-items:center;justify-content:center;gap:10px;margin:8px 0 10px}"
  ".lg svg{width:40px;height:40px}"
  ".lg b{font-size:34px;letter-spacing:3px;font-weight:800}"
  ".lg i{font-style:normal;color:#ff4d4d}"
  ".tm{display:inline-block;font-size:12px;color:#a9bbe3;background:rgba(255,255,255,.06);"
  "border:1px solid rgba(255,255,255,.1);padding:5px 14px;border-radius:99px}"
  ".hint{text-align:center;color:#8fa2c9;font-size:14px;margin:30px 0 0}"
  ".sosw{display:flex;justify-content:center;margin:26px 0 34px}"
  ".sosb{position:relative;width:210px;height:210px;border-radius:50%;display:flex;flex-direction:column;"
  "align-items:center;justify-content:center;text-decoration:none;color:#fff;"
  "background:radial-gradient(circle at 35% 30%,#ff8a8a,#e11d1d 55%,#a10000);"
  "box-shadow:0 14px 44px rgba(225,29,29,.55),inset 0 -8px 20px rgba(0,0,0,.25)}"
  ".sosb b{font-size:58px;letter-spacing:4px;line-height:1}"
  ".sosb span{font-size:12px;letter-spacing:1px;margin-top:8px;opacity:.9}"
  ".sosb:before,.sosb:after{content:'';position:absolute;left:0;top:0;right:0;bottom:0;border-radius:50%;"
  "border:2px solid rgba(255,77,77,.65);animation:rp 2.4s infinite}"
  ".sosb:after{animation-delay:1.2s}"
  "@keyframes rp{0%{transform:scale(1);opacity:.8}100%{transform:scale(1.65);opacity:0}}"
  ".opt{display:flex;align-items:center;gap:14px;padding:15px;margin:10px 0;border-radius:18px;"
  "text-decoration:none;color:#eaf0ff;background:rgba(255,255,255,.05);border:1px solid rgba(255,255,255,.09)}"
  ".opt .ic{width:46px;height:46px;border-radius:14px;display:flex;align-items:center;justify-content:center;"
  "font-size:22px;flex:none}"
  ".opt b{display:block;font-size:16px}"
  ".opt small{color:#8fa2c9;font-size:12px}"
  ".opt:after{content:'\\203A';margin-left:auto;font-size:28px;color:#6f84b0}"
  ".i1{background:rgba(255,152,0,.2)}.i2{background:rgba(47,128,255,.22)}"
  ".btn{display:block;width:100%;padding:16px;margin:12px 0;border:0;border-radius:16px;font-size:16px;"
  "font-weight:700;color:#fff;text-align:center;text-decoration:none;font-family:inherit}"
  ".bp{background:linear-gradient(135deg,#ff5252,#d61f1f);box-shadow:0 8px 22px rgba(214,31,31,.35)}"
  ".bb{background:linear-gradient(135deg,#3b8cff,#1d57c7)}"
  ".bk{background:linear-gradient(135deg,#25c26e,#14884b)}"
  ".bg{background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.12)}"
  ".fc{background:rgba(255,255,255,.05);border:1px solid rgba(255,255,255,.09);border-radius:20px;padding:16px;margin:16px 0}"
  ".fl{display:block;font-size:11px;letter-spacing:1.2px;text-transform:uppercase;color:#8fa2c9;font-weight:700;margin:18px 0 8px}"
  ".fl.f{margin-top:0}"
  "input[type=text],input[type=number],input[type=tel],textarea{width:100%;padding:14px;font-size:16px;color:#eaf0ff;"
  "background:rgba(0,0,0,.28);border:1px solid rgba(255,255,255,.12);border-radius:12px;font-family:inherit;outline:0}"
  "input[type=text]:focus,input[type=number]:focus,input[type=tel]:focus,textarea:focus{border-color:#3b8cff}"
  ".chips{position:relative;display:flex;flex-wrap:wrap;gap:8px}"
  ".chips input{position:absolute;opacity:0;pointer-events:none}"
  ".chips label{padding:10px 15px;border-radius:99px;font-size:14px;background:rgba(255,255,255,.06);"
  "border:1px solid rgba(255,255,255,.12)}"
  ".chips input:checked+label{background:#ff4d4d;border-color:#ff4d4d;color:#fff;font-weight:700}"
  ".okc{width:96px;height:96px;margin:24px auto 6px;border-radius:50%;background:rgba(37,194,110,.16);"
  "display:flex;align-items:center;justify-content:center;font-size:50px;color:#3ddc84;animation:pop .5s}"
  "@keyframes pop{0%{transform:scale(.4);opacity:0}100%{transform:scale(1);opacity:1}}"
  ".c{text-align:center}"
  ".pill{display:inline-block;padding:7px 18px;border-radius:99px;font-weight:700;font-size:14px}"
  ".wait{background:rgba(255,193,7,.15);color:#ffc94d}.ok{background:rgba(37,194,110,.16);color:#4be08a}"
  ".cx{background:rgba(255,255,255,.1);color:#b8c6e6}"
  ".steps{margin:20px 4px}"
  ".s{display:flex;gap:12px;align-items:center;padding:9px 0;color:#6f84b0;font-size:15px}"
  ".s i{width:26px;height:26px;border-radius:50%;background:rgba(255,255,255,.08);display:flex;"
  "align-items:center;justify-content:center;font-style:normal;font-size:13px}"
  ".s.on{color:#eaf0ff}.s.on i{background:#25c26e;color:#fff}"
  ".stats{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:14px 0}"
  ".stat{background:rgba(255,255,255,.05);border:1px solid rgba(255,255,255,.09);border-radius:16px;"
  "text-align:center;padding:12px 6px;font-size:11px;color:#8fa2c9;text-transform:uppercase;letter-spacing:1px}"
  ".stat b{display:block;font-size:28px;color:#fff;letter-spacing:0;margin-bottom:2px}"
  ".stat.hot{background:rgba(255,77,77,.14);border-color:rgba(255,77,77,.4)}"
  ".live{display:flex;align-items:center;justify-content:center;gap:7px;color:#8fa2c9;font-size:12px;margin:2px 0 8px}"
  ".live i{width:8px;height:8px;border-radius:50%;background:#ff4d4d;animation:bl 1.2s infinite}"
  "@keyframes bl{50%{opacity:.2}}"
  ".cd{background:rgba(255,255,255,.05);border:1px solid rgba(255,255,255,.09);border-left:5px solid #3b8cff;"
  "border-radius:16px;padding:14px;margin:12px 0}"
  ".p3{border-left-color:#ff4d4d}.p2{border-left-color:#ff9800}.p1{border-left-color:#3b8cff}"
  ".dn{opacity:.5;border-left-color:#25c26e}"
  ".badge{display:inline-block;font-size:10px;font-weight:800;letter-spacing:.5px;border-radius:6px;"
  "padding:3px 8px;margin-left:6px;background:#ff4d4d;color:#fff;vertical-align:middle}"
  ".b2{background:#ff9800}.b1{background:#3b8cff}.bd{background:#25c26e}"
  ".small{color:#8fa2c9;font-size:12px}"
  ".kv{display:grid;grid-template-columns:78px 1fr;gap:5px 10px;font-size:14px;margin-top:10px}"
  ".kv span{color:#8fa2c9}"
  ".ft{display:flex;align-items:center;justify-content:center;gap:8px;color:#5d7098;font-size:11px;margin-top:28px}"
  ".ft i{width:7px;height:7px;border-radius:50%;background:#25c26e;box-shadow:0 0 8px #25c26e}";

String esc(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

String clip(String s, int n) {
  s.trim();
  if ((int)s.length() > n) s = s.substring(0, n);
  return s;
}

int needPriority(const String& need) {
  if (need.indexOf("Medical") >= 0 || need.indexOf("Trapped") >= 0 || need.indexOf("Fire") >= 0) return 3;
  if (need.indexOf("Flood") >= 0) return 2;
  return 1;
}

int pendingCount() {
  int c = 0;
  for (int i = 0; i < MAX_REPORTS; i++)
    if (reports[i].used && !reports[i].handled) c++;
  return c;
}

int criticalPending() {
  int c = 0;
  for (int i = 0; i < MAX_REPORTS; i++)
    if (reports[i].used && !reports[i].handled && reports[i].prio == 3) c++;
  return c;
}

int countSOS() {
  int c = 0;
  for (int i = 0; i < MAX_REPORTS; i++)
    if (reports[i].used && reports[i].isSOS) c++;
  return c;
}

int findIndexById(int id) {
  for (int i = 0; i < MAX_REPORTS; i++)
    if (reports[i].used && reports[i].id == id) return i;
  return -1;
}

int newReport(bool sos) {
  int i = nextSlot;
  nextSlot = (nextSlot + 1) % MAX_REPORTS;
  reports[i] = Report();
  reports[i].used = true;
  reports[i].isSOS = sos;
  reports[i].handled = false;
  reports[i].cancelled = false;
  reports[i].id = nextId++;
  reports[i].prio = sos ? 3 : 1;
  reports[i].token = (unsigned long)(esp_random() % 900000UL) + 100000UL;
  reports[i].t = millis();
  totalReceived++;
  muted = false;
  lastInputAt = millis();
  Serial.printf("[NEW] %s #%d | total=%d pending=%d\n",
                sos ? "SOS" : "HELP", reports[i].id, totalReceived, pendingCount());
  return i;
}

String ago(unsigned long t) {
  unsigned long s = (millis() - t) / 1000;
  if (s < 60) return String(s) + " s ago";
  return String(s / 60) + " min ago";
}

String statusUrl(int idx) {
  return "/status?id=" + String(reports[idx].id) + "&k=" + String(reports[idx].token);
}

void redirectTo(String url) {
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
}

String pageHead(const char* title, int refreshSec) {
  String h = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<meta name='theme-color' content='#070b16'>";
  if (refreshSec > 0) h += "<meta http-equiv='refresh' content='" + String(refreshSec) + "'>";
  h += "<title>" + String(title) + "</title><style>" + String(STYLE) + "</style></head><body><div class='wrap'>";
  h += "<div class='hd'><div class='sc'>" + esc(SCHOOL_NAME) + "</div>";
  h += "<div class='lg'><svg viewBox='0 0 24 24' fill='none'>";
  h += "<path d='M12 2l8 3v6c0 5-3.4 9.4-8 11-4.6-1.6-8-6-8-11V5l8-3z' fill='#ff4d4d'/>";
  h += "<path d='M12 7v8M8 11h8' stroke='#fff' stroke-width='2.2' stroke-linecap='round'/></svg>";
  h += "<b>LIFE<i>LINE</i></b></div>";
  h += "<div class='tm'>Team " + esc(TEAM_NAME) + "</div></div>";
  return h;
}

String pageFoot() {
  return "<div class='ft'><i></i>Offline network &middot; no internet needed</div></div></body></html>";
}

String chip(const char* name, const char* id, const char* val, bool chk) {
  String s = "<input type='radio' name='" + String(name) + "' id='" + String(id) + "' value='" + String(val) + "'";
  if (chk) s += " checked";
  s += "><label for='" + String(id) + "'>" + String(val) + "</label>";
  return s;
}

void handleRoot() {
  String p = pageHead("Lifeline", 0);
  p += "<div class='hint'>In danger? Tap once.</div>";
  p += "<div class='sosw'><a class='sosb' href='/sos'><b>SOS</b><span>TAP FOR EMERGENCY</span></a></div>";
  p += "<a class='opt' href='/victim'><div class='ic i1'>&#9995;</div><div><b>I need help</b>";
  p += "<small>Share your details</small></div></a>";
  p += "<a class='opt' href='/rescue'><div class='ic i2'>&#128737;</div><div><b>Rescue team</b>";
  p += "<small>Staff login</small></div></a>";
  p += pageFoot();
  server.send(200, "text/html", p);
}

void handleSOS() {
  uint32_t ipv = (uint32_t)server.client().remoteIP();
  if (ipv == lastSosIp && lastSosId > 0 && (millis() - lastSosAt) < 15000UL) {
    int old = findIndexById(lastSosId);
    if (old >= 0) { redirectTo(statusUrl(old)); return; }
  }
  int idx = newReport(true);
  reports[idx].need = "SOS";
  lastSosIp = ipv;
  lastSosAt = millis();
  lastSosId = reports[idx].id;
  redirectTo(statusUrl(idx));
}

void handleStatus() {
  int idx = findIndexById(server.arg("id").toInt());
  if (idx < 0 || String(reports[idx].token) != server.arg("k")) {
    String e = pageHead("Not found", 0);
    e += "<p class='c'>Not found</p><a class='btn bg' href='/'>Home</a>";
    e += pageFoot();
    server.send(404, "text/html", e);
    return;
  }
  Report& r = reports[idx];
  String link = "id=" + String(r.id) + "&k=" + String(r.token);
  String p = pageHead("Status", 5);
  p += "<div class='okc'>&#10003;</div><h2 class='c' style='margin:6px 0 10px'>Request received</h2><div class='c'>";
  if (r.cancelled) p += "<span class='pill cx'>Cancelled</span>";
  else if (r.handled) p += "<span class='pill ok'>Help is on the way</span>";
  else p += "<span class='pill wait'>Waiting for rescue team</span>";
  p += "</div><div class='steps'>";
  p += "<div class='s on'><i>&#10003;</i>Request sent</div>";
  p += "<div class='s on'><i>&#10003;</i>Rescue team notified</div>";
  p += String("<div class='s") + ((r.handled && !r.cancelled) ? " on" : "") + "'><i>&#10003;</i>Team handled it</div>";
  p += "</div><p class='c small'>Request #" + String(r.id) + " &middot; keep this page open</p>";
  if (!r.handled) {
    p += "<a class='btn bp' href='/victim?" + link + "'>Add more details</a>";
    p += "<a class='btn bg' href='/cancel?" + link + "'>Cancel (false alarm)</a>";
  }
  p += "<a class='btn bg' href='/'>Home</a>";
  p += pageFoot();
  server.send(200, "text/html", p);
}

void handleVictim() {
  String id = server.hasArg("id") ? server.arg("id") : "0";
  String k = server.hasArg("k") ? server.arg("k") : "0";
  String p = pageHead("Need help", 0);
  p += "<form action='/submit' method='POST'><div class='fc'>";
  p += "<input type='hidden' name='id' value='" + esc(id) + "'>";
  p += "<input type='hidden' name='k' value='" + esc(k) + "'>";
  p += "<label class='fl f'>What do you need?</label><div class='chips'>";
  p += chip("need", "n1", "Medical help", true);
  p += chip("need", "n2", "Trapped", false);
  p += chip("need", "n3", "Fire", false);
  p += chip("need", "n4", "Flood / water", false);
  p += chip("need", "n5", "Food / water", false);
  p += chip("need", "n6", "Other", false);
  p += "</div>";
  p += "<label class='fl'>Number of people</label><input type='number' name='people' min='1' value='1'>";
  p += "<label class='fl'>Ages</label><input type='text' name='ages' placeholder='30, 28, 5'>";
  p += "<label class='fl'>Gender</label><div class='chips'>";
  p += chip("gender", "g1", "Male", true);
  p += chip("gender", "g2", "Female", false);
  p += chip("gender", "g3", "Mixed", false);
  p += chip("gender", "g4", "Other", false);
  p += "</div>";
  p += "<label class='fl'>Location</label><input type='text' name='location' placeholder='Landmark, floor, room'>";
  p += "<label class='fl'>Phone (optional)</label><input type='tel' name='phone'>";
  p += "<label class='fl'>Notes</label><textarea name='notes' rows='3'></textarea>";
  p += "</div><button class='btn bp' style='font-size:18px;padding:18px' type='submit'>SEND REQUEST</button></form>";
  p += "<a class='btn bg' href='/'>Back</a>";
  p += pageFoot();
  server.send(200, "text/html", p);
}

void handleSubmit() {
  int id = server.hasArg("id") ? server.arg("id").toInt() : 0;
  int idx = (id > 0) ? findIndexById(id) : -1;
  if (idx >= 0 && String(reports[idx].token) != server.arg("k")) idx = -1;
  if (idx < 0) idx = newReport(false);

  reports[idx].people   = clip(server.arg("people"), 6);
  reports[idx].ages     = clip(server.arg("ages"), 40);
  reports[idx].gender   = clip(server.arg("gender"), 12);
  reports[idx].need     = clip(server.arg("need"), 30);
  reports[idx].location = clip(server.arg("location"), 120);
  reports[idx].phone    = clip(server.arg("phone"), 20);
  reports[idx].notes    = clip(server.arg("notes"), 200);
  int pr = needPriority(reports[idx].need);
  if (reports[idx].isSOS) pr = 3;
  reports[idx].prio = pr;
  reports[idx].handled = false;
  reports[idx].cancelled = false;
  lastInputAt = millis();
  muted = false;
  redirectTo(statusUrl(idx));
}

void handleCancel() {
  int idx = findIndexById(server.arg("id").toInt());
  if (idx >= 0 && String(reports[idx].token) == server.arg("k")) {
    reports[idx].handled = true;
    reports[idx].cancelled = true;
    redirectTo(statusUrl(idx));
    return;
  }
  redirectTo("/");
}

String rescueLogin(String msg) {
  String p = pageHead("Rescue", 0);
  p += "<div class='fc' style='margin-top:30px'><label class='fl f'>Rescue team PIN</label>";
  if (msg.length() > 0) p += "<p style='color:#ff7b7b;margin:0 0 10px'>" + msg + "</p>";
  p += "<form action='/rescue' method='GET'><input type='tel' name='pin' placeholder='Enter PIN'>";
  p += "<button class='btn bb' type='submit' style='margin-bottom:0'>OPEN DASHBOARD</button></form></div>";
  p += "<a class='btn bg' href='/'>Back</a>";
  p += pageFoot();
  return p;
}

bool comesBefore(int x, int y) {
  Report& a = reports[x];
  Report& b = reports[y];
  if (a.handled != b.handled) return !a.handled;
  if (a.prio != b.prio) return a.prio > b.prio;
  return a.id > b.id;
}

String row(const char* k, String v) {
  return "<span>" + String(k) + "</span><div>" + v + "</div>";
}

String listHtml(String pin) {
  String p = "<div data-p='" + String(pendingCount()) + "' data-c='" + String(criticalPending()) + "'>";
  p += "<div class='stats'>";
  p += "<div class='stat'><b>" + String(totalReceived) + "</b>Messages</div>";
  p += "<div class='stat'><b>" + String(countSOS()) + "</b>SOS</div>";
  p += String("<div class='stat") + (pendingCount() > 0 ? " hot" : "") + "'><b>" + String(pendingCount()) + "</b>Pending</div></div>";

  int order[MAX_REPORTS];
  int n = 0;
  for (int i = 0; i < MAX_REPORTS; i++) if (reports[i].used) order[n++] = i;
  for (int a = 0; a < n - 1; a++) {
    for (int b = 0; b < n - 1 - a; b++) {
      if (comesBefore(order[b + 1], order[b])) {
        int tmp = order[b]; order[b] = order[b + 1]; order[b + 1] = tmp;
      }
    }
  }

  if (n == 0) p += "<p class='c small' style='margin:30px 0'>No requests yet</p>";

  for (int k = 0; k < n; k++) {
    Report& r = reports[order[k]];
    String cls = "cd p" + String(r.prio);
    if (r.handled) cls += " dn";
    p += "<div class='" + cls + "'><b>#" + String(r.id) + "</b>";
    if (r.isSOS) p += "<span class='badge'>SOS</span>";
    if (r.prio == 3) p += "<span class='badge'>CRITICAL</span>";
    else if (r.prio == 2) p += "<span class='badge b2'>HIGH</span>";
    else p += "<span class='badge b1'>NORMAL</span>";
    if (r.cancelled) p += "<span class='badge bd'>CANCELLED</span>";
    else if (r.handled) p += "<span class='badge bd'>DONE</span>";
    p += " <span class='small' style='float:right'>" + ago(r.t) + "</span>";
    if (r.people.length() > 0) {
      p += "<div class='kv'>";
      p += row("Need", esc(r.need));
      p += row("People", esc(r.people) + " &middot; " + esc(r.gender));
      if (r.ages.length() > 0) p += row("Ages", esc(r.ages));
      p += row("Location", esc(r.location));
      if (r.phone.length() > 0) p += row("Phone", esc(r.phone));
      if (r.notes.length() > 0) p += row("Notes", esc(r.notes));
      p += "</div>";
    } else {
      p += "<div class='small' style='margin-top:8px'>No details given</div>";
    }
    if (!r.handled) {
      p += "<a class='btn bk' style='padding:12px;font-size:14px;margin:12px 0 0' href='#' ";
      p += "onclick=\"act('/done?id=" + String(r.id) + "&amp;pin=" + pin + "');return false\">Mark as handled</a>";
    }
    p += "</div>";
  }
  p += "<a class='btn bg' style='font-size:13px;padding:12px' href='#' ";
  p += "onclick=\"if(confirm('Clear all?'))act('/clear?pin=" + pin + "');return false\">Clear all</a>";
  p += "</div>";
  return p;
}

void handleRescue() {
  if (!server.hasArg("pin")) { server.send(200, "text/html", rescueLogin("")); return; }
  String pin = server.arg("pin");
  if (pin != String(RESCUE_PIN)) { server.send(200, "text/html", rescueLogin("Wrong PIN")); return; }

  String p = pageHead("Rescue", 0);
  p += "<div class='live'><i></i>LIVE DASHBOARD</div>";
  p += "<a id='sb' class='btn bg' style='font-size:14px;padding:13px' href='#' onclick='tog();return false'>&#128276; Enable alarm sound</a>";
  p += "<div id='box'>" + listHtml(pin) + "</div>";
  p += "<script>var snd=false,ac=null;";
  p += "function beep(n){if(!ac)return;for(var i=0;i<n;i++){var o=ac.createOscillator(),g=ac.createGain();";
  p += "o.type='square';o.frequency.value=880;g.gain.value=0.15;o.connect(g);g.connect(ac.destination);";
  p += "var t=ac.currentTime+i*0.3;o.start(t);o.stop(t+0.18);}}";
  p += "function tog(){var b=document.getElementById('sb');if(!snd){";
  p += "ac=new(window.AudioContext||window.webkitAudioContext)();ac.resume();snd=true;";
  p += "b.innerHTML='&#128266; Alarm ON (tap to mute)';beep(2);}else{snd=false;b.innerHTML='&#128276; Enable alarm sound';}}";
  p += "function refresh(){fetch('/list?pin=" + pin + "').then(function(r){return r.text();}).then(function(h){";
  p += "var box=document.getElementById('box');box.innerHTML=h;var m=box.firstElementChild;";
  p += "if(m&&snd){var pe=+m.getAttribute('data-p'),c=+m.getAttribute('data-c');if(pe>0)beep(c>0?3:1);}";
  p += "}).catch(function(){});}";
  p += "function act(u){fetch(u).then(refresh);}";
  p += "setInterval(refresh,3000);</script>";
  p += pageFoot();
  server.send(200, "text/html", p);
}

void handleList() {
  String pin = server.arg("pin");
  if (pin != String(RESCUE_PIN)) { server.send(403, "text/plain", "Wrong PIN"); return; }
  server.send(200, "text/html", listHtml(pin));
}

void handleDone() {
  String pin = server.arg("pin");
  if (pin != String(RESCUE_PIN)) { server.send(403, "text/plain", "Wrong PIN"); return; }
  int idx = findIndexById(server.arg("id").toInt());
  if (idx >= 0) reports[idx].handled = true;
  server.send(200, "text/plain", "ok");
}

void handleClear() {
  String pin = server.arg("pin");
  if (pin != String(RESCUE_PIN)) { server.send(403, "text/plain", "Wrong PIN"); return; }
  for (int i = 0; i < MAX_REPORTS; i++) reports[i].used = false;
  totalReceived = 0;
  nextSlot = 0;
  muted = false;
  lastSosId = 0;
  server.send(200, "text/plain", "ok");
}

void redirectToPortal() {
  redirectTo("http://192.168.4.1/");
}

void checkButton() {
  static int lastRead = HIGH;
  static unsigned long lastChange = 0;
  int r = digitalRead(BUTTON_PIN);
  if (r != lastRead && (millis() - lastChange) > 50) {
    lastChange = millis();
    lastRead = r;
    if (r == LOW) {
      muted = true;
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

unsigned long lastBuzz = 0;
unsigned long lastLed = 0;
bool buzzState = false;
bool ledState = false;

void updateAlarm() {
  if (pendingCount() > 0) {
    unsigned long iv = (criticalPending() > 0) ? 150UL : 500UL;
    if (millis() - lastBuzz >= iv) {
      lastBuzz = millis();
      buzzState = !buzzState;
      digitalWrite(BUZZER_PIN, (buzzState && !muted) ? HIGH : LOW);
    }
    if (millis() - lastInputAt < 3000UL) {
      if (millis() - lastLed >= 80) {
        lastLed = millis();
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      }
    } else {
      digitalWrite(LED_PIN, HIGH);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  for (int i = 0; i < MAX_REPORTS; i++) reports[i].used = false;

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);
  delay(200);
  dnsServer.start(53, "*", apIP);

  server.on("/", handleRoot);
  server.on("/sos", handleSOS);
  server.on("/status", handleStatus);
  server.on("/victim", handleVictim);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/cancel", handleCancel);
  server.on("/rescue", handleRescue);
  server.on("/list", handleList);
  server.on("/done", handleDone);
  server.on("/clear", handleClear);
  server.on("/generate_204", redirectToPortal);
  server.on("/gen_204", redirectToPortal);
  server.on("/hotspot-detect.html", redirectToPortal);
  server.on("/fwlink", redirectToPortal);
  server.on("/connecttest.txt", redirectToPortal);
  server.onNotFound(redirectToPortal);
  server.begin();

  Serial.println("Ready: join LIFELINE_SOS, open http://192.168.4.1");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  checkButton();
  updateAlarm();
}
