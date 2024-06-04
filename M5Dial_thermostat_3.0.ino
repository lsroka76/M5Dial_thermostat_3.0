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
#include <supla/network/html/time_parameters.h>
#include <supla/network/html/hvac_parameters.h>
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
Supla::LittleFsConfig configSupla(2048);

Supla::EspWebServer suplaServer;

Supla::Html::DeviceInfo htmlDeviceInfo(&SuplaDevice);
Supla::Html::WifiParameters htmlWifi;
Supla::Html::ProtocolParameters htmlProto;
Supla::Html::TimeParameters htmlTimeParameters(&SuplaDevice);
Supla::Html::HvacParameters htmlHvacParameters;

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

#define MENU_1_ITEMS 10
#define MENU_1_ANGLE 36
#define MENU_1_OFFSET 166

#define MENU_11_ITEMS 5
#define MENU_11_ANGLE 72
#define MENU_11_OFFSET 166
#define MENU_11_POS 5 

#define MENU_12_ITEMS 4
#define MENU_12_ANGLE 90
#define MENU_12_OFFSET 166
#define MENU_12_POS 1

const static char menu_1_labels_1[MENU_1_ITEMS][12] PROGMEM = { "TERMOSTAT","TIMER","NASTAWA","AKTYWNY","NASTAWA","NASTAWA","WYBÓR","STATUS","CZUJNIKI","EKRAN"};
const static char menu_1_labels_2[MENU_1_ITEMS][12] PROGMEM = { "TRYB","USTAWIENIA","TEMPERATURY","EKRAN","HISTEREZY","PROGRAMÓW","TERMOMETRU","","","GŁÓWNY"};
const static char menu_1_labels_3[MENU_1_ITEMS][12] PROGMEM = { "PRACY","","(°C)","GŁÓWNY","(°C)","TYGODNIA","","",""};

const static byte menu_1_lines[MENU_1_ITEMS] PROGMEM = {3,2,3,3,3,3,2,1,1,2};

const static char menu_11_labels_1[MENU_11_ITEMS][12] PROGMEM = { "PROGRAM 1","PROGRAM 2","PROGRAM 3","PROGRAM 4","POPRZEDNIE"};
const static char menu_11_labels_2[MENU_11_ITEMS][12] PROGMEM = { "TEMPERATURA","TEMPERATURA","TEMPERATURA","TEMPERATURA","MENU"};
const static char menu_11_labels_3[MENU_11_ITEMS][12] PROGMEM = { "","","","",""};

const static byte menu_11_lines[MENU_11_ITEMS] PROGMEM = {2,2,2,2,2};


const static char menu_12_labels_1[MENU_12_ITEMS][12] PROGMEM = { "TIMER","NASTAWA","NASTAWA","POPRZEDNIE"};
const static char menu_12_labels_2[MENU_12_ITEMS][12] PROGMEM = { "ON/OFF","CZASU","TEMPERATURY","MENU"};
const static char menu_12_labels_3[MENU_12_ITEMS][12] PROGMEM = { "","TIMERA","TIMERA",""};

const static byte menu_12_lines[MENU_12_ITEMS] PROGMEM = {2,3,3,2};

static constexpr const char* const week_days[7] = {"ND", "PN", "WT", "ŚR","CZ", "PT", "SB"};

long nm_old_position = 0;
long nm_new_position = 0;
long nm_position_delta = 0;
long nm_menu_level = 0;
long nm_prev_menu[4] = {0,0,0,0};
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

const static char HVAC_T_TIME [] PROGMEM = "Hvac_T_time";
const static char HVAC_T_TEMP [] PROGMEM = "Hvac_T_temp";

#ifdef USE_HARDCODED_DATA

#include "hcd/hcd_data.h"
  
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

      Sensors_cnt = HCD_SENSORS;
      
      cfg->setInt32(XIAOMI_CNT, Sensors_cnt);
      
      for (int j = 0; j < HCD_SENSORS; j++) {

        cfg->setString(XIAOMI_PARAMS[j],xiaomiBleDeviceMacsTEMP[j][0]);
        cfg->setString(LOCS_PARAMS[j],xiaomiBleDeviceMacsTEMP[j][1]);
      }
    }
  }

#endif //USE_HARDCODED_DATA

  if (cfg) {

    if(!cfg->getInt32(XIAOMI_CNT, &Sensors_cnt)) Sensors_cnt = 0;

    for (int j = 0; j < Sensors_cnt; j++) {
      cfg->getString(XIAOMI_PARAMS[j],xiaomiBleDeviceMacs[j],18);
      cfg->getString(LOCS_PARAMS[j],xiaomiBleDeviceLocs[j],31);
    }

    uint32_t t_timer;

    if (cfg->getUInt32(HVAC_T_TIME, &t_timer)) tht_timer = (time_t)t_timer;
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

  htmlHvacParameters.setHvacPtr(M5Dial_hvac);

  SHT31 = new Supla::Sensor::SHT3x(0x44);
  SHT31->setInitialCaption("pomiar lokalny");

  THT_dah = new Supla::Sensor::DisplayAH(&M5Dial, &nm_menu_redraw);

  DPrograms = new Supla::Sensor::ProgDisplay(0,Sensors_cnt, &sensors_temperature[0]);
  DPrograms->setInitialCaption("aktualny program");
  
  PDstep_mvr = new Supla::Control::Multi_VirtualRelay(DPrograms);
  PDstep_mvr->setInitialCaption("zmiana programu");
  PDstep_mvr->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
  PDstep_mvr->addAction(Supla::TURN_ON, THT_dah,Supla::ON_CHANGE);

  hvac_therm = new Supla::Sensor::XiaomiCalcThermHygroMeter(DPrograms, &sensors_temperature[0], &sensors_humidity[0], Sensors_cnt, SHT31);

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
          nm_menu_position = nm_prev_menu[nm_menu_level];
          nm_old_position = 0;
          reset_encoder();
          nm_position_delta = 0;
          nm_menu_redraw = true;
      }
      else {
          nm_prev_menu[nm_menu_level] = nm_menu_position;
          nm_menu_position = 0;
          nm_old_position = 0;
          nm_menu_level++;
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
          nm_menu_max = MENU_1_ITEMS - 1;
          M5Dial.Speaker.tone(8000, 20);
          drawMenu1(MENU_1_ANGLE, MENU_1_OFFSET, menu_1_lines, menu_1_labels_1[0], menu_1_labels_2[0], menu_1_labels_3[0]);
          break;
      case 2:
          M5Dial.Speaker.tone(8000, 20);
          if (nm_prev_menu[1] == 5) {
            nm_menu_end = 3;
            nm_menu_max = MENU_11_ITEMS - 1;
            drawMenu1(MENU_11_ANGLE, MENU_11_OFFSET, menu_11_lines, menu_11_labels_1[0], menu_11_labels_2[0], menu_11_labels_3[0]);
          }
          else
          if (nm_prev_menu[1] == 1) {
            nm_menu_end = 3;
            nm_menu_max = MENU_12_ITEMS - 1;
            drawMenu1(MENU_12_ANGLE, MENU_12_OFFSET, menu_12_lines, menu_12_labels_1[0], menu_12_labels_2[0], menu_12_labels_3[0]);
          }
          else drawMenu2(nm_prev_menu[1]);
          break;  
      case 3:
          M5Dial.Speaker.tone(8000, 20);
          if (nm_prev_menu[1] == MENU_11_POS) drawMenu3(nm_prev_menu[2]);
          else
          if (nm_prev_menu[1] == MENU_12_POS) drawMenu31(nm_prev_menu[2]);
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

void drawMenu1(int menu_angle, int menu_offset,const byte *menu_lines, const char *menu_labels_1, const char *menu_labels_2, const char *menu_labels_3){

      canvas.fillScreen(TFT_NAVY);

      int start_angle = nm_menu_position*menu_angle + menu_offset;
      int end_angle = start_angle + menu_angle;

      canvas.fillArc(120, 120, 100, 120, start_angle, end_angle, TFT_GREEN);
      canvas.fillArc(120,120,90,99,0,360,TFT_GREEN); 
      canvas.setTextColor(TFT_WHITE);
      canvas.fillCircle(120, 120, 89, TFT_NAVY);
      switch (*(menu_lines + nm_menu_position)) {
        case 1:
          canvas.drawString((menu_labels_1 + (12 * nm_menu_position)), 120, 120);
        break;
        case 2:
          canvas.drawString((menu_labels_1 + (12 * nm_menu_position)), 120, 100);
          canvas.drawString((menu_labels_2 + (12 * nm_menu_position)), 120, 140);
        break;
        case 3:
          canvas.drawString((menu_labels_1 + (12 * nm_menu_position)), 120, 80);
          canvas.drawString((menu_labels_2 + (12 * nm_menu_position)), 120, 120);
          canvas.drawString((menu_labels_3 + (12 * nm_menu_position)), 120, 160);
        break;
      }
      
      canvas.pushSprite(0,0);
}

void drawMenu2(long selector){

      canvas.setTextColor(TFT_WHITE); 
      canvas.fillCircle(120,120,100,TFT_NAVY); 
      
      canvas.drawString(menu_1_labels_1[selector], 120, 80);     
        switch (selector){
        case 0:
          nm_menu_max = 2;
          if (nm_position_delta !=0) M5Dial_hvac->handleAction(0,Supla::TOGGLE_OFF_MANUAL_WEEKLY_SCHEDULE_MODES); 
          nm_drawTMGauge();
          break;
        case 1:
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
          break;
      }
      canvas.pushSprite(0,0);
}

void drawMenu3(long selector){

      canvas.setTextColor(TFT_WHITE); 
      canvas.fillCircle(120,120,100,TFT_NAVY); 
      
      canvas.drawString(menu_11_labels_1[selector], 120, 80);     
        switch (selector){
        case 0:
          nm_drawProgramTempGauge(menu_11_labels_1[selector],1);
          break;
        case 1:
          nm_drawProgramTempGauge(menu_11_labels_1[selector],2);
          break;
        case 2:
          nm_drawProgramTempGauge(menu_11_labels_1[selector],3);
           break;  
        case 3:
          nm_drawProgramTempGauge(menu_11_labels_1[selector],4);
          break;
        case 4:
          nm_menu_end = 2;
          nm_menu_level = 1;
          nm_menu_position = nm_prev_menu[1];
          nm_menu_redraw = true;
          break;
        default:
          break;
      }
      canvas.pushSprite(0,0);
}

void drawMenu31(long selector){

      auto cfg = Supla::Storage::ConfigInstance();

      canvas.setTextColor(TFT_WHITE); 
      canvas.fillCircle(120,120,100,TFT_NAVY); 
      
      canvas.drawString(menu_12_labels_1[selector], 120, 80);     
        switch (selector){
        case 0:
          nm_menu_max = 1;
          if (nm_position_delta !=0) 
            if (M5Dial_hvac->isCountdownEnabled()) M5Dial_hvac->applyNewRuntimeSettings(SUPLA_HVAC_MODE_NOT_SET, 0);
            else M5Dial_hvac->applyNewRuntimeSettings(SUPLA_HVAC_MODE_NOT_SET, tht_timer * 60);
          nm_drawOnOffGauge(M5Dial_hvac->isCountdownEnabled());
          break;
        case 1:
          nm_menu_max  = 0;
          
          if (nm_position_delta > 0) tht_timer += 15;
          if (nm_position_delta < 0) tht_timer -= 15;
          
          if (tht_timer < 0) tht_timer = 0;
          
          if (cfg && (nm_position_delta != 0))
              cfg->setUInt32(HVAC_T_TIME, tht_timer);

          nm_drawScaleGauge("TIMER", "minuty", tht_timer, 0, 1440);
          break;
        case 2:
           break;  
        case 3:
          nm_menu_end = 2;
          nm_menu_level = 1;
          nm_menu_position = nm_prev_menu[1];
          nm_menu_redraw = true;
          break;
        default:
          break;
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

void nm_drawTempScaleGauge (const char* g_title, _supla_int16_t t_value, _supla_int16_t min_value, _supla_int16_t max_value) {

    
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
    canvas.drawString(program_names[12 + pdisplay->getDP() - (Sensors_cnt + 3)], 120,120);
  else
    canvas.drawString(program_names[pdisplay->getDP()], 120,120);

  }

void nm_drawProgramTempGauge(const char *g_title, int program_id) {

  TWeeklyScheduleProgram program = M5Dial_hvac->getProgramById(program_id,false); 
  
  if (nm_position_delta > 0) M5Dial_hvac->setProgram(program_id, program.Mode, program.SetpointTemperatureHeat + 50,false);
  if (nm_position_delta < 0) M5Dial_hvac->setProgram(program_id, program.Mode, program.SetpointTemperatureHeat - 50,false);
  if (nm_position_delta != 0) program = M5Dial_hvac->getProgramById(program_id,false);
  
  nm_drawTempScaleGauge(g_title,program.SetpointTemperatureHeat,M5Dial_hvac->getDefaultTemperatureRoomMin(),M5Dial_hvac->getDefaultTemperatureRoomMax());
          
}