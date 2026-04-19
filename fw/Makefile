all : build

TARGET := main
TARGET_MCU := CH32X035
TARGET_MCU_PACKAGE := CH32X035F8U6

# include u8g2
U8G2_DIR:=u8g2/csrc
U8G2_SRC:=u8g2/csrc/u8x8_d_ssd1312.c $(filter-out $(U8G2_DIR)/u8x8_d_%.c, $(wildcard $(U8G2_DIR)/*.c))
# U8G2_SRC:=u8g2/csrc/u8x8_d_ssd1306_128x32.c $(filter-out $(U8G2_DIR)/u8x8_d_%.c, $(wildcard $(U8G2_DIR)/*.c))

EXTRA_CFLAGS += -I$(U8G2_DIR)

ADDITIONAL_C_FILES += $(U8G2_SRC)
ADDITIONAL_C_FILES += lib_i2c.c
ADDITIONAL_C_FILES += display.c

include ch32fun/ch32fun/ch32fun.mk

FLASH_COMMAND=$(MINICHLINK)/minichlink -C isp -w $< $(WRITE_SECTION) -b

flash : cv_flash
clean : cv_clean

monitor :
	picocom --imap lfcrlf /dev/ttyACM0
