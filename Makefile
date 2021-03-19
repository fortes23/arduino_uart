CFG_FILE := .config

# Read configuration file
include $(CFG_FILE)

# Linux application
include linux/linux.mk

# Arduino application
include arduino/arduino.mk
