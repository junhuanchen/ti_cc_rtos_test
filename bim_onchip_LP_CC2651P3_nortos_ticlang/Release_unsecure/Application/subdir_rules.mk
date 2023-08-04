################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
Application/bim_onchip_main.o: /home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/bim/bim_onchip_main.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/home/juwan/workspace_v12/bim_onchip_LP_CC2651P3_nortos_ticlang" -I"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/include" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/flash/no_rtos/extFlash" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/startup_files" -DDeviceFamily_CC26X1 -DSET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID=0x56000 -DBIM_ONCHIP -DxSECURITY -DxDEBUG_BIM -gdwarf-3 -fshort-enums -munaligned-access -funsigned-char -fcommon -ffunction-sections -fdata-sections -march=armv7e-m -MMD -MP -MF"Application/$(basename $(<F)).d_raw" -MT"$(@)" -std=gnu9x $(GEN_OPTS__FLAG) -o"$@" "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

Application/bim_util.o: /home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/cc26xx/bim/bim_util.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/home/juwan/workspace_v12/bim_onchip_LP_CC2651P3_nortos_ticlang" -I"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/include" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/flash/no_rtos/extFlash" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/startup_files" -DDeviceFamily_CC26X1 -DSET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID=0x56000 -DBIM_ONCHIP -DxSECURITY -DxDEBUG_BIM -gdwarf-3 -fshort-enums -munaligned-access -funsigned-char -fcommon -ffunction-sections -fdata-sections -march=armv7e-m -MMD -MP -MF"Application/$(basename $(<F)).d_raw" -MT"$(@)" -std=gnu9x $(GEN_OPTS__FLAG) -o"$@" "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

Application/crc32.o: /home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/cc26xx/crc/crc32.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/home/juwan/workspace_v12/bim_onchip_LP_CC2651P3_nortos_ticlang" -I"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/include" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/flash/no_rtos/extFlash" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/startup_files" -DDeviceFamily_CC26X1 -DSET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID=0x56000 -DBIM_ONCHIP -DxSECURITY -DxDEBUG_BIM -gdwarf-3 -fshort-enums -munaligned-access -funsigned-char -fcommon -ffunction-sections -fdata-sections -march=armv7e-m -MMD -MP -MF"Application/$(basename $(<F)).d_raw" -MT"$(@)" -std=gnu9x $(GEN_OPTS__FLAG) -o"$@" "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

Application/flash_interface_internal.o: /home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/cc26xx/flash_interface/internal/flash_interface_internal.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/home/juwan/workspace_v12/bim_onchip_LP_CC2651P3_nortos_ticlang" -I"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/include" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/flash/no_rtos/extFlash" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/startup_files" -DDeviceFamily_CC26X1 -DSET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID=0x56000 -DBIM_ONCHIP -DxSECURITY -DxDEBUG_BIM -gdwarf-3 -fshort-enums -munaligned-access -funsigned-char -fcommon -ffunction-sections -fdata-sections -march=armv7e-m -MMD -MP -MF"Application/$(basename $(<F)).d_raw" -MT"$(@)" -std=gnu9x $(GEN_OPTS__FLAG) -o"$@" "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

Application/led_debug.o: /home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/cc26xx/debug/led_debug.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/home/juwan/workspace_v12/bim_onchip_LP_CC2651P3_nortos_ticlang" -I"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/include" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/flash/no_rtos/extFlash" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/startup_files" -DDeviceFamily_CC26X1 -DSET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID=0x56000 -DBIM_ONCHIP -DxSECURITY -DxDEBUG_BIM -gdwarf-3 -fshort-enums -munaligned-access -funsigned-char -fcommon -ffunction-sections -fdata-sections -march=armv7e-m -MMD -MP -MF"Application/$(basename $(<F)).d_raw" -MT"$(@)" -std=gnu9x $(GEN_OPTS__FLAG) -o"$@" "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

Application/startup_ticlang.o: /home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/startup_files/startup_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/home/juwan/workspace_v12/bim_onchip_LP_CC2651P3_nortos_ticlang" -I"/home/juwan/ti/ti_cgt_tiarmclang_1.3.0.LTS/include" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/common/flash/no_rtos/extFlash" -I"/home/juwan/ti/simplelink_cc13xx_cc26xx_sdk_7_10_01_24/source/ti/devices/cc13x1_cc26x1/startup_files" -DDeviceFamily_CC26X1 -DSET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID=0x56000 -DBIM_ONCHIP -DxSECURITY -DxDEBUG_BIM -gdwarf-3 -fshort-enums -munaligned-access -funsigned-char -fcommon -ffunction-sections -fdata-sections -march=armv7e-m -MMD -MP -MF"Application/$(basename $(<F)).d_raw" -MT"$(@)" -std=gnu9x $(GEN_OPTS__FLAG) -o"$@" "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


