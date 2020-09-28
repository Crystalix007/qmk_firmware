#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define VENDOR_ID 0x4B4B   // KK
#define PRODUCT_ID 0x5941  // YA
#define DEVICE_VER 0x0001
#define MANUFACTURER YAEMK
#define PRODUCT YAEMK 1.1
#define DESCRIPTION YAEMK 1.1
#define PRODUCT YAEMK 1.0
#define DESCRIPTION YAEMK 1.0

#define MATRIX_COL_PINS \
    { A8, B15, A9, B11, B10, B2, B1, A3 }
#define MATRIX_ROW_PINS \
    { A10, B14, A6, A4, A5 }
#define MATRIX_COL_PINS_RIGHT \
    { A3, B1, B2, B10, B11, A9, B15, A8 }
#define MATRIX_ROW_PINS_RIGHT \
    { A10, B14, A6, A4, A5 }

#define ENCODERS_PAD_A_RIGHT \
    { A7 }
#define ENCODERS_PAD_B_RIGHT \
    { B0 }
#define ENCODERS_PAD_A \
    { B0 }
#define ENCODERS_PAD_B \
    { A7 }

#define SPLIT_HAND_PIN C13

#define RGBLIGHT_SPLIT
#define RGB_MATRIX_SPLIT RGBLED_SPLIT
#define SPLIT_TRANSPORT_MIRROR
