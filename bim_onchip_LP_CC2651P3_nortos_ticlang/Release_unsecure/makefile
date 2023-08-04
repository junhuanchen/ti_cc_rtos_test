################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CG_TOOL_ROOT := /home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./Application/bim_onchip_main.o" \
"./Application/bim_util.o" \
"./Application/crc32.o" \
"./Application/flash_interface_internal.o" \
"./Application/led_debug.o" \
"./Application/startup_ticlang.o" \
"./SPI/bsp_spi_cc13x2_cc26x2.o" \
"../bim_cc26x1_cc13x1.cmd" \
$(GEN_CMDS__FLAG) \
-Wl,-l"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/driverlib/bin/ticlang/driverlib.lib" \
-Wl,-llibc.a \

-include ../makefile.init

RM := rm -rf
RMDIR := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include Application/subdir_vars.mk
-include SPI/subdir_vars.mk
-include subdir_rules.mk
-include Application/subdir_rules.mk
-include SPI/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
bim_onchip_LP_CC2651P3_nortos_ticlang.out \

EXE_OUTPUTS__QUOTED += \
"bim_onchip_LP_CC2651P3_nortos_ticlang.out" \


# All Target
all: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "bim_onchip_LP_CC2651P3_nortos_ticlang.out"

# Tool invocations
bim_onchip_LP_CC2651P3_nortos_ticlang.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: Arm Linker'
	"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmclang" -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -DDeviceFamily_CC26X1 -DSET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID=0x56000 -DBIM_ONCHIP -DxSECURITY -DxDEBUG_BIM -gdwarf-3 -fshort-enums -munaligned-access -funsigned-char -fcommon -ffunction-sections -fdata-sections -march=armv7e-m -Wl,-m"bim_onchip_LP_CC2651P3_nortos_ticlang.map" -Wl,--stack_size=256 -Wl,-i"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1" -Wl,-i"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/inc" -Wl,-i"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/driverlib" -Wl,-i"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/lib" -Wl,--reread_libs -Wl,--diag_wrap=off -Wl,--display_error_number -Wl,--warn_sections -Wl,--xml_link_info="bim_onchip_LP_CC2651P3_nortos_ticlang_linkInfo.xml" -Wl,--rom_model -Wl,--unused_section_elimination=on -o "bim_onchip_LP_CC2651P3_nortos_ticlang.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)
	-$(RM) "Application/bim_onchip_main.o" "Application/bim_util.o" "Application/crc32.o" "Application/flash_interface_internal.o" "Application/led_debug.o" "Application/startup_ticlang.o" "SPI/bsp_spi_cc13x2_cc26x2.o" 
	-$(RM) "Application/bim_onchip_main.d" "Application/bim_util.d" "Application/crc32.d" "Application/flash_interface_internal.d" "Application/led_debug.d" "Application/startup_ticlang.d" "SPI/bsp_spi_cc13x2_cc26x2.d" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmhex -order MS --memwidth=8 --romwidth=8 --intel -o bim_onchip_LP_CC2651P3_nortos_ticlang.hex bim_onchip_LP_CC2651P3_nortos_ticlang
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

