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

#ifndef _multi_virtual_relay_h
#define _multi_virtual_relay_h

#include <supla/control/relay.h>
#include <supla/actions.h>
#include <supla/action_handler.h>
#include <supla/local_action.h>
#include <supla/events.h>

#include "TempDisplay.h"

namespace Supla {
namespace Control {

class Multi_VirtualRelay : public Supla::Control::Relay {
 public:
  Multi_VirtualRelay(Supla::Sensor::SetDisplay *SD,_supla_int_t functions =
                   (0xFF ^ SUPLA_BIT_FUNC_CONTROLLINGTHEROLLERSHUTTER));

  ~Multi_VirtualRelay();
  void onInit(); 
  void turnOn(_supla_int_t duration = 0);
  void turnOff(_supla_int_t duration = 0);
  bool isOn();
  int getCount();
  void iterateAlways(); 

 protected:
  bool state;
  int cCount;
  Supla::Sensor::SetDisplay *SD;	

};

};  // namespace Control
};  // namespace Supla

#endif
