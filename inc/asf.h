#ifndef ASF_H
#define ASF_H

/*
 * This file includes all API header files for the selected drivers from ASF.
 * Note: There might be duplicate includes required by more than one driver.
 *
 */

#include <compiler.h>
#include <status_codes.h>
#include <board.h>
#include <sysclk.h>
#include <gpio.h>
#include <rstc.h>
#include <pmc.h>
#include <wdt.h>
#include <dacc.h>
#include <adc.h>
#include <tc.h>
#include <fifo.h>
#include <twi.h>
#include <delay.h>

// From module: USB CDC Protocol
#include <usb_protocol_cdc.h>

// From module: USB Device CDC (Single Interface Device)
#include <udi_cdc.h>

// From module: USB Device Stack Core (Common API)
#include <udc.h>
#include <udd.h>

#endif // ASF_H
