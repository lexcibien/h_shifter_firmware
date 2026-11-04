#pragma once

// Versões de hardware/lógica disponíveis.
#define SHIFTER_PROJECT_V1 1
#define SHIFTER_PROJECT_V2 2

// O preset selecionado no CMake define a versão do projeto.
#ifndef SHIFTER_PROJECT_VERSION
#error "Select a CMake preset: waveshare_rp2040_zero_v1 or waveshare_rp2040_zero_v2"
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
#define SHIFTER_BUTTON_COUNT 12U
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
#define SHIFTER_BUTTON_COUNT 13U
#else
#error "SHIFTER_PROJECT_VERSION must be SHIFTER_PROJECT_V1 or SHIFTER_PROJECT_V2"
#endif

// O relatório HID ocupa sempre uma quantidade inteira de bytes.
#define SHIFTER_HID_REPORT_SIZE_BYTES ((SHIFTER_BUTTON_COUNT + 7U) / 8U)
#define SHIFTER_HID_PADDING_COUNT     ((SHIFTER_HID_REPORT_SIZE_BYTES * 8U) - SHIFTER_BUTTON_COUNT)
