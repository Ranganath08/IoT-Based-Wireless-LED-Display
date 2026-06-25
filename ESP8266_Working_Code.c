// ================= FULL CORRECTED MAIN .INO CODE ==================
// Includes EEPROM permanent saving for Text1, Text2 & all settings

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DMDESP.h>
#include <fonts/Mono5x7.h>
#include <fonts/ElektronMart6x8.h>
#include "PageIndex.h"
#include <EEPROM.h>
#include <math.h>

// ---------------- WiFi AP ----------------
const char* ssid     = "NodeMCU_ESP8266";
const char* password = "goodluck";

ESP8266WebServer server(80);

// ---------------- P10 MATRIX CONFIG (6 PANELS: 3x2) ------------------
#define DISPLAYS_WIDE 3
#define DISPLAYS_HIGH 2
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);

// ---------------- Two-line Text Buffers -----------------------------
char Text1[200] = "WELCOME LINE 1";
char Text2[200] = "WELCOME LINE 2";

// ---------------- Independent Settings ------------------------------
uint8_t font1 = 0, font2 = 0;
uint8_t mode1 = 0, mode2 = 0;
uint16_t speed1 = 50, speed2 = 50;

uint8_t brightnessGlobal = 200;   // 0–255 (single global brightness)

// ---------------- Animation State ------------------------------
int scrollX1 = 0, scrollX2 = 0;
int bounceX1 = 0, bounceX2 = 0;
int bounceDir1 = 1, bounceDir2 = 1;
int typingIndex1 = 0, typingIndex2 = 0;

unsigned long lastAnim1 = 0, lastAnim2 = 0;
unsigned long lastTyping1 = 0, lastTyping2 = 0;

// Fade variables (smooth fade mode)
int fadeV1 = 0, fadeV2 = 0;
int fadeDir1 = 1, fadeDir2 = 1;

// ================= EEPROM SAVE/LOAD ===================
#define EEPROM_SIZE 600  // enough for texts and settings

void saveToEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  // Save Text1
  for (int i = 0; i < 200; i++)
    EEPROM.write(i, Text1[i]);

  // Save Text2
  for (int i = 0; i < 200; i++)
    EEPROM.write(200 + i, Text2[i]);

  // Save settings
  EEPROM.write(400, mode1);
  EEPROM.write(401, mode2);
  EEPROM.write(402, font1);
  EEPROM.write(403, font2);

  EEPROM.write(404, speed1 >> 8);
  EEPROM.write(405, speed1 & 0xFF);
  EEPROM.write(406, speed2 >> 8);
  EEPROM.write(407, speed2 & 0xFF);

  EEPROM.write(408, brightnessGlobal);

  EEPROM.commit();
}

void loadFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  // Load Text1
  for (int i = 0; i < 200; i++)
    Text1[i] = EEPROM.read(i);

  // Load Text2
  for (int i = 0; i < 200; i++)
    Text2[i] = EEPROM.read(200 + i);

  // Load settings
  mode1 = EEPROM.read(400);
  mode2 = EEPROM.read(401);

  font1 = EEPROM.read(402);
  font2 = EEPROM.read(403);

  speed1 = (EEPROM.read(404) << 8) | EEPROM.read(405);
  speed2 = (EEPROM.read(406) << 8) | EEPROM.read(407);

  brightnessGlobal = EEPROM.read(408);
  if (brightnessGlobal > 255) brightnessGlobal = 180;
}

// ---------------- Get Y coordinate ---------------------
int getY(int line) {
  return (line == 1 ? 20 : 4);   // bottom / top alignment
}

// ---------------- FONT SELECTOR -------------------------------
void applyFont(uint8_t f) {
  if (f == 0) Disp.setFont(Mono5x7);
  else Disp.setFont(ElektronMart6x8);
}

// ------------------- ROUTE: HANDLE WEB INPUT ------------------------
void handle_Incoming() {

  if (server.hasArg("text1")) {
    String s = server.arg("text1");  s.toCharArray(Text1, 200);
    typingIndex1 = 0;
  }
  if (server.hasArg("text2")) {
    String s = server.arg("text2");  s.toCharArray(Text2, 200);
    typingIndex2 = 0;
  }

  if (server.hasArg("font1")) font1 = server.arg("font1").toInt();
  if (server.hasArg("font2")) font2 = server.arg("font2").toInt();

  if (server.hasArg("mode1")) mode1 = server.arg("mode1").toInt();
  if (server.hasArg("mode2")) mode2 = server.arg("mode2").toInt();

  if (server.hasArg("speed1")) {
    int s = server.arg("speed1").toInt();
    if (s < 1) s = 1;
    speed1 = s;
  }
  if (server.hasArg("speed2")) {
    int s = server.arg("speed2").toInt();
    if (s < 1) s = 1;
    speed2 = s;
  }

  if (server.hasArg("bg")) {
    int b = server.arg("bg").toInt();
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    brightnessGlobal = b;
  }

  saveToEEPROM();   // <<<<<< PERMANENT SAVE HERE

  server.send(200, "text/plain", "OK");
}

// ---------------- EFFECTS ----------------------------

void effectScrollLeft(const char *txt, int line) {
  int &sx = (line == 1 ? scrollX1 : scrollX2);
  unsigned long &tm = (line == 1 ? lastAnim1 : lastAnim2);
  uint16_t sp = (line == 1 ? speed1 : speed2);

  applyFont(line == 1 ? font1 : font2);

  int y = getY(line);
  int textW  = Disp.textWidth(txt);

  if (millis() - tm >= sp) {
    tm = millis();
    sx--;
    if (sx < -textW) sx = Disp.width();
  }

  Disp.drawText(sx, y, txt);
}

void effectScrollRight(const char *txt, int line) {
  int &sx = (line == 1 ? scrollX1 : scrollX2);
  unsigned long &tm = (line == 1 ? lastAnim1 : lastAnim2);
  uint16_t sp = (line == 1 ? speed1 : speed2);

  applyFont(line == 1 ? font1 : font2);

  int y = getY(line);
  int textW  = Disp.textWidth(txt);

  if (millis() - tm >= sp) {
    tm = millis();
    sx++;
    if (sx > Disp.width()) sx = -textW;
  }

  Disp.drawText(sx, y, txt);
}

void effectBounce(const char *txt, int line) {
  int &bx = (line == 1 ? bounceX1 : bounceX2);
  int &bd = (line == 1 ? bounceDir1 : bounceDir2);
  unsigned long &tm = (line == 1 ? lastAnim1 : lastAnim2);
  uint16_t sp = (line == 1 ? speed1 : speed2);

  applyFont(line == 1 ? font1 : font2);

  int y = getY(line);
  int textW  = Disp.textWidth(txt);

  if (millis() - tm >= sp) {
    tm = millis();
    bx += bd;

    if (bx < -(textW - 1)) bd = 1;
    if (bx > Disp.width() - 1)   bd = -1;
  }

  Disp.drawText(bx, y, txt);
}

void effectStaticCentered(const char *txt, int line) {
  int y = getY(line);
  applyFont(line == 1 ? font1 : font2);
  int x = (Disp.width() - Disp.textWidth(txt)) / 2;
  if (x < 0) x = 0;
  Disp.drawText(x, y, txt);
}

// Blink — speed-controlled
void effectBlink(const char *txt, int line) {
  uint16_t sp = (line == 1 ? speed1 : speed2);
  if (sp < 1) sp = 1;

  unsigned long period = sp * 40UL;
  unsigned long half = period / 2;
  unsigned long t = millis() % period;

  if (t >= half) return;

  applyFont(line == 1 ? font1 : font2);
  int y = getY(line);
  int x = (Disp.width() - Disp.textWidth(txt)) / 2;
  Disp.drawText(x, y, txt);
}

// Typing
void effectTyping(const char *txt, int line) {
  int &ti = (line == 1 ? typingIndex1 : typingIndex2);
  unsigned long &tm = (line == 1 ? lastTyping1 : lastTyping2);
  uint16_t sp = (line == 1 ? speed1 : speed2);
  if (sp < 1) sp = 1;

  if (millis() - tm >= sp) {
    tm = millis();
    ti++;
    if (ti > (int)strlen(txt)) ti = 0;
  }

  char temp[200];
  int copyLen = ti;
  if (copyLen > 199) copyLen = 199;
  strncpy(temp, txt, copyLen);
  temp[copyLen] = '\0';

  applyFont(line == 1 ? font1 : font2);

  int y = getY(line);
  int x = (Disp.width() - Disp.textWidth(temp)) / 2;

  Disp.drawText(x, y, temp);
}

// Fade (smooth)
void effectFade(const char *txt, int line) {
  uint16_t sp = (line == 1 ? speed1 : speed2);
  if (sp < 5) sp = 5;

  float freq = 0.0016f * (50.0f / sp);
  float osc = (sinf(millis() * freq) + 1.0f) * 0.5f;

  int duty = osc * 255;
  if (duty < 12) duty = 12;

  unsigned long pwmPeriod = 30;
  unsigned long onMs = (duty * pwmPeriod) / 255;
  unsigned long phase = millis() % pwmPeriod;

  if (phase >= onMs) return;

  applyFont(line == 1 ? font1 : font2);
  int y = getY(line);
  int x = (Disp.width() - Disp.textWidth(txt)) / 2;
  Disp.drawText(x, y, txt);
}

// Smiley
void effectSmiley(int line) {
  int cy = getY(line) + 6;
  int cx = Disp.width() / 2;

  for (int a = 0; a < 360; a += 12) {
    float r = a * 3.14 / 180;
    int x = cx + 7 * cos(r);
    int y = cy + 7 * sin(r);
    Disp.setPixel(x, y, 1);
  }
  Disp.setPixel(cx - 3, cy - 2, 1);
  Disp.setPixel(cx + 3, cy - 2, 1);

  for (int x = -3; x <= 3; x++) {
    int y = 3 + (0.5 * x * x / 3);
    Disp.setPixel(cx + x, cy + y, 1);
  }
}

// ---------------- SETUP ------------------------------
void setup() {
  Serial.begin(115200);

  loadFromEEPROM();  // <<< RESTORE SAVED MESSAGE

  Disp.start();
  Disp.setBrightness(brightnessGlobal);

  WiFi.softAP(ssid, password);

  server.on("/setText", handle_Incoming);
  server.on("/", []() {
    server.send(200, "text/html", MAIN_page);
  });
  server.begin();
}

// ---------------- MAIN LOOP ----------------------------
void loop() {
  server.handleClient();
  Disp.loop();

  Disp.clear();
  Disp.setBrightness(brightnessGlobal);

  // LINE 1
  switch (mode1) {
    case 0: effectScrollLeft(Text1, 1); break;
    case 1: effectScrollRight(Text1, 1); break;
    case 2: effectBounce(Text1, 1); break;
    case 3: effectStaticCentered(Text1, 1); break;
    case 4: effectBlink(Text1, 1); break;
    case 5: effectTyping(Text1, 1); break;
    case 6: effectFade(Text1, 1); break;
    case 7: effectSmiley(1); break;
  }

  // LINE 2
  switch (mode2) {
    case 0: effectScrollLeft(Text2, 2); break;
    case 1: effectScrollRight(Text2, 2); break;
    case 2: effectBounce(Text2, 2); break;
    case 3: effectStaticCentered(Text2, 2); break;
    case 4: effectBlink(Text2, 2); break;
    case 5: effectTyping(Text2, 2); break;
    case 6: effectFade(Text2, 2); break;
    case 7: effectSmiley(2); break;
  }
}

// ================= END MAIN .INO =====================
