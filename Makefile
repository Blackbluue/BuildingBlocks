#
# **************************************************************
# *                Simple C++ Makefile Template                *
# *                                                            *
# * Author: Arash Partow (2003)                                *
# * URL: http://www.partow.net/programming/makefile/index.html *
# *                                                            *
# * Copyright notice:                                          *
# * Free use of this C++ Makefile template is permitted under  *
# * the guidelines and in accordance with the the MIT License  *
# * http://www.opensource.org/licenses/MIT                     *
# *                                                            *
# **************************************************************
#
# Modifications from: https://tech.davis-hansson.com/p/make/
SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error This Make does not support .RECIPEPREFIX. Please use GNU Make 4.0 or later)
endif
.RECIPEPREFIX = >

CC              := gcc
CFLAGS          := -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS         := -L/usr/lib
BUILD           := blocks
OBJ_DIR         := $(BUILD)/objects
APP_DIR         := $(BUILD)/apps

ARR_LST_TARGETS := array_list
UDP_TARGETS  := client udp_server
UNIX_TARGETS := client unix_server

TARGETS         := $(ARR_LST_TARGETS) $(UDP_TARGETS) $(UNIX_TARGETS)
INCLUDE         := -Iinclude/
SRC_DIR         := src
SRC             := $(wildcard $(SRC_DIR)/*.c)
OBJECTS         := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPENDENCIES    := $(OBJECTS:.o=.d)

all: build $(addprefix $(APP_DIR)/, $(TARGETS))


# create object files from source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
> @mkdir -p $(@D)
> $(CC) $(CFLAGS) $(INCLUDE) -c $< -MMD -o $@

# build targets
$(APP_DIR)/%: $(OBJ_DIR)/%.o $(OBJ_DIR)/counter_utils.o
> @mkdir -p $(@D)
> $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

array_list: $(addprefix $(OBJ_DIR)/, $(ARR_LST_TARGETS))

udp: $(addprefix $(APP_DIR)/, $(UDP_TARGETS))

unix: $(addprefix $(APP_DIR)/, $(UNIX_TARGETS))


-include $(DEPENDENCIES)

# special make commands
.PHONY: all build clean debug release info tcp udp

build:
> @mkdir -p $(APP_DIR)

debug: CFLAGS += -DDEBUG -g
debug: all

release: CFLAGS += -O2
release: all

clean:
> -@rm -rvf $(OBJ_DIR)/*
> -@rm -rvf $(APP_DIR)/*

info:
> @echo "[*] Targets:         ${TARGETS}     "
> @echo "[*] Application dir: ${APP_DIR}     "
> @echo "[*] Object dir:      ${OBJ_DIR}     "
> @echo "[*] Sources:         ${SRC}         "
> @echo "[*] Objects:         ${OBJECTS}     "
> @echo "[*] Dependencies:    ${DEPENDENCIES}"
