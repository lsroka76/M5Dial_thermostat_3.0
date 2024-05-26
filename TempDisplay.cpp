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

#include "TempDisplay.h"

namespace Supla {
namespace Sensor {

/*void SetDisplay::handleAction(int event, int action) {
  (void)(event);
  switch (action) {
    case TURN_ON: {
      stepUp();
      break;
    }
    case TURN_OFF: {
      stepDown();
      break;
    }    
  }
}*/

TempDisplay::TempDisplay(double dt_current, double dt_min, double dt_max, double dt_step)
    : dt_current(dt_current), dt_min(dt_min), dt_max(dt_max), dt_step(dt_step) {
}

double TempDisplay::getValue() {
  return dt_current;
}

double TempDisplay::getMinValue() {
  return dt_min;
}

double TempDisplay::getMaxValue() {
  return dt_max;
}

double TempDisplay::getStepValue() {
  return dt_step;
}

void TempDisplay::onInit() {
 
  channel.setNewValue(getValue());
}

void TempDisplay::onSaveState() {
  Supla::Storage::WriteState((unsigned char *)&dt_current,
                             sizeof(dt_current));
 
}

void TempDisplay::onLoadState() {
  Supla::Storage::ReadState((unsigned char *)&dt_current,
                            sizeof(dt_current));
 
}

void TempDisplay::setTemp(double dt_set) {

  dt_current = dt_set;
}

void TempDisplay::stepUp() {

  double dt_new = dt_current + dt_step;
  if (dt_new < dt_max) dt_current = dt_new;
  else dt_current = dt_max;

  channel.setNewValue(dt_current);

  // Schedule save in 5 s after state change
  Supla::Storage::ScheduleSave(5000);

}

void TempDisplay::stepDown() {

  double dt_new = dt_current - dt_step;
  if (dt_new > dt_min) dt_current = dt_new;
  else dt_current = dt_min;

  channel.setNewValue(dt_current);

  // Schedule save in 5 s after state change
  Supla::Storage::ScheduleSave(5000);
}

ProgDisplay::ProgDisplay(int dp_current, int dp_cnt, double *dpt):
                        dp_current(dp_current), dp_cnt(dp_cnt), dpt(dpt) {

}

double ProgDisplay::getValue() {
  double rt_value = -275;
  //Serial.println(dp_current);
  switch (dp_current) {
    case 0: 
          rt_value = -100;
          break;
    case 1:
          rt_value = 0;
          break;
    case 2:
          rt_value = 100;
          break;
    default:
          if ((dp_current > 2)&&(dp_current - 3 < dp_cnt))
          rt_value = *(dpt + (dp_current - 3));
          else
          if ((dp_current >= dp_cnt + 3)&&(dp_current  < (2*dp_cnt+3))) {
            rt_value = 1*(*(dpt +(dp_current - (dp_cnt + 3))));
            if (rt_value > -275) rt_value = -1*rt_value;
          } 
          break;
  }
    return rt_value;
}

void ProgDisplay::onInit() {
 
  channel.setNewValue(getValue());
}

void ProgDisplay::onSaveState() {
  Supla::Storage::WriteState((unsigned char *)&dp_current,
                             sizeof(dp_current)); 
}

void ProgDisplay::onLoadState() {
  Supla::Storage::ReadState((unsigned char *)&dp_current,
                            sizeof(dp_current));
  if (dp_current >= (2*dp_cnt+3)) dp_current = 0;
 
}

void ProgDisplay::stepUp() {

  //Serial.println("entering stepUp");
  int dp_new = dp_current + 1;
  if (dp_new < (2*dp_cnt+3)) dp_current = dp_new;
  else dp_current = 0;
  //Serial.println(dp_current);

  channel.setNewValue(getValue());

  // Schedule save in 5 s after state change
  Supla::Storage::ScheduleSave(5000);

}

void ProgDisplay::stepDown() {

  int dp_new = dp_current - 1;
  if (dp_new < 0) dp_current = 0;
  else dp_current = dp_new;

  channel.setNewValue(getValue());

  // Schedule save in 5 s after state change
  Supla::Storage::ScheduleSave(5000);
}

void ProgDisplay::setDP(int new_dp){

  if (new_dp < (2*dp_cnt+3)) dp_current = new_dp;
  else dp_current = 0;
  //Serial.println(dp_current);

  channel.setNewValue(getValue());

  // Schedule save in 5 s after state change
  Supla::Storage::ScheduleSave(5000);

}


int ProgDisplay::getDP(){
  return dp_current; 
}

int ProgDisplay::getCount(){
  return dp_cnt;
} 

int ProgDisplay::getPCount(){
  return (2*dp_cnt+3);
} 

DisplayAH::DisplayAH(m5::M5_DIAL *dah_m5dial, bool *dah_switch) : dah_m5dial(dah_m5dial), dah_switch(dah_switch) {

  dah_timestamp = millis();
};

void DisplayAH::handleAction(int event, int action) {
  (void)(event);
  switch (action) {
    case TURN_ON: {
      (*dah_switch) = true;
      break;
    }
    case TURN_OFF: {
      (*dah_switch) = true;      
      break;
    }
    case TOGGLE: {
      (*dah_switch) = true;
      break;
    }
  }
}

void DisplayAH::iterateAlways(){

  
  dah_timestamp = millis();
  //if (dah_m5dial) dah_m5dial->update();
}

};  // namespace Sensor
};  // namespace Supla
