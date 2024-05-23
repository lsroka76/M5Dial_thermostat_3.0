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

#include "multi_virtual_relay.h"

 Supla::Control::Multi_VirtualRelay::Multi_VirtualRelay(Supla::Sensor::SetDisplay *SD,_supla_int_t functions)
    : Supla::Control::Relay(-1, true, functions), 
   
    state(false),cCount(0), SD(SD) {
}

 Supla::Control::Multi_VirtualRelay::~Multi_VirtualRelay() {
 
}

void  Supla::Control::Multi_VirtualRelay::onInit() {
  /*if (stateOnInit == STATE_ON_INIT_ON ||
      stateOnInit == STATE_ON_INIT_RESTORED_ON) {
    turnOn();
  } else {
    turnOff();
  }*/
}

void Supla::Control::Multi_VirtualRelay::iterateAlways() {

  if (durationMs && millis() - durationTimestamp > durationMs) {
    toggle();
  }
} 

void  Supla::Control::Multi_VirtualRelay::turnOn(_supla_int_t duration) {
  durationMs = duration;
  durationTimestamp = millis();
  if (keepTurnOnDurationMs) {
    durationMs = storedTurnOnDurationMs;
  }
  
  state = true;
  cCount = cCount +1;

  //Serial.println("jump to stepUp");
  SD->stepUp();  
  
  //channel.setNewValue(state);
  //runAction(Supla::ON_TURN_ON);
  // Schedule save in 5 s after state change
  Supla::Storage::ScheduleSave(5000);
}

void  Supla::Control::Multi_VirtualRelay::turnOff(_supla_int_t duration) {
  durationMs = duration;
  durationTimestamp = millis();
  
  state = false;
  cCount = cCount - 1;

  SD->stepDown();

  //channel.setNewValue(state);
  //runAction(Supla::ON_TURN_OFF);
  // Schedule save in 5 s after state change
  Supla::Storage::ScheduleSave(5000);
}

bool  Supla::Control::Multi_VirtualRelay::isOn() {
  return state;
}

int  Supla::Control::Multi_VirtualRelay::getCount() {
  return cCount;
}