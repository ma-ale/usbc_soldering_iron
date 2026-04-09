all : build

TARGET:=main
TARGET_MCU:=CH32X035
TARGET_MCU_PACKAGE:=CH32X035F8U6

include ch32fun/ch32fun/ch32fun.mk

FLASH_COMMAND=$(MINICHLINK)/minichlink -C isp -w $< $(WRITE_SECTION) -b

flash : cv_flash
clean : cv_clean
