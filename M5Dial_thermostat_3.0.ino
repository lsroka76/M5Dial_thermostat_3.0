//#define SUPLA_DISABLE_LOGS

#include <Wire.h>

#include <M5Dial.h>

#include <SuplaDevice.h>
#include <supla/network/esp_wifi.h>
#include <supla/control/relay.h>
#include <supla/control/button.h>
#include <supla/control/action_trigger.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/network/html/custom_parameter.h>
#include <supla/network/html/custom_text_parameter.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/events.h>
#include <supla/storage/eeprom.h>
#include <supla/control/virtual_relay.h>
#include <supla/sensor/SHT3x.h>
#include <supla/sensor/virtual_therm_hygro_meter.h>
#include <supla/control/hvac_base.h>
#include <supla/control/internal_pin_output.h>

#include "TempDisplay.h"
#include "multi_virtual_relay.h"
#include "xiaomi_therm_hygro_meter.h"
#include "M5RTC.h"

#include "OCRAExtended16.h"

#include "NimBLEDevice.h"
#if !CONFIG_BT_NIMBLE_EXT_ADV
#  error Must enable extended advertising, see nimconfig.h file.
#endif

#include "encoder_pcnt.h"

#define BT_SCAN_INTERVAL 120
#define BT_SCAN_TIME 4

// Supla section

#undef USE_HARDCODED_DATA
#define USE_HARDCODED_DATA

Supla::Eeprom eeprom;


Supla::ESPWifi wifi;
Supla::LittleFsConfig configSupla;

Supla::EspWebServer suplaServer;

Supla::Html::DeviceInfo htmlDeviceInfo(&SuplaDevice);
Supla::Html::WifiParameters htmlWifi;
Supla::Html::ProtocolParameters htmlProto;

Supla::Sensor::SHT3x *SHT31;

Supla::Sensor::ProgDisplay *DPrograms;
Supla::Control::Multi_VirtualRelay *PDstep_mvr;

Supla::Control::HvacBase *M5Dial_hvac;

#define MAX_SENSORS 9
#define THT_OUTPUT_PIN 2

Supla::Sensor::XiaomiThermHygroMeter *xiaomiSensors[MAX_SENSORS];
Supla::Sensor::XiaomiCalcThermHygroMeter *hvac_therm;

int32_t Sensors_cnt = 0;

bool local_mode = true;

long screen_saver_time;
long btScanInterval;
long timer_time;

//end of Supla section

NimBLEScan* pBLEScan;
NimBLEClient *pClient;

#define MENU_ITEMS 10
#define MENU_ANGLE 36
#define MENU_OFFSET 166

const static char* menu_labels1[MENU_ITEMS] PROGMEM = { "TERMOSTAT","TIMER","NASTAWA","AKTYWNY","NASTAWA","NASTAWA","WYBÓR","STATUS","CZUJNIKI","EKRAN"};
const static char* menu_labels2[MENU_ITEMS] PROGMEM = { "- TRYBY","ON/OFF","TEMPERATURY","EKRAN","CZASU","HISTEREZY","PROGRAMU","","","GŁÓWNY"};
const static char* menu_labels3[MENU_ITEMS] PROGMEM = { "","","(°C)","GŁÓWNY","TIMERA","(°C)","","","",""};

const static byte menu_lines[MENU_ITEMS] PROGMEM = {2,2,3,3,3,3,2,1,1,2};

static constexpr const char* const week_days[7] = {"ND", "PN", "WT", "ŚR","CZ", "PT", "SB"};

long nm_old_position = 0;
long nm_new_position = 0;
long nm_position_delta = 0;
long nm_menu_level = 0;
long nm_prev_menu = 0;
long nm_menu_position = 0;
long nm_menu_end = 2;
long nm_menu_max = 7;
bool nm_menu_redraw = true;

bool screen_saver_on = false;

bool nm_config_mode = false;

bool ms_tht_change = true;

time_t tht_timer = 0;

m5::touch_state_t touch_prev_state;

M5Canvas canvas(&M5Dial.Display);

const char XIAOMI_CNT[] = "Xiaomi_cnt";

const static char *XIAOMI_PARAMS[] PROGMEM = {
  "Xiaomi#1",
  "Xiaomi#2",
  "Xiaomi#3",
  "Xiaomi#4",
  "Xiaomi#5",
  "Xiaomi#6",
  "Xiaomi#7",
  "Xiaomi#8",
  "Xiaomi#9" };

const static char *LOCS_PARAMS[] PROGMEM = {
  "loc#1",
  "loc#2",
  "loc#3",
  "loc#4",
  "loc#5",
  "loc#6",
  "loc#7",
  "loc#8",
  "loc#9" };


#ifdef USE_HARDCODED_DATA
#define SUPLA_WIFI_SSID wifi_ssid
#define SUPLA WIFI_PASS wifi_pass
#define SUPLA_SVR supla_svr
#define SUPLA_EMAIL supla_email

const static char *xiaomiBleDeviceMacsTEMP[][2] PROGMEM = {  
{"A4:C1:38:63:D1:8D","lokalizacja 1"},
{"A4:C1:38:19:74:38","lokalizacja 2"},
{"A4:C1:38:9C:6C:1D","lokalizacja 3"},
{"A4:C1:38:9C:27:8E","lokalizacja 4"},
{"A4:C1:38:75:50:F7","lokalizacja 5"},
{"A4:C1:38:CB:AC:94","lokalizacja 6"}
};

#endif //USE_HARDCODED_DATA

char  xiaomiBleDeviceMacs[MAX_SENSORS][18];
char  xiaomiBleDeviceLocs[MAX_SENSORS][32];

const static char *program_names[] PROGMEM = {
"minimum",
"średnia",
"maximum",
"lokalizacja 1",
"lokalizacja 2",
"lokalizacja 3",
"lokalizacja 4",
"lokalizacja 5",
"lokalizacja 6",
"lokalizacja 7",
"lokalizacja 8",
"lokalizacja 9",
"avg - lokalizacja 1",
"avg - lokalizacja 2",
"avg - lokalizacja 3",
"avg - lokalizacja 4",
"avg - lokalizacja 5",
"avg - lokalizacja 6",
"avg - lokalizacja 7",
"avg - lokalizacja 8",
"avg - lokalizacja 9"
};

double sensors_temperature [MAX_SENSORS];
double sensors_humidity [MAX_SENSORS];
byte sensors_battery [MAX_SENSORS];
int sensors_rssi [MAX_SENSORS];
byte sensors_tick [MAX_SENSORS];


std::string strServiceData;

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    if (advertisedDevice->haveName() && advertisedDevice->haveServiceData()) {

      int serviceDataCount = advertisedDevice->getServiceDataCount();
      strServiceData = advertisedDevice->getServiceData(0);

      String macAddress = advertisedDevice->getAddress().toString().c_str();
      macAddress.toUpperCase();
      for (int i = 0; i < Sensors_cnt; i++) {
        if (macAddress == xiaomiBleDeviceMacs[i]) {
          int16_t rawTemperature = (strServiceData[7] | (strServiceData[6] << 8));
          float current_temperature = rawTemperature * 0.1;
          byte current_humidity = (float)(strServiceData[8]);
          byte current_batt = (float)(strServiceData[9]);

          sensors_temperature[i] = current_temperature;
          sensors_humidity[i] = current_humidity;
          sensors_battery[i] = current_batt;
          sensors_rssi[i] = advertisedDevice->getRSSI();
          sensors_tick[i] = 0; 
          break;  
        }
      }
    }
  }
};

String getAllHeap(){
char temp[300];
sprintf(temp, "Heap: Free:%i, Min:%i, Size:%i, Alloc:%i", ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
return temp;
}

Supla::Sensor::DisplayAH *THT_dah;

void setup() {
  
  #ifdef USE_HARDCODED_DATA

  char USER_GUID[SUPLA_GUID_SIZE] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  char USER_AUTH_KEY[SUPLA_AUTHKEY_SIZE] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  
  #endif

  Serial.begin(115200);
 
  Wire.begin();
  
  auto M5cfg = M5.config();
  
  M5Dial.begin(M5cfg, false, false);
  init_encoder();
  
  eeprom.setStateSavePeriod(5000);

  new Supla::Html::CustomParameter(XIAOMI_CNT,"Ilość czujników (1-9)", 0);

  for (int j = 0; j < MAX_SENSORS; j++) {

    new Supla::Html::CustomTextParameter(XIAOMI_PARAMS[j], "Adres czujnika #", 17);
    new Supla::Html::CustomTextParameter(LOCS_PARAMS[j], "lokalizacja czujnika #", 32);
  }

  Supla::Storage::Init();

  auto cfg = Supla::Storage::ConfigInstance();
  
  #ifdef USE_HARDCODED_DATA

  if (cfg) {
    
    char buf[100];
    
    if (!cfg->getGUID(buf)) {
      cfg->setGUID(USER_GUID);
      cfg->setAuthKey(USER_AUTH_KEY);
      cfg->setWiFiSSID(SUPLA_WIFI_SSID);
      cfg->setWiFiPassword(SUPLA_WIFI_PASS);
      cfg->setSuplaServer(SUPLA_SVR);
      cfg->setEmail(SUPLA_EMAIL);
    }

    cfg->getInt32(XIAOMI_CNT, &Sensors_cnt);
    
    if (Sensors_cnt == 0) { 

      Sensors_cnt = 6;
      cfg->setInt32(XIAOMI_CNT, Sensors_cnt);
      for (int j = 0; j < 6; j++) {

        cfg->setString(XIAOMI_PARAMS[j],xiaomiBleDeviceMacsTEMP[j][0]);
        cfg->setString(LOCS_PARAMS[j],xiaomiBleDeviceMacsTEMP[j][1]);
      }
    }
  }

#endif //USE_HARDCODED_DATA

  if (cfg) {

    cfg->getInt32(XIAOMI_CNT, &Sensors_cnt);

    for (int j = 0; j < Sensors_cnt; j++) {
      cfg->getString(XIAOMI_PARAMS[j],xiaomiBleDeviceMacs[j],18);
      cfg->getString(LOCS_PARAMS[j],xiaomiBleDeviceLocs[j],31);
    }
}
  
  for (int i = 0; i < Sensors_cnt; i++) {
    sensors_temperature[i] = -275.0;
    sensors_humidity[i] = -1;
    sensors_battery[i] = 255;
    sensors_rssi[i] = -100;
    sensors_tick[i] = 0;
  }
  
  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
  NimBLEDevice::setScanDuplicateCacheSize(200);
  NimBLEDevice::init("M5Dial");

  pBLEScan = NimBLEDevice::getScan(); //create new scan
  // Set the callback for when devices are discovered, no duplicates.
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true); // Set active scanning, this will get more data from the advertiser.
  pBLEScan->setInterval(97); // How often the scan occurs / switches channels; in milliseconds,
  pBLEScan->setWindow(37);  // How long to scan during the interval; in milliseconds.
  pBLEScan->setMaxResults(0); // do not store the scan results, use callback only.
  
  auto output = new Supla::Control::InternalPinOutput(THT_OUTPUT_PIN);

  M5Dial_hvac = new Supla::Control::HvacBase(output);
  
  SHT31 = new Supla::Sensor::SHT3x(0x44);
  SHT31->setInitialCaption("pomiar lokalny");

  THT_dah = new Supla::Sensor::DisplayAH(&M5Dial, &nm_menu_redraw);

  DPrograms = new Supla::Sensor::ProgDisplay(0,Sensors_cnt, &sensors_temperature[0]);
  DPrograms->setInitialCaption("aktualny program");
  
  PDstep_mvr = new Supla::Control::Multi_VirtualRelay(DPrograms);
  PDstep_mvr->setInitialCaption("zmiana programu");
  PDstep_mvr->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
  PDstep_mvr->addAction(Supla::TURN_ON, THT_dah,Supla::ON_CHANGE);

  hvac_therm = new Supla::Sensor::XiaomiCalcThermHygroMeter(DPrograms, &sensors_temperature[0], &sensors_humidity[0], Sensors_cnt);

  M5Dial_hvac->setMainThermometerChannelNo(4);
  
  for (int i = 0; i < Sensors_cnt; i++) {
    xiaomiSensors[i] = new Supla::Sensor::XiaomiThermHygroMeter(&sensors_temperature[i], &sensors_humidity[i],&sensors_battery[i],&sensors_rssi[i], xiaomiBleDeviceMacs[i]);
    xiaomiSensors[i]->setInitialCaption(xiaomiBleDeviceLocs[i]);
    xiaomiSensors[i]->getChannel()->setFlag(SUPLA_CHANNEL_FLAG_CHANNELSTATE);
  }

  M5Dial_hvac->addAction(Supla::TURN_ON, THT_dah,Supla::ON_HVAC_MODE_OFF);
  M5Dial_hvac->addAction(Supla::TURN_ON, THT_dah,Supla::ON_HVAC_MODE_HEAT);
  M5Dial_hvac->addAction(Supla::TURN_ON, THT_dah,Supla::ON_HVAC_WEEKLY_SCHEDULE_ENABLED);
  M5Dial_hvac->addAction(Supla::TURN_ON, THT_dah,Supla::ON_HVAC_WEEKLY_SCHEDULE_DISABLED);
  M5Dial_hvac->addAction(Supla::TURN_ON, THT_dah,Supla::ON_HVAC_STANDBY);
  M5Dial_hvac->addAction(Supla::TURN_ON, THT_dah,Supla::ON_HVAC_HEATING);

  new Supla::M5RTC(&M5Dial.Rtc);
  
  SuplaDevice.setStatusFuncImpl(&status_func);

  SuplaDevice.setSuplaCACert(suplaCACert);
  SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);
  
  SuplaDevice.setName("M5dial Supla test");
  //wifi.enableSSL(true);

  SuplaDevice.setActivityTimeout(240);

  SuplaDevice.begin();
  
  //M5Dial.Rtc.setDateTime( { { 2024, 05, 05 }, { 20, 30, 00 } } );
  M5Dial.Display.begin();
  
  M5Dial.Display.setColorDepth(8);
  canvas.setColorDepth(8);
  canvas.createSprite(M5Dial.Display.width(),M5Dial.Display.height());
  
  canvas.fillScreen(TFT_NAVY);
  canvas.setTextDatum(middle_center);
  
  canvas.loadFont(SegoeUISemiBold20);
    
  canvas.setTextSize(1);
  canvas.pushSprite(0,0);

  screen_saver_time = millis();
  btScanInterval = millis();
  timer_time = millis();
}



void status_func(int status, const char *msg) {

  if (status != STATUS_REGISTERED_AND_READY) local_mode = true;
  else local_mode = false;
  if (status == STATUS_REGISTER_IN_PROGRESS) screen_saver_on = false;
}


void loop() {

  SuplaDevice.iterate();
  
  M5Dial.update();

  for (int i = 0; i < Sensors_cnt; i++)
    if (sensors_tick[i] > 5) {
      sensors_temperature[i] = -275;
      sensors_humidity[i] = -1;
      sensors_battery[i] = 255;
      sensors_rssi[i] = -100;
      sensors_tick[i] = 0;
    }
  

  if ((millis() - btScanInterval) > (BT_SCAN_INTERVAL * 1000)) {
     
    btScanInterval = millis();

    for (int i = 0; i < Sensors_cnt; i++) {
      xiaomiSensors[i]->setTemp(sensors_temperature[i]);
      xiaomiSensors[i]->setHumi(sensors_humidity[i]);
      sensors_tick[i] = sensors_tick[i] + 1;
    }

    nm_menu_redraw = true;
  }

  if (M5Dial_hvac->isCountdownEnabled()&&((millis() - timer_time) > 1000)) {

    timer_time = millis();
    nm_menu_redraw = true;
  }
  
//BLE section
if(pBLEScan->isScanning() == false) {
      // Start scan with: duration = 0 seconds(forever), no scan end callback, not a continuation of a previous scan.
    pBLEScan->start(0, nullptr, false);}

//end of BLE section

  //Serial.println(getAllHeap());
  
  //double curTemp = SHT31->getTemp();
  //double curHumi = SHT31->getHumi();
  //Serial.println(curTemp);

  auto t = M5Dial.Touch.getDetail();
  
  if (touch_prev_state != t.state){
    touch_prev_state = t.state;
    
    //if ((t.state == m5::touch_state_t::touch_end)||(t.state == m5::touch_state_t::touch_end)){
      
      screen_saver_time = millis();
      
      if (screen_saver_on) {
        screen_saver_on = false;
        nm_menu_redraw = true;
      }
    //}
  }

  if (!screen_saver_on) {
    if (millis()-screen_saver_time > 30000) screen_saver_on = true;
    }

  if (M5Dial.BtnA.pressedFor(5000)) {

      screen_saver_on = false;
      nm_config_mode = true;
      nm_menu_redraw = true;
      SuplaDevice.enterConfigMode();
    }

  if (M5Dial.BtnA.pressedFor(10000)) {

      SuplaDevice.softRestart();
    }

  if (!screen_saver_on){
    
    if (M5Dial.BtnA.wasPressed()){

      screen_saver_on = false;
      screen_saver_time = millis();

      if (nm_menu_level == nm_menu_end){

          nm_menu_level--; 
          nm_menu_position = nm_prev_menu;
          nm_old_position = 0;
          reset_encoder();
          nm_position_delta = 0;
          nm_menu_redraw = true;
      }
      else {
          nm_menu_level++;
          nm_prev_menu = nm_menu_position;
          nm_menu_position = 0;
          nm_old_position = 0;
          reset_encoder();
          nm_position_delta = 0;
        nm_menu_redraw = true;
      }
    }

    nm_new_position = get_encoder()/4; 

    nm_position_delta = nm_new_position - nm_old_position;

    if (nm_position_delta != 0){
    
      screen_saver_on = false;
      screen_saver_time = millis();
      
      nm_old_position = nm_new_position;
      nm_new_position = nm_new_position + nm_position_delta;
    
      nm_menu_position = nm_menu_position + nm_position_delta;
      
      if (nm_menu_position < 0) nm_menu_position = nm_menu_max;
      if (nm_menu_position > nm_menu_max) nm_menu_position = 0;
      
      nm_menu_redraw = true;  
    }

    if (nm_config_mode) {
      
      canvas.fillScreen(TFT_NAVY);
      canvas.setTextDatum(middle_center);
      canvas.setTextSize(1);
      canvas.loadFont(SegoeUISemiBold20);
      canvas.drawString(F("SuplaDevice config mode "),120,100);
      canvas.drawString(F("http://192.168.4.1/"),120,140);
      canvas.pushSprite(0,0);
    }
    else
    if (nm_menu_redraw){
      
      nm_menu_redraw = false;
         
      switch (nm_menu_level){
        case 0:
          //nm_menu_max =
          if ((ms_tht_change) && (nm_position_delta != 0)) {
            M5Dial_hvac->handleAction(0,Supla::TOGGLE_OFF_MANUAL_WEEKLY_SCHEDULE_MODES);
            M5Dial.Speaker.tone(8000, 20);
            }
          drawMenu0();
          break;
        case 1:
          nm_menu_max = MENU_ITEMS - 1;
          M5Dial.Speaker.tone(8000, 20);
          drawMenu1();
          break;
      case 2:
          M5Dial.Speaker.tone(8000, 20);
          drawMenu2(nm_prev_menu);
          break;    
      }
    }
  }  
  else {
    canvas.fillScreen(TFT_BLACK);
    canvas.pushSprite(0,0);}
}  

void drawFlameIcon(int32_t start_x, int32_t start_y, int outer_color, int inner_color){
  
  canvas.drawBezier(start_x,start_y,start_x-20,start_y-10,start_x-10,start_y-30,start_x,start_y-40,outer_color);
  canvas.drawBezier(start_x,start_y,start_x+20,start_y-10,start_x+10,start_y-30,start_x,start_y-40,outer_color);
  canvas.drawBezier(start_x,start_y,start_x-10,start_y-10,start_x-5,start_y-15,start_x,start_y-25,inner_color);
  canvas.drawBezier(start_x,start_y,start_x+10,start_y-10,start_x+5,start_y-15,start_x,start_y-25,inner_color);
  canvas.floodFill(start_x,start_y-35,outer_color);
  canvas.floodFill(start_x,start_y-10,inner_color);
}

void drawCalendarIcon(int32_t start_x, int32_t start_y, int outer_color, int inner_color){
  
  canvas.fillRect(start_x,start_y,40,40,outer_color);
  canvas.fillRect(start_x + 2,start_y + 2,36,36,inner_color);
  canvas.drawRect(start_x,start_y,40,10,outer_color);
  for (int i = 1; i < 8; i++) canvas.drawPixel(start_x + (i * 5), start_y + 10, outer_color);
  for (int i = 1; i < 8; i++) canvas.drawPixel(start_x + (i * 5), start_y + 20, outer_color);
  for (int i = 1; i < 8; i++) canvas.drawPixel(start_x + (i * 5), start_y + 30, outer_color);
}

void drawWrenchIcon(int32_t start_x, int32_t start_y, int outer_color, int inner_color){
  
  canvas.fillCircle(start_x, start_y,10, outer_color);
  canvas.fillRect(start_x - 5,start_y - 10,10,10,inner_color);
  canvas.fillRect(start_x - 5,start_y,10,30,outer_color);
}

void drawDisabledIcon(int32_t start_x, int32_t start_y, int outer_color, int inner_color){
  
  canvas.fillCircle(start_x, start_y,20, outer_color);
  canvas.fillRect(start_x - 10,start_y - 5, 20,10, inner_color);
}

void drawMenu0(){

      char tempstr_fmt[25];
      
      canvas.clearClipRect();
      canvas.setTextColor(TFT_WHITE);
      canvas.fillScreen(TFT_NAVY);  
      canvas.setTextSize(1);
      canvas.fillRoundRect(90,70,60,60,10, lgfx::v1::color332(0,255,0));
      canvas.fillRoundRect(95,75,50,50,10, lgfx::v1::color332(128,128,128));
      
      if (M5Dial_hvac->getChannel()->isHvacFlagHeating()) drawFlameIcon(120,120,TFT_RED, TFT_YELLOW);
      else drawFlameIcon(120,120,TFT_LIGHTGREY, TFT_DARKGREY);

      if (M5Dial_hvac->getPrimaryTemp() > INT16_MIN)
        canvas.drawString(String(((double)M5Dial_hvac->getPrimaryTemp())/100,1)+"°C",40, 100);
      else
        canvas.drawString("--- °C",40, 100);
      canvas.drawString(String(((double)M5Dial_hvac->getTemperatureSetpointHeat())/100,1)+"°C",200, 100);

      canvas.setTextColor(TFT_GREEN);
      if (M5Dial_hvac->isWeeklyScheduleEnabled()) drawCalendarIcon(100,160, TFT_GREEN, TFT_NAVY);
      if (M5Dial_hvac->isManualModeEnabled()) drawWrenchIcon(120,160,TFT_GREEN, TFT_NAVY);
      if (M5Dial_hvac->isCountdownEnabled()) canvas.drawString("timer : " +String(M5Dial_hvac->getCountDownTimerEnds()-Supla::Clock::GetTimeStamp()),120,160);

      if (M5Dial_hvac->isThermostatDisabled()) drawDisabledIcon(120,180,TFT_RED,TFT_WHITE);

      canvas.setTextColor(TFT_WHITE);

      auto dt = M5Dial.Rtc.getDateTime();
      sprintf (tempstr_fmt,"%02d.%02d.%04d(%s)",dt.date.date,dt.date.month,dt.date.year,week_days[dt.date.weekDay]);
      canvas.loadFont(SegoeUI16);
      canvas.drawString(tempstr_fmt, 120,45);
      sprintf (tempstr_fmt,"%02d:%02d", dt.time.hours, dt.time.minutes);
      canvas.drawString(tempstr_fmt, 120,20);
      canvas.loadFont(SegoeUISemiBold20);

      canvas.setTextSize(1);
      canvas.pushSprite(0,0);
}

void drawMenu1(){

      canvas.fillScreen(TFT_NAVY);
      canvas.fillArc(120, 120, 100, 120, nm_menu_position*MENU_ANGLE + MENU_OFFSET, nm_menu_position*MENU_ANGLE + MENU_ANGLE + MENU_OFFSET, TFT_GREEN);// TFT_VIOLET);
      canvas.fillArc(120,120,90,99,0,360,TFT_GREEN); //TFT_VIOLET);
      canvas.setTextColor(TFT_WHITE);
      canvas.fillCircle(120, 120, 89, TFT_NAVY);
      switch (menu_lines[nm_menu_position]) {
        case 1:
          canvas.drawString(menu_labels1[nm_menu_position], 120, 120);
        break;
        case 2:
          canvas.drawString(menu_labels1[nm_menu_position], 120, 100);
          canvas.drawString(menu_labels2[nm_menu_position], 120, 140);
        break;
        case 3:
          canvas.drawString(menu_labels1[nm_menu_position], 120, 80);
          canvas.drawString(menu_labels2[nm_menu_position], 120, 120);
          canvas.drawString(menu_labels3[nm_menu_position], 120, 160);
        break;
      }
      
      canvas.pushSprite(0,0);
}

void drawMenu2(long selector){

      canvas.setTextColor(TFT_WHITE); 
      canvas.fillCircle(120,120,100,TFT_NAVY); 
      
      canvas.drawString(menu_labels1[selector], 120, 80);     
        switch (selector){
        case 0:
          nm_menu_max = 2;
          if (nm_position_delta !=0) M5Dial_hvac->handleAction(0,Supla::TOGGLE_OFF_MANUAL_WEEKLY_SCHEDULE_MODES); 
          nm_drawTMGauge();
          break;
        case 1:
          nm_menu_max = 1;
          if (nm_position_delta !=0) 
            if (M5Dial_hvac->isCountdownEnabled()) M5Dial_hvac->applyNewRuntimeSettings(SUPLA_HVAC_MODE_NOT_SET, 0);
            else M5Dial_hvac->applyNewRuntimeSettings(SUPLA_HVAC_MODE_NOT_SET, tht_timer*60);
          nm_drawOnOffGauge(M5Dial_hvac->isCountdownEnabled());
          break;
        case 2:
          nm_menu_max = 1;
          if (nm_position_delta > 0) M5Dial_hvac->handleAction(0, Supla::INCREASE_HEATING_TEMPERATURE);
          if (nm_position_delta < 0) M5Dial_hvac->handleAction(0, Supla::DECREASE_HEATING_TEMPERATURE);
          nm_drawTempScaleGauge("TEMPERATURA",M5Dial_hvac->getTemperatureSetpointHeat(), M5Dial_hvac->getDefaultTemperatureRoomMin(),M5Dial_hvac->getDefaultTemperatureRoomMax());
          break;  
        case 3:
          nm_menu_max = 1;
          if (nm_position_delta !=0) ms_tht_change = !ms_tht_change;
          nm_drawOnOffGauge(ms_tht_change);
          break;
        case 4:
          nm_menu_max  = 0;
          if (nm_position_delta > 0) tht_timer += 15;
          if (nm_position_delta < 0) tht_timer -= 15;
          if (tht_timer < 0) tht_timer = 0;
          nm_drawScaleGauge("TIMER", "minuty", tht_timer,0,1440);
          break;
        case 5:
          if (nm_position_delta > 0) M5Dial_hvac->setTemperatureHisteresis(M5Dial_hvac->getTemperatureHisteresis() + 10);
          if (nm_position_delta < 0) M5Dial_hvac->setTemperatureHisteresis(M5Dial_hvac->getTemperatureHisteresis() - 10);
          nm_drawTempScaleGauge("HISTEREZA",M5Dial_hvac->getTemperatureHisteresis(),M5Dial_hvac->getTemperatureHisteresisMin(), M5Dial_hvac->getTemperatureHisteresisMax());
          break;
        case 6:
          nm_menu_max = DPrograms->getPCount() - 1;
          if (nm_position_delta !=0) DPrograms->setDP(nm_menu_position);
          else nm_menu_position = DPrograms->getDP();
          nm_drawPrograms(DPrograms);
          break;
        case 7:
          nm_menu_max = 1;
          nm_drawStatus();
          break;
        case 8:
          nm_menu_max = Sensors_cnt-1;
          nm_drawSensors();
          break;
        case 9:
          nm_menu_level = 0;
          nm_menu_redraw = true;
          break;
        default:
          ;
      }
      canvas.pushSprite(0,0);
}

void nm_drawTMGauge() {

      
    canvas.loadFont(SegoeUISemiBold20);

    if (M5Dial_hvac->isThermostatDisabled()) {
        
      canvas.fillArc(120, 120, 100, 120, 0, 360, TFT_RED); 
      canvas.drawString(F("OFF"), 120, 140);
    }
    else
    if (M5Dial_hvac->isManualModeEnabled()) {

      canvas.fillArc(120, 120, 100, 120, 0, 360, TFT_GREEN);
      canvas.drawString(F("TRYB RĘCZNY"), 120, 140);
    }
    else
    if (M5Dial_hvac->isWeeklyScheduleEnabled()) {

      canvas.fillArc(120, 120, 100, 120, 0, 360, TFT_GREEN);
      canvas.drawString(F("TRYB TYGODNIOWY"), 120, 140);
    }

}

void nm_drawOnOffGauge(bool isOn){

      
      canvas.loadFont(SegoeUISemiBold20);

     if (isOn) {
        canvas.fillArc(120, 120, 100, 120, 0, 360, TFT_GREEN);
        canvas.drawString(F("ON"), 120, 140);}
      else {
        canvas.fillArc(120, 120, 100, 120, 0, 360, TFT_RED); 
        canvas.drawString(F("OFF"), 120, 140);}
}

void nm_drawTempScaleGauge (char* g_title, _supla_int16_t t_value, _supla_int16_t min_value, _supla_int16_t max_value) {

    
      double range = (max_value - min_value);

      double vscale = (t_value - min_value)/ range;

      int arc = int(vscale*180);

      canvas.fillCircle(120, 120, 120, TFT_NAVY);
      canvas.fillArc(120, 120, 100, 120, 180, 180+arc, TFT_RED);
      canvas.fillArc(120, 120, 100, 120, 180+arc, 360, TFT_GREEN);
      canvas.loadFont(SegoeUI16); //canvas.setTextSize(0.5);
      canvas.drawString(String(min_value/100)+"°C", 20, 140);
      canvas.drawString(String(max_value/100)+"°C", 220, 140);
      canvas.loadFont(SegoeUISemiBold20);//canvas.setTextSize(1);
      canvas.drawString(String(((double)t_value)/100,1)+"°C", 120, 140);
      canvas.drawString(g_title, 120, 80);
}

void nm_drawScaleGauge (char *g_utitle,char *g_dtitle, _supla_int16_t t_value, _supla_int16_t min_value, _supla_int16_t max_value) {

    
      double range = (max_value - min_value);

      double vscale = (t_value - min_value)/ range;

      int arc = int(vscale*180);

      canvas.fillCircle(120, 120, 120, TFT_NAVY);
      canvas.fillArc(120, 120, 100, 120, 180, 180+arc, TFT_RED);
      canvas.fillArc(120, 120, 100, 120, 180+arc, 360, TFT_GREEN);
      canvas.loadFont(SegoeUI16); //canvas.setTextSize(0.5);
      canvas.drawNumber(min_value, 20, 140);
      canvas.drawNumber(max_value, 220, 140);
      canvas.loadFont(SegoeUISemiBold20);//canvas.setTextSize(1);
      canvas.drawNumber(t_value, 120, 140);
      canvas.drawString(g_utitle, 120, 80);
      canvas.drawString(g_dtitle, 120, 180);
}

void nm_drawStatus(){
  
  canvas.fillScreen(TFT_NAVY);

  switch (nm_menu_position){
  case 0:
  if (wifi.isReady()) {
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.loadFont(SegoeUISemiBold20);
    canvas.drawString(F("Wi-Fi"),120,40);
    canvas.drawString(F("connected"),120,80);
    canvas.drawString(WiFi.SSID(),120,120);
    }
  else {
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.loadFont(SegoeUISemiBold20);
    canvas.drawString(F("no Wi-Fi "),120,100);
    canvas.drawString(F("connection "),120,140);
    };
    break;
  case  1:
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.loadFont(SegoeUISemiBold20);
     canvas.drawString(F("LOCAL MODE:"),120,100);
     if (local_mode) canvas.drawString(F("ON"),120,140);
     else canvas.drawString(F("OFF"),120,140);
     break;
    
  }
}

void nm_drawSensors(){
  
  canvas.fillScreen(TFT_NAVY); 
  canvas.setTextDatum(middle_center);
  canvas.loadFont(SegoeUI16); //canvas.setTextSize(0.7);
  canvas.drawString(xiaomiBleDeviceMacs[nm_menu_position],120,170);
  canvas.drawString(xiaomiBleDeviceLocs[nm_menu_position],120,200);
  canvas.drawString(String(sensors_temperature[nm_menu_position]),120,20);
  canvas.drawString(String(sensors_humidity[nm_menu_position]),120,50);
  canvas.drawString(String(sensors_battery[nm_menu_position]),120,80);
  canvas.drawString(String(sensors_rssi[nm_menu_position]),120,110);
  canvas.drawString(String(sensors_tick[nm_menu_position]),120,140);
  canvas.setTextSize(1);
  canvas.loadFont(SegoeUISemiBold20);

}

void nm_drawPrograms(Supla::Sensor::ProgDisplay *pdisplay){

  canvas.fillScreen(TFT_NAVY); 
  canvas.setTextDatum(middle_center);
  canvas.setTextSize(1);
  canvas.loadFont(SegoeUISemiBold20);
  canvas.drawString(String("program"), 120,80);
  canvas.drawString(String(pdisplay->getValue()), 120,160);
  if (pdisplay->getDP() > Sensors_cnt+2)
    canvas.drawString(program_names[12+ pdisplay->getDP() - (Sensors_cnt + 3)], 120,120);
  else
    canvas.drawString(program_names[pdisplay->getDP()], 120,120);

  }
