all : build

TARGET := main
TARGET_MCU := CH32X035
TARGET_MCU_PACKAGE := CH32X035F8U6
BUILD_DIR := .build
DISPLAY := ssd1312

# include u8g2
U8G2_DIR:=u8g2/csrc
U8G2_SRC:=$(U8G2_DIR)/u8x8_d_$(DISPLAY).c $(filter-out $(U8G2_DIR)/u8x8_d_%.c, \
	$(filter-out $(U8G2_DIR)/mui%.c, $(wildcard $(U8G2_DIR)/*.c)))

EXTRA_CFLAGS += -I$(U8G2_DIR)
ADDITIONAL_C_FILES += lib_i2c.c display.c sc7a20.c

include ch32fun/ch32fun/ch32fun.mk

FLASH_COMMAND=$(MINICHLINK)/minichlink -C isp -w $< $(WRITE_SECTION) -b

# create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# build u8g2
U8G2_OBJS := $(addprefix $(BUILD_DIR)/, $(notdir $(U8G2_SRC:.c=.o)))
$(BUILD_DIR)/%.o : $(U8G2_DIR)/%.c | $(BUILD_DIR)
	$(PREFIX)-gcc $(CFLAGS) -c $< -o $@

# build target
TARGET_OBJS := $(addprefix $(BUILD_DIR)/, $(filter-out ch32fun.o, $(notdir $(FILES_TO_COMPILE:.c=.o))))
$(BUILD_DIR)/%.o : %.c | $(BUILD_DIR)
	$(PREFIX)-gcc $(CFLAGS) -c $< -o $@

# link target, the rest of the build is specified in ch32fun.mk
$(TARGET).elf : $(FILES_TO_COMPILE) $(LINKER_SCRIPT) $(EXTRA_ELF_DEPENDENCIES) $(U8G2_OBJS) $(TARGET_OBJS) ch32fun.o
	$(PREFIX)-gcc -o $@ $(U8G2_OBJS) $(TARGET_OBJS) ch32fun.o $(CFLAGS) $(LDFLAGS)


flash : cv_flash
clean : cv_clean
	rm -f $(BUILD_DIR)/*.o *.o || true

monitor :
	picocom --imap lfcrlf /dev/ttyACM0
