/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef SRC_SUPLA_SENSOR_XIAOMI_THERM_HYGRO_METER_H_
#define SRC_SUPLA_SENSOR_XIAOMI_THERM_HYGRO_METER_H_

#include <supla/sensor/therm_hygro_meter.h>

#include "TempDisplay.h"

namespace Supla {
namespace Sensor {

class XiaomiThermHygroMeter : public Supla::Sensor::ThermHygroMeter {
 public:
  XiaomiThermHygroMeter (double *sens_temp, double *sens_humi, byte* sens_batt, int *sens_rssi, const char *sens_mac) {
    xiaomi_temperature = sens_temp;
    xiaomi_humidity = sens_humi;    
    xiaomi_battery = sens_batt;
    xiaomi_rssi = sens_rssi;
    xiaomi_mac = sens_mac;
  }
  
  double getTemp() override {
    return *(xiaomi_temperature);
  }

  double getHumi() override {
    return *(xiaomi_humidity);
  }
  
  byte getBatt()  {
    return *(xiaomi_battery);
  }

  void setTemp(double val) {
    temperature = val;
  }

  void setHumi(double val) {
    humidity = val;
  }

  void iterateAlways() override {
    if (millis() - lastReadTime > 120000) {
      lastReadTime = millis();
      channel.setNewValue(getTemp(), getHumi());
      //channel.setBatteryLevel(getBatt());
    }
  }

  void onInit() override {
    channel.setNewValue(*xiaomi_temperature, *xiaomi_humidity);
    channel.setBatteryLevel(*xiaomi_battery);
  }

  void handleGetChannelState(TDSC_ChannelState *channelState) override {

    channelState->Fields = channelState->Fields | SUPLA_CHANNELSTATE_FIELD_MAC |
                          SUPLA_CHANNELSTATE_FIELD_BATTERYLEVEL |
                          SUPLA_CHANNELSTATE_FIELD_BATTERYPOWERED |
                          SUPLA_CHANNELSTATE_FIELD_WIFIRSSI |
                          SUPLA_CHANNELSTATE_FIELD_WIFISIGNALSTRENGTH;
                           
    channelState->BatteryLevel = *xiaomi_battery;
    channelState->BatteryPowered = true;
    channelState->WiFiRSSI = *xiaomi_rssi;

    char temp_mac[3];
    temp_mac[2] = '\0';

    for (int i = 0; i < 6; i++){
    
      temp_mac[0] = xiaomi_mac[i*3];
      temp_mac[1] = xiaomi_mac[i*3+1];
      channelState->MAC[i] = strtoul(temp_mac,nullptr,16);
    }
    if (*xiaomi_rssi > -50) {
      channelState->WiFiSignalStrength = 100;
    } else if (*xiaomi_rssi <= -100) {
      channelState->WiFiSignalStrength = 0;
    } else {
      channelState->WiFiSignalStrength = 2 * (*xiaomi_rssi + 100);
    }
  }

 protected:
  double temperature = TEMPERATURE_NOT_AVAILABLE;
  double humidity = HUMIDITY_NOT_AVAILABLE;
  double *xiaomi_temperature = NULL;
  double *xiaomi_humidity = NULL;
  byte *xiaomi_battery = NULL;
  int *xiaomi_rssi = NULL;
  const char *xiaomi_mac = NULL;

};

class XiaomiCalcThermHygroMeter : public Supla::Sensor::ThermHygroMeter {
 public:
  XiaomiCalcThermHygroMeter (Supla::Sensor::ProgDisplay *PD, double *sens_temp, double *sens_humi, int sens_cnt, Supla::Sensor::SHT3x *lSHT3x) {
    this->PD = PD;
    xiaomi_temperatures = sens_temp;
    xiaomi_humidities = sens_humi;
    sensors_cnt = sens_cnt;    
    this->lSHT3x = lSHT3x;
  }
  double getTemp() override {
    
    double calc_temp = TEMPERATURE_NOT_AVAILABLE; 
    
    if ((sensors_cnt < 1) && (lSHT3x)) 
      return lSHT3x->getTemp();
    
    if (PD)
      calc_temp = get_ref_temp(PD->getDP());
    
    if ((calc_temp == TEMPERATURE_NOT_AVAILABLE)  && (lSHT3x))
      calc_temp = lSHT3x->getTemp();

    return calc_temp;

  }

  double getHumi() override {

    double calc_humi = HUMIDITY_NOT_AVAILABLE; 
    
    if ((sensors_cnt < 1) && (lSHT3x)) 
      return lSHT3x->getHumi();
    
    if (PD)
      calc_humi = get_ref_humi(PD->getDP());
    
    if ((calc_humi == HUMIDITY_NOT_AVAILABLE)  && (lSHT3x))
      calc_humi = lSHT3x->getHumi();

    return calc_humi;

    
  }
  
  void setTemp(double val) {
    temperature = val;
  }

  void setHumi(double val) {
    humidity = val;
  }

  void iterateAlways() override {
    if (millis() - lastReadTime > 10000) {
      lastReadTime = millis();
      channel.setNewValue(getTemp(), getHumi());
    }
  }

  void onInit() override {
    channel.setNewValue(temperature, humidity);
  }

 protected:
  
  double get_min_temp() {
    
    double min_temp = 100;
    int th_cnt = 0;

  for (int i = 0; i < sensors_cnt; i++) {
  
    double temp_temp = *(xiaomi_temperatures + i);

    if ((temp_temp > -100) && (temp_temp < min_temp)) {
      min_temp = temp_temp;
      ++th_cnt;
    }
  }

  if (th_cnt == 0) min_temp = TEMPERATURE_NOT_AVAILABLE;

  return min_temp;
  }

double get_min_humi() {
    
    double min_humi = 101;
    int th_cnt = 0;

  for (int i = 0; i < sensors_cnt; i++) {
    
    double temp_humi = *(xiaomi_humidities + i);

    if ((temp_humi > -1) && (temp_humi < min_humi)) {
      min_humi = temp_humi;
      ++th_cnt;
    }
  }

  if (th_cnt == 0) min_humi = HUMIDITY_NOT_AVAILABLE;

  return min_humi;
  }

  double get_avg_temp(int eidx) {
  
  double avg_temp = 0;
  int th_cnt = 0;

  for (int i = 0; i < sensors_cnt; i++) {
  
    double temp_temp = *(xiaomi_temperatures + i);

    if ((temp_temp > -100) && (i != eidx)) {
    avg_temp = avg_temp + temp_temp;
    ++th_cnt;
    }
  }

  if (th_cnt > 0) avg_temp = avg_temp / th_cnt;
  else avg_temp = TEMPERATURE_NOT_AVAILABLE;

  return avg_temp;
}

double get_avg_humi(int eidx) {
  
  double avg_humi = 0;
  int th_cnt = 0;

  for (int i = 0; i < sensors_cnt; i++) {

    double temp_humi = *(xiaomi_humidities + i);
  
    if ((temp_humi > -1) && (i != eidx)) {
      avg_humi = avg_humi + temp_humi;
      ++th_cnt;
    }
  }

  if (th_cnt > 0) avg_humi = avg_humi / th_cnt;
  else avg_humi = HUMIDITY_NOT_AVAILABLE;

  return avg_humi;
}

double get_max_temp() {
  
  double max_temp = TEMPERATURE_NOT_AVAILABLE;

  for (int i = 0; i < sensors_cnt; i++) {

    double temp_temp = *(xiaomi_temperatures + i);

    if ((temp_temp > -100) && (temp_temp > max_temp)) max_temp = temp_temp;
  }

  return max_temp;
}

double get_max_humi() {
  
  double max_humi = HUMIDITY_NOT_AVAILABLE;

  for (int i = 0; i < sensors_cnt; i++) {
    
    double temp_humi = *(xiaomi_humidities + i);

    if ((temp_humi > -1) && (temp_humi > max_humi)) max_humi = temp_humi;
  }

  return max_humi;
}

double get_ref_temp(int idx){
  
  double ref_temp = TEMPERATURE_NOT_AVAILABLE;
  
  switch (idx){
    case 0:
          ref_temp = get_min_temp();
          break;
    case 1:
          ref_temp = get_avg_temp(-1);
          break;
    case 2:
          ref_temp = get_max_temp();
          break;
    default:
          if (idx - 3 < sensors_cnt) ref_temp = *(xiaomi_temperatures + idx - 3);
          else
            if (idx - 3 - sensors_cnt < sensors_cnt) ref_temp = get_avg_temp(idx - 3 - sensors_cnt);
          break;
  }
  return ref_temp;
}

double get_ref_humi(int idx){
  
  double ref_humi = HUMIDITY_NOT_AVAILABLE;
  
  switch (idx){
    case 0:
          ref_humi = get_min_humi();
          break;
    case 1:
          ref_humi = get_avg_humi(-1);
          break;
    case 2:
          ref_humi = get_max_humi();
          break;
    default:
          if (idx - 3 < sensors_cnt) ref_humi = *(xiaomi_humidities + idx - 3);
          else
            if (idx - 3 - sensors_cnt < sensors_cnt) ref_humi = get_avg_temp(idx - 3 - sensors_cnt);
          break;
  }
  return ref_humi;
}

  Supla::Sensor::ProgDisplay *PD = NULL;
  double temperature = TEMPERATURE_NOT_AVAILABLE;
  double humidity = HUMIDITY_NOT_AVAILABLE;
  double *xiaomi_temperatures = NULL;
  double *xiaomi_humidities = NULL;
  int sensors_cnt = 0;
  Supla::Sensor::SHT3x *lSHT3x = NULL;

};

};  // namespace Sensor
};  // namespace Supla

#endif  // SRC_SUPLA_SENSOR_XIAOMI_THERM_HYGRO_METER_H_
