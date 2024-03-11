#include <EEPROM.h>
#include <Ethernet.h>
#include <SPI.h>
#include <avr/pgmspace.h>

#define relayPin 2
#define btnPin 6

#define ipStartAddress 0
#define dnsStartAddress 4
#define ipTrackStartAddress 8
#define ipTrackStatStartAddress 12
#define gatewayStartAddress 16
#define subnetStartAddress 20
#define passwordStartAddress 24
#define timeDelayStartAddress 60
#define timeOffStartAddress 68
#define timeLoadStartAddress 76
#define timePingStartAddress 84

#define maxPasswordLength 32

/*
BTN - A6 & VCC

relayPin - D2 & GND & VCC

W5500 - Arduino Nano.
SCLK - D13
MOSI - D11
MISO - D12
SCS - D10
GND
VCC
*/

const char web_code_302[] PROGMEM =
    "HTTP/1.1 302 Found\n"
    "Location: /\n"
    "Connection: close\n"
    "\n";

const char web_code_200[] PROGMEM =
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "Connection: close\n"
    "\n";

const char styles[] PROGMEM =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=0.5'><title>Управление устройством</title><style>"
    "body, .container {font-family: sans-serif;background-color: #EEE;}"
    ".container {max-width: 600px;margin: 20px auto;padding: 20px;background-color: #fff;border-radius: 8px;box-shadow: 0 2px 4px rgba(0, 0, 0, .1);}"
    "h2 {margin: 0 0 50px;color: #333;}"
    "label {display: block;font-weight: bold;margin-bottom: 8px;}"
    "input {width: 90%;padding: 8px;margin-bottom: 20px;border: 1px solid #ccc;border-radius: 4px;}"
    ".btn {display-block;background-color: #4285f4;color: #fff;border: none;"
    "border-radius: 4px;padding: 10px 20px;font-size: 16px;width: 40%;}"
    ".smb {margin-left:55%;}"
    ".m10 {margin-left: 5%;margin-right: 5%;width: 80%;}"
    ".m30 {margin-left: 30%;margin-right: 30%;}"
    ".flex-container {vertical-align: top;}"
    ".w50 {display: inline-block;vertical-align: top;width: 48%;margin: 1%; }"
    ".button-group {display: flex;margin-bottom: 20px;}"
    ".animatable-element {height: 8px; background-color: black; transition: all 0.5s ease; overflow: hidden;display: flex; border-radius:99px;}"
    ".animatable-element:hover {height: 80px; background-color: transparent;border-radius:0px;}"
    "</style>"
    "</head>";

const char web_page_index_part_1[] PROGMEM =
    "<body style='user-select: none;'><div class='container'>"
    "<h2>Управление розеткой</h2>"
    "<div class='w50'><div class='button-group'><button onclick=\"location='/toggle';\" class=\"m10 btn\" '>";

const char web_page_index_part_2[] PROGMEM =
    "</button> <button onclick=\"location='/restart';\" class=\"m10 btn\">Рестарт</button></div></div>"
    "<div class='w50'>"
    "<form action='/track'>"
    "<label>Отслеживание состояния</label><input type='text' pattern='^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$' name='ip' value=";

const char web_page_index_part_2_1[] PROGMEM =
    ">"
    "<label>Время ожидания ответа (сек)</label><p class='animatable-element'>Сколько ждать ответа от IP перед перезагрузкой</p><input type='number' min='1' max='600' name='timeDelay' value=";

const char web_page_index_part_2_2[] PROGMEM =
    ">"
    "<label>Время выключения (сек)</label><p class='animatable-element'>на сколько отключить питание</p><input type='number' min='1' max='600'  name='timeOff' value=";

const char web_page_index_part_2_3[] PROGMEM =
    ">"
    "<label>Время загрузки (сек)</label><p class='animatable-element'>на сколько отложить отслеживание после перезагрузки отслеживаемого устройства</p><input type='number' min='1' max='600'  name='timeLoad' value=";

const char web_page_index_part_2_4[] PROGMEM =
    ">"
    "<label>Таймаут пинга (мс) </label><input type='number' min='1' max='20000'  name='timePing' value=";

const char web_page_index_part_3[] PROGMEM =
    ">"
    "<input type='submit' value='Применить' class=\"btn smb\">"
    "</form>"
    "<button onclick=\"location='/toggleTrack';\" class=\"btn smb\" '>";

const char web_page_index_part_4[] PROGMEM =
    "</button>"
    "</div>"
    "<div><div class='w50'>"
    "<form action='/password'>"
    "<label>Новый пароль:</label><input type='text' name='password'>"
    "<input type='submit' value='Применить' class=\"btn smb\">"
    "</form></div>"
    "<div class='w50'><form action='/settings'>"
    "<label>Новый IP:</label><input type='text' pattern='^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$' name='ip' value='";

const char web_page_index_part_5[] PROGMEM =
    "'>"
    "<label>Новый DNS:</label><input type='text' pattern='^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$' name='dns' value='";

const char web_page_index_part_6[] PROGMEM =
    "'>"
    "<label>Новый шлюз:</label><input type='text' pattern='^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$' name='gateway' value='";

const char web_page_index_part_7[] PROGMEM =
    "'>"
    "<label>Новая подсеть:</label><input type='text' pattern='^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$' name='subnet' value='";


const char web_page_index_part_8[] PROGMEM =
    "'>"
    "<input type='submit' value='Применить' class=\"btn smb\">"
    "</form>"
    "</div></div></div></body></html>\n\n";

const char web_page_login[] PROGMEM =
    "<body><div class='container'>"
    "<form action='/login' method='get'>"
    "<label>Пароль:</label><input type='password' name='password'>"
    "<input type='submit' value='Войти' class=\"btn m30\">"
    "</form>"
    "</div></body></html>\n\n";

const char web_page_restart[] PROGMEM =
    "<body><div class='container'>"
    "<h1>Сервер перезагружается...</h1>"
    "<script>setTimeout(function(){document.location.href='/'}, 2000);</script>"
    "</div></body></html>\n\n";

const char web_page_cookie_part_1[] PROGMEM =
    "HTTP/1.1 200 OK\n"
    "Set-Cookie: password=";

const char web_page_cookie_part_2[] PROGMEM =
    "; Path=/; HttpOnly\n"
    "Content-Type: text/html\n"
    "Connection: close\n\n";

const char web_page_cookie_part_3[] PROGMEM =
    "<body><div class='container'>"
    "<h1>Успешный вход.</h1>"
    "</div></body>"
    "<script>document.location='/'</script>"
    "</html>\n\n";

void sendHtmlPage(EthernetClient &client, const char *page) {
  char *buffer = new char[80];
  unsigned int pageLength = strlen_P(page);
  for (unsigned int i = 0; i < pageLength; i++) {
    buffer[i % 80] = pgm_read_byte_near(page + i);

    if (i % 80 == 79 || i == pageLength - 1) {
      buffer[(i % 80) + 1] = '\0';
      client.print(buffer);
    }
  }
  delete[] buffer;
}

void ip2byte(const char *inputString, byte ipBytes[4]) {
  char ipCopy[16];
  strncpy(ipCopy, inputString, 15);
  ipCopy[15] = '\0';
  char *p = strtok(ipCopy, ".");
  int i = 0;

  while (p != NULL && i < 4) {
    int value = atoi(p);
    ipBytes[i] = (byte)value;
    p = strtok(NULL, ".");
    i++;
  }
}

void extract(const String &data, const String &key, String &result) {
  int keyStart = data.indexOf(key);

  if (keyStart == -1) {
    result = "";
    return;
  }

  int lineStart = data.lastIndexOf('\r', keyStart) + 1;
  if (lineStart == -1) lineStart = 0;

  int lineEnd = data.indexOf('\r', keyStart);
  if (lineEnd == -1) lineEnd = data.length();

  result = data.substring(lineStart, lineEnd);
}

void exURL(const String &inputStr, const String &splitBy, int index) {
  int startIndex = 0;
  int endIndex = 0;
  int foundCount = 0;

  while (endIndex != -1) {
    endIndex = inputStr.indexOf(splitBy, startIndex);

    if (foundCount == index) {
      if (endIndex == -1) endIndex = inputStr.length();
      inputStr = inputStr.substring(startIndex, endIndex);
      return;
    }
    foundCount++;
    startIndex = endIndex + splitBy.length();
  }

  inputStr = "";
}

void splitRequest(const String &request, String &path, String &params) {
  int paramsStart = request.indexOf("?");

  if (paramsStart != -1) {
    path = request.substring(0, paramsStart);
    params = request.substring(paramsStart + 1);
  } else {
    path = request;
    params = "";
  }
}

String parseParams(const String &params, const String &key) {
  String keyWithEqual = key + "=";
  int start = params.indexOf(keyWithEqual);

  if (start == -1) return "";
  start += keyWithEqual.length();
  int end = params.indexOf('&', start);

  if (end == -1)
    return params.substring(start);
  else
    return params.substring(start, end);
}

void IndexPage(EthernetClient &client) {
  pageCode200(client);
  byte ip[4], dns[4], gateway[4], subnet[4];
  byte ipTrack[4];
  bool ipTrackStat;
  unsigned int timeDelay, timeOff, timeLoad, timePing;

  readSettings(ip, dns, gateway, subnet);
  readTrack(ipTrack, &ipTrackStat, timeDelay, timeOff, timeLoad, timePing);

  sendHtmlPage(client, styles);
  sendHtmlPage(client, web_page_index_part_1);
  if (digitalRead(relayPin))
    client.println(F("ВКЛ"));
  else
    client.println(F("ВЫКЛ"));

  sendHtmlPage(client, web_page_index_part_2);
  for (int i = 0; i < 4; ++i) {
    client.print(ipTrack[i], DEC);
    if (i < 3) client.print(F("."));
  }

  sendHtmlPage(client, web_page_index_part_2_1);
  client.println(timeDelay);

  sendHtmlPage(client, web_page_index_part_2_2);
  client.println(timeOff);

  sendHtmlPage(client, web_page_index_part_2_3);
  client.println(timeLoad);

  sendHtmlPage(client, web_page_index_part_2_4);
  client.println(timePing);

  sendHtmlPage(client, web_page_index_part_3);
  if (ipTrackStat)
    client.println(F("ВЫКЛ"));
  else
    client.println(F("ВКЛ"));

  sendHtmlPage(client, web_page_index_part_4);
  for (int i = 0; i < 4; ++i) {
    client.print(ip[i], DEC);
    if (i < 3) client.print(F("."));
  }

  sendHtmlPage(client, web_page_index_part_5);
  for (int i = 0; i < 4; ++i) {
    client.print(dns[i], DEC);
    if (i < 3) client.print(F("."));
  }

  sendHtmlPage(client, web_page_index_part_6);
  for (int i = 0; i < 4; ++i) {
    client.print(gateway[i], DEC);
    if (i < 3) client.print(F("."));
  }

  sendHtmlPage(client, web_page_index_part_7);
  for (int i = 0; i < 4; ++i) {
    client.print(subnet[i], DEC);
    if (i < 3) client.print(F("."));
  }

  sendHtmlPage(client, web_page_index_part_8);
}

void loginPage(EthernetClient &client) {
  pageCode200(client);
  sendHtmlPage(client, styles);
  sendHtmlPage(client, web_page_login);
}

void restartPage(EthernetClient &client) {
  pageCode200(client);
  sendHtmlPage(client, styles);
  sendHtmlPage(client, web_page_restart);
}

void setting(EthernetClient &client, const String &ip, const String &dns, const String &gateway, const String &subnet) {
  byte intIp[4], intDns[4], intGateway[4], intSubnet[4];

  ip2byte(ip.c_str(), intIp);
  ip2byte(dns.c_str(), intDns);
  ip2byte(gateway.c_str(), intGateway);
  ip2byte(subnet.c_str(), intSubnet);
  
  

  writeSettings((byte *)intIp, (byte *)intDns, (byte *)intGateway, (byte *)intSubnet);

  pageCode302(client);
}
void setCookie(EthernetClient &client, String inputPassword) {
  sendHtmlPage(client, web_page_cookie_part_1);

  client.print(inputPassword);

  sendHtmlPage(client, web_page_cookie_part_2);
  sendHtmlPage(client, styles);
  sendHtmlPage(client, web_page_cookie_part_3);
}

void pageCode302(EthernetClient &client) { sendHtmlPage(client, web_code_302); }

void pageCode200(EthernetClient &client) { sendHtmlPage(client, web_code_200); }

void toggleTrack() {
  byte ipTrack[4];
  bool ipTrackStat;
  unsigned int timeDelay, timeOff, timeLoad, timePing;

  readTrack(ipTrack, &ipTrackStat, timeDelay, timeOff, timeLoad, timePing);
  ipTrackStat = !ipTrackStat;
  writeTrack(ipTrack, &ipTrackStat, timeDelay, timeOff, timeLoad, timePing);
}

void SetTrackSettings(const String &ip, const String &val_timeDelay, const String &val_timeOff, const String &val_timeLoad, const String &val_timePing) {
  byte intIp[4], intDns[4], ipTrack[4];
  bool ipTrackStat;
  unsigned int timeDelay, timeOff, timeLoad, timePing;

  ip2byte(ip.c_str(), intIp);

  readTrack(ipTrack, &ipTrackStat, timeDelay, timeOff, timeLoad, timePing);

  // Конвертация String в unsigned int
  timeDelay = val_timeDelay.toInt();
  timeOff = val_timeOff.toInt();
  timeLoad = val_timeLoad.toInt();
  timePing = val_timePing.toInt();

  writeTrack(intIp, &ipTrackStat, timeDelay, timeOff, timeLoad, timePing);
}

void writeAddressToEEPROM(int address, byte *values) {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(address + i, values[i]);
  }
}

void readAddressFromEEPROM(int address, byte *values) {
  for (int i = 0; i < 4; i++) {
    values[i] = EEPROM.read(address + i);
  }
}

void writeSettings(byte *ip, byte *dns, byte *gateway, byte *subnet) {
  writeAddressToEEPROM(ipStartAddress, ip);
  writeAddressToEEPROM(dnsStartAddress, dns);
  writeAddressToEEPROM(gatewayStartAddress, gateway);
  writeAddressToEEPROM(subnetStartAddress, subnet);
}

void readSettings(byte *ip, byte *dns, byte *gateway, byte *subnet) {
  readAddressFromEEPROM(ipStartAddress, ip);
  readAddressFromEEPROM(dnsStartAddress, dns);
  readAddressFromEEPROM(gatewayStartAddress, gateway);
  readAddressFromEEPROM(subnetStartAddress, subnet);
}

void writeIntIntoEEPROM(int address, unsigned int value) {
  byte lowByte = value & 0xFF;
  byte highByte = (value >> 8) & 0xFF;

  EEPROM.write(address, lowByte);
  EEPROM.write(address + 1, highByte);
}

unsigned int readIntFromEEPROM(int address) {
  byte lowByte = EEPROM.read(address);
  byte highByte = EEPROM.read(address + 1);

  return (unsigned int)lowByte + ((unsigned int)highByte << 8);
}

void writeTrack(byte *ipTrack, bool *ipTrackStat, unsigned int timeDelay, unsigned int timeOff, unsigned int timeLoad, unsigned int timePing) {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(ipTrackStartAddress + i, ipTrack[i]);
  }
  EEPROM.write(ipTrackStatStartAddress, *ipTrackStat ? 1 : 0);  // Приведение bool к byte

  writeIntIntoEEPROM(timeDelayStartAddress, timeDelay);
  writeIntIntoEEPROM(timeOffStartAddress, timeOff);
  writeIntIntoEEPROM(timeLoadStartAddress, timeLoad);
  writeIntIntoEEPROM(timePingStartAddress, timePing);
}

void readTrack(byte *ipTrack, bool *ipTrackStat, unsigned int &timeDelay, unsigned int &timeOff, unsigned int &timeLoad, unsigned int &timePing) {
  for (int i = 0; i < 4; i++) {
    ipTrack[i] = EEPROM.read(ipTrackStartAddress + i);
  }
  *ipTrackStat = EEPROM.read(ipTrackStatStartAddress) == 1;  // Преобразование byte обратно в bool

  timeDelay = readIntFromEEPROM(timeDelayStartAddress);
  timeOff = readIntFromEEPROM(timeOffStartAddress);
  timeLoad = readIntFromEEPROM(timeLoadStartAddress);
  timePing = readIntFromEEPROM(timePingStartAddress);
}

void writePassword(const char *password) {
  for (int i = 0; i < maxPasswordLength; i++) {
    EEPROM.write(passwordStartAddress + i, password[i]);

    if (password[i] == '\0') break;
  }

  EEPROM.write(passwordStartAddress + maxPasswordLength, '\0');
}

String readPassword() {
  char password[maxPasswordLength + 1];

  for (int i = 0; i < maxPasswordLength; i++) {
    password[i] = EEPROM.read(passwordStartAddress + i);

    if (password[i] == '\0') break;
  }
  password[maxPasswordLength] = '\0';
  return String(password);
}

void defaultSettings() {
  byte ip[4] = {10, 0, 0, 1};
  byte dns[4] = {8, 8, 8, 8};
  byte ipTrack[4] = {10, 0, 0, 2};
  
  byte gateway[4] = {10, 0, 0, 10};
  byte subnet[4] = {255, 0, 0, 0};
  
  
  bool ipTrackStat = false;
  unsigned int timeDelay = 30;
  unsigned int timeOff = 10;
  unsigned int timeLoad = 300;
  unsigned int timePing = 5000;

  writeSettings(ip, dns, gateway, subnet);
  writeTrack(ipTrack, &ipTrackStat, timeDelay, timeOff, timeLoad, timePing);

  writePassword("pass");
  void (*resetFunc)(void) = 0;
  resetFunc();
}

bool ping(EthernetClient &client, IPAddress &server, int pingTime) {
  if (client.connect(server, 80)) {
    // Отправляем HTTP GET запрос
    client.println(F("GET / HTTP/1.1"));
    client.println(F("Connection: close"));
    client.println();  // Пустая строка завершает запрос

    // Ожидаем начала ответа
    unsigned long timeout = millis() + pingTime;  // 5 секунд таймаут
    while (!client.available() && millis() < timeout) {
      ;  // Просто ждем начала ответа или таймаута
    }

    if (client.available()) {
      client.stop();
      return true;
    } else {
      client.stop();
      return false;
    }
  } else {
    return false;
  }
}

EthernetServer server(80);
int counter = 0;
int offline_counter = 0;
int core_timer = 0;
int timer = 605;

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  pinMode(btnPin, INPUT);

  digitalWrite(relayPin, 1);

  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  byte ip[4];
  byte dns[4];
  byte gateway[4];
  byte subnet[4];

  readSettings(ip, dns, gateway, subnet);

  Ethernet.begin(mac, ip, dns, gateway, subnet);
  server.begin();

  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());

  Serial.println();
}

void loop() {
  EthernetClient client = server.available();

  String httpReq = "";
  byte ipTrack[4];
  bool ipTrackStat;
  unsigned int timeDelay, timeOff, timeLoad, timePing;

  readTrack(ipTrack, &ipTrackStat, timeDelay, timeOff, timeLoad, timePing);

  IPAddress server(ipTrack);

  core_timer++;
  core_timer %= 600000;

  int btnStatus = analogRead(btnPin);

  if (btnStatus > 1000) {
    counter++;

    if (counter > 20) defaultSettings();

    delay(100);

  } else
    counter = 0;

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        httpReq += c;

        if (httpReq.endsWith("\r\n\r\n")) {
          serverLogic(client, httpReq);
          break;
        }
      }
    }
  }

  if (core_timer % timePing == 0) {  // Действие происходит раз в сек
    timer++;
    if (timer > timeLoad) {
      if (ipTrackStat) {
        if (digitalRead(relayPin) == 0) {
          if (timeDelay < 5) {
            timeDelay = 5;
          }
          if (core_timer % (1000 * (timeDelay / 5)) == 0) {
            if (ping(client, server, timePing)) {
              offline_counter = 0;
              Serial.print("Online");
            } else {
              offline_counter++;
              Serial.print("Offline ");
              Serial.println(offline_counter);
              core_timer += timePing;
            }
          }

          if (offline_counter == 5) {
            digitalWrite(relayPin, 1);  // Выкл
            delay(timeOff * 1000);
            digitalWrite(relayPin, 0);  // Вкл
            offline_counter = 0;
            timer = 0;
          }
        } else
          offline_counter = 0;
      }
    }
  }

  delay(1);
  client.stop();
}

void serverLogic(EthernetClient &client, const String &httpReq) {
  String path, params, pass, getRqData, CookieRqData, inputPassword, savedPassword;

  extract(httpReq, "GET", getRqData);

  exURL(getRqData, " ", 1);

  splitRequest(getRqData, path, params);

  extract(httpReq, "Cookie", CookieRqData);

  exURL(CookieRqData, " ", 1);

  pass = parseParams(CookieRqData, "password");

  inputPassword = parseParams(params, "password");

  savedPassword = readPassword();

  if (pass == savedPassword) {
    if (path == "/settings") {
      String valueIp = parseParams(params, "ip");
      String valueDns = parseParams(params, "dns");
      String valueGateway = parseParams(params, "gateway");
      String valueSubnet = parseParams(params, "subnet");
      
      setting(client, valueIp, valueDns, valueGateway, valueSubnet);

    } else if (path == "/toggle") {
      int relayState = digitalRead(relayPin);
      relayState = !relayState;
      digitalWrite(relayPin, relayState);
      pageCode302(client);

    } else if (path == "/password") {
      writePassword(inputPassword.c_str());
      pageCode302(client);

    } else if (path == "/restart") {
      restartPage(client);
      void (*resetFunc)(void) = 0;
      resetFunc();
    } else if (path == "/login") {
      pageCode302(client);
    } else if (path == "/toggleTrack") {
      toggleTrack();
      pageCode302(client);
    } else if (path == "/track") {
      String valueIp = parseParams(params, "ip");
      String valueTimeDelay = parseParams(params, "timeDelay");
      String valueTimeOff = parseParams(params, "timeOff");
      String valueTimeLoad = parseParams(params, "timeLoad");
      String valueTimePing = parseParams(params, "timePing");

      SetTrackSettings(valueIp, valueTimeDelay, valueTimeOff, valueTimeLoad, valueTimePing);
      pageCode302(client);
    } else {
      IndexPage(client);
    }
  } else {
    if (path == "/login") {
      if (inputPassword == savedPassword) {
        setCookie(client, inputPassword);
        pageCode302(client);
      } else {
        pageCode302(client);
      }
    } else {
      loginPage(client);
    }
  }
}
