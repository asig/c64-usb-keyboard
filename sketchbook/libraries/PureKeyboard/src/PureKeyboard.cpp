/*
  PureKeyboard.cpp

  Copyright (c) 2016 Andreas Signer.
  Original code: Copyright (c) 2015 Arduino LLC; Copyright (c) 2011 Peter Barrett

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "PureKeyboard.h"

#if defined(_USING_HID)

//================================================================================
//================================================================================
//	Keyboard

//  Low level key report: up to 6 keys and shift, ctrl etc at once
static struct KeyReport {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} keyReport;

static const uint8_t _hidReportDescriptor[] PROGMEM = {

  //  Keyboard
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)  // 47
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x02,                    //   REPORT_ID (2)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
   
  0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    
  0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    
  0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    
  0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0,                          // END_COLLECTION
};

static void sendReport() {
	HID().SendReport(2, &keyReport, sizeof(KeyReport));
}

PureKeyboard_::PureKeyboard_() {
	static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
	HID().AppendDescriptor(&node);
}

void PureKeyboard_::begin(void) {
}

void PureKeyboard_::end(void) {
}

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way 
// USB HID works, the host acts like the key remains pressed until we 
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t PureKeyboard_::press(uint8_t k) {
	uint8_t i;
	if (k >= KEY_LEFT_CTRL) {
		k -= KEY_LEFT_CTRL;
		keyReport.modifiers |= (1<<k);
	} else {
		// Add k to the key report only if it's not already present
		// and if there is an empty slot.
		if (keyReport.keys[0] != k && keyReport.keys[1] != k && 
			keyReport.keys[2] != k && keyReport.keys[3] != k &&
			keyReport.keys[4] != k && keyReport.keys[5] != k) {
			
			for (i=0; i<6; i++) {
				if (keyReport.keys[i] == 0x00) {
					keyReport.keys[i] = k;
					break;
				}
			}
			if (i == 6) {
				return 0;
			}	
		}
	}
	sendReport();
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t PureKeyboard_::release(uint8_t k) {
	uint8_t i;
	if (k >= KEY_LEFT_CTRL) {
		k -= KEY_LEFT_CTRL;
		keyReport.modifiers &= ~(1<<k);
	} else {		
		// Test the key report to see if k is present.  Clear it if it exists.
		// Check all positions in case the key is present more than once (which it shouldn't be)
		for (i=0; i<6; i++) {
			if (0 != k && keyReport.keys[i] == k) {
				keyReport.keys[i] = 0x00;
			}
		}
	}

	sendReport();
	return 1;
}

void PureKeyboard_::releaseAll() {
	keyReport.keys[0] = 0;
	keyReport.keys[1] = 0;	
	keyReport.keys[2] = 0;
	keyReport.keys[3] = 0;	
	keyReport.keys[4] = 0;
	keyReport.keys[5] = 0;	
	keyReport.modifiers = 0;
	sendReport();
}

PureKeyboard_ PureKeyboard;

#endif
