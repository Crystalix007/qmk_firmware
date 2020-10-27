# MCU name
MCU = STM32F103

OPT_DEFS = -DCORTEX_VTOR_INIT=0x2000
MCU_LDSCRIPT = STM32F103xB_stm32duino_bootloader
BOARD = YAEMK
STM32_BOOTLOADER_ADDRESS = 0x80000000

DFU_ARGS = -d 1eaf:0003 -a2 -R
DFU_SUFFIX_ARGS = -v 1eaf -p 0003

# project specific files
SRC =	led_config.c
LTO_ENABLE = yes
EXTRAFLAGS += -Ofast
OPT_DEFS += -DSTM32_DMA_REQUIRED=TRUE

MOUSEKEY_ENABLE = yes	# Mouse keys
EXTRAKEY_ENABLE = yes	# Audio control and System control
CONSOLE_ENABLE = no		# Console for debug
NKRO_ENABLE = yes		# USB Nkey Rollover
RGB_MATRIX_ENABLE = WS2812
WS2812_DRIVER = pwm
ENCODER_ENABLE = yes
DEBOUNCE_TYPE = eager_pk
OLED_DRIVER_ENABLE = no
WPM_ENABLE = no
SPLIT_KEYBOARD = yes
SERIAL_DRIVER = usart_duplex
VIA_ENABLE = yes

LAYOUTS = split_5x8

# Enter lower-power sleep mode when on the ChibiOS idle thread
OPT_DEFS += -DCORTEX_ENABLE_WFI_IDLE=TRUE
