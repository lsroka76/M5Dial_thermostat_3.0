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

#ifndef TempDisplay_h
#define TempDisplay_h

#include <Arduino.h>
#include <supla/sensor/thermometer.h>
#include <supla/storage/storage.h>
#include <supla/actions.h>
#include <supla/action_handler.h>
#include <supla/element.h>
#include <supla/local_action.h>
#include <supla/events.h>
#include <M5Dial.h>

namespace Supla {
namespace Sensor {

class SetDisplay : public Thermometer{//, public ActionHandler {
  public:

  //void handleAction(int event, int action);

  virtual void stepUp() = 0;
  virtual void stepDown() = 0;
};

class TempDisplay : public SetDisplay {
  public:
  TempDisplay(double dt_current, double dt_min, double dt_max, double dt_step);
  
  double getValue();
  double getMinValue();
  double getMaxValue();
  double getStepValue();
  
  void onInit();
  void onLoadState();
  void onSaveState();

  //void handleAction(int event, int action);

  virtual void stepUp();
  virtual void stepDown();

  void setTemp(double dt_current);

 protected:
  double dt_min;
  double dt_max;
  double dt_step;

  double dt_current;  
  
};

class ProgDisplay : public SetDisplay {
  public:
  ProgDisplay(int dp_current, int dp_cnt, double *dpt);  
  
  double getValue();

  void onInit();
  void onLoadState();
  void onSaveState();

  //void handleAction(int event, int action);

  virtual void stepUp();
  virtual void stepDown();

  virtual void setDP(int new_dp);

  virtual int getDP();
  virtual int getCount();
  virtual int getPCount();

 protected:
  int dp_current;
  int dp_cnt;
  double *dpt;
  
};

class DisplayAH : public ActionHandler, public Element {
  public:
    DisplayAH(m5::M5_DIAL *dah_m5dial, bool *dah_switch);
    void handleAction(int event, int action);
    void iterateAlways();
  protected:
    bool *dah_switch = NULL;
    long dah_timestamp = 0;
    m5::M5_DIAL *dah_m5dial = NULL;
};

};  // namespace Sensor
};  // namespace Supla

extern long screen_saver_time;
extern bool nm_menu_redraw;

extern bool screen_saver_on;

extern m5::touch_state_t touch_prev_state;

#endif
