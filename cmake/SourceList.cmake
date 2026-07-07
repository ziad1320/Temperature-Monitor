file(GLOB_RECURSE SRC_FILES  ${PROJECT_PATH}/*.c)

if (NOT USE_HAL)
    list(FILTER SRC_FILES EXCLUDE REGEX ".*/(.*(build|STM32-base).*)/.*")
else ()
    list(FILTER SRC_FILES EXCLUDE REGEX ".*/(.*(build).*)|STM32-base/.*")
    set(EXCLUDED_FILES
            "${PROJECT_PATH}/STM32-base-STM32Cube/HAL/${SERIES_FOLDER}/src/stm32f4xx_hal_timebase_tim_template.c"
            "${PROJECT_PATH}/STM32-base-STM32Cube/HAL/${SERIES_FOLDER}/src/stm32f4xx_hal_timebase_rtc_wakeup_template.c"
            "${PROJECT_PATH}/STM32-base-STM32Cube/HAL/${SERIES_FOLDER}/src/stm32f4xx_hal_timebase_rtc_alarm_template.c"
            "${PROJECT_PATH}/STM32-base-STM32Cube/HAL/${SERIES_FOLDER}/src/stm32f4xx_hal_msp_template.c"
    )
    list(REMOVE_ITEM SRC_FILES ${EXCLUDED_FILES})
endif ()

set(SOURCE_LIST ${SOURCE_LIST} ${SRC_FILES}
        ${PROJECT_PATH}/STM32-base-STM32Cube/CMSIS/${SERIES_FOLDER}/src/system_${SERIES_FOLDER}.c)
