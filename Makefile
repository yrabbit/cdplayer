MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --warn-undefined-variables

SHELL       := zsh
.ONESHELL:
# error on unset vars, exit on error in pipe commands
.SHELLFLAGS := -o nounset -e -c
.DELETE_ON_ERROR:

ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error This Make does not support .RECIPEPREFIX. Please use GNU Make 4.0 or later)
endif
.RECIPEPREFIX = >

# dirs
OUTPUT              := .out
LIBDIR              := .out

# Flags
PREFIX:=riscv-unknown-elf
RISCV_ARCH_PATH     :=riscv-unknown-elf
RISCV_FULL_PATH     :=/opt/riscv32ec/${RISCV_ARCH_PATH}
RISCV_COMPILER_PATH :=/opt/riscv32ec/
RISCV_BINS          :=${RISCV_COMPILER_PATH}/bin
RISCV_LIBS          :=${RISCV_FULL_PATH}/lib
RISCV_INCS          :=${RISCV_FULL_PATH}/include
CH32V003FUN         :=${HOME}/src/misc-src/ch32v003fun/ch32v003fun

CC:=${RISCV_BINS}/${PREFIX}-gcc
MINICHLINK:=../../minichlink

CFLAGS:= \
    -I${RISCV_INCS} \
    -L${RISCV_LIBS} \
	-g -Os -flto -ffunction-sections \
	-static-libgcc -lgcc \
	-march=rv32ec \
	-mabi=ilp32e \
	-I${CH32V003FUN} \
	-nostdlib


CINC_APP        := -I./include
CFLAGS_APP      := ${CFLAGS} ${CINC_APP}

LDFLAGS:=-T ${CH32V003FUN}/ch32v003fun.ld -Wl,--gc-sections

WARNINGS        := -Wall -Wextra \
                   -Wformat-overflow=2 -Wshift-overflow=2 -Wimplicit-fallthrough=5 \
                   -Wformat-signedness -Wformat-truncation=2 \
                   -Wstringop-overflow=4 -Wunused-const-variable=2 -Walloca \
                   -Warray-bounds=2 -Wswitch-bool -Wsizeof-array-argument \
                   -Wlto-type-mismatch -Wnull-dereference \
                   -Wdangling-else \
                   -Wpacked -Wfloat-equal -Winit-self -Wmissing-include-dirs \
                   -Wmissing-noreturn -Wbool-compare \
                   -Wsuggest-attribute=noreturn -Wsuggest-attribute=format \
                   -Wmissing-format-attribute \
                   -Wuninitialized -Wtrampolines -Wframe-larger-than=2048 \
                   -Wunsafe-loop-optimizations -Wpointer-arith -Wbad-function-cast \
                   -Wwrite-strings -Wsequence-point -Wlogical-op \
                   -Wlogical-not-parentheses \
                   -Wvla -Wdisabled-optimization \
                   -Wunreachable-code -Wparentheses -Wdiscarded-array-qualifiers \
                   -Winline -Wmultistatement-macros -Warray-bounds=2 \
                   -Wno-error=unsafe-loop-optimizations \
                   -Wno-packed \
                   -Wno-unused-parameter

SYSTEM_C:=$(CH32V003FUN)/ch32v003fun.c

#####################################
# The project parts
LIB_DIR           := .
LIB_SRC           := ${LIB_DIR}/src
LIB_INCLUDE       := ${LIB_DIR}/include
LIB_OUTPUT        := ${LIB_DIR}/${OUTPUT}-lib
LIB_SENTINEL      := ${LIB_OUTPUT}/.sentinel
LIB               := calc
LIB_FILE          := ${LIBDIR}/lib${LIB}.a

PROJ_DIR          := .
PROJ_SRC          := ${PROJ_DIR}/app
PROJ_INCLUDE      := ${PROJ_DIR}/include
PROJ_OUTPUT       := ${PROJ_DIR}/${OUTPUT}
PROJ_SENTINEL     := ${PROJ_OUTPUT}/.sentinel
PROJ_CFLAGS       := ${CFLAGS_APP}
PROJ_LDFLAGS      := ${LDFLAGS}
PROJ              := calc
PROJ_ELF_FILE     := ${PROJ_OUTPUT}/${PROJ}.elf
PROJ_BIN_FILE     := ${PROJ_OUTPUT}/${PROJ}.bin
PROJ_HEX_FILE     := ${PROJ_OUTPUT}/${PROJ}.hex
PROJ_MAP_FILE     := ${PROJ_OUTPUT}/${PROJ}.map
PROJ_LST_FILE     := ${PROJ_OUTPUT}/${PROJ}.lst

#####################################
# Batch comiple. The parameters are:
# - the output path
# - list of C files
# - compiler options
#
# zsh/gnu-make magic to make the obj file name: {name%pattern}
# removes the pattern from the name, use $$ in order to pass the $ to zsh
# -o $(@D)/$${$$(basename $${src})%.c}.o
define compile_c_files
    mkdir -p $(1)
    for src in $(2); do
        ${CC} ${WARNINGS} $(3) \
            -c $${src} -o $(1)/$${$$(basename $${src})%.c}.o ;
    done
endef

#####################################
#  The parameters:
#  - the output path
#  - list of C files
#  - inc flags
define compile_lib
    $(call compile_c_files, $(1), $(2), ${CFLAGS_APP} $(3))
endef

#####################################
#  Compile with options for the app files
#  including local libs
#  The parameters:
#  - the output path
#  - list of C files
#  - inc flags
define compile_app
    $(call compile_c_files, $(1), $(2), ${CFLAGS_APP} $(3))
endef

#####################################
all: calc
.PHONY: all

lib: calc
.PHONY: lib

clean:
>   find `pwd` -name ${OUTPUT} -exec rm -rf \{\} +
>   rm -rf ${LIB_OUTPUT}


#####################################
# YR calc library
# := static assigment
# = dynamic assigment
LIB_C_FILES       := ${SYSTEM_C} $(shell find ${LIB_SRC} -name '*.c')
LIB_H_DEPS        := $(shell find ${LIB_INCLUDE} -name '*.h')
LIB_O_FILES       = $(shell find ${LIB_OUTPUT} -name '*.o')
LIB_INC_CFLAGS    := -I${LIB_INCLUDE}

${LIB_SENTINEL}: ${LIB_C_FILES} ${LIB_H_DEPS}
>   $(call compile_lib, $(@D), ${LIB_C_FILES}, ${LIB_INC_CFLAGS})
>   touch $@

${LIB_FILE}: ${LIB_SENTINEL}
>   mkdir -p ${LIB_DIR}
>   rm -rf $@ 2> /dev/null
>   ${AR} r $@ ${LIB_O_FILES}

proj_lib:   ${LIB_FILE}
.PHONY: proj_lib

#####################################
# --start-group/--end-group make linker
#  search for names several times in the
#  group
# := static assigment
# = dynamic assigment
PROJ_C_FILES    := $(shell find ${PROJ_SRC} -name '*.c')
PROJ_H_DEPS     := $(shell find ${PROJ_INCLUDE} -name '*.h')
PROJ_O_FILES    = $(shell find ${PROJ_OUTPUT} -name '*.o')
PROJ_INC_CFLAGS :=  -I${PROJ_INCLUDE}

${PROJ_SENTINEL}: ${PROJ_C_FILES} ${PROJ_H_DEPS}
>   $(call compile_app, $(@D), ${PROJ_C_FILES}, ${PROJ_INC_CFLAGS})
>   touch $@

${PROJ_ELF_FILE}: ${PROJ_SENTINEL} proj_lib
>   mkdir -p ${PROJ_OUTPUT}
>   ${CC} -o $@ \
    -Wl,--start-group \
    ${PROJ_O_FILES} ${LIB_FILE} \
    -Wl,--end-group -o $@ \
    ${PROJ_CFLAGS} ${PROJ_LDFLAGS}

#    -Wl,--start-group \
#    ${PROJ_O_FILES} ${LIB_FILE} \
#    -Wl,--end-group -o $@

${PROJ_BIN_FILE}: ${PROJ_ELF_FILE}
>  	${RISCV_BINS}/${PREFIX}-size $^
>	${RISCV_BINS}/${PREFIX}-objdump -S $^ > ${PROJ_LST_FILE}
>	${RISCV_BINS}/${PREFIX}-objdump -t $^ > ${PROJ_MAP_FILE}
>	${RISCV_BINS}/${PREFIX}-objcopy -O binary $< ${PROJ_BIN_FILE}
>	${RISCV_BINS}/${PREFIX}-objcopy -O ihex $< ${PROJ_HEX_FILE}


calc: ${PROJ_BIN_FILE}
.PHONY: calc

# vim: expandtab: ts=4 sw=4 ft=yrmake:
