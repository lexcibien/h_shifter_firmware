#pragma once
// NOLINTBEGIN
// Plataforma e execução
#define CFG_TUSB_MCU   OPT_MCU_RP2040
#define CFG_TUSB_OS    OPT_OS_PICO
#define CFG_TUSB_DEBUG 0

// Apenas USB device nativo; USB host desativado
#define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
#define CFG_TUH_ENABLED       0

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN TU_ATTR_ALIGNED(4)

// Endpoint de controle
#define CFG_TUD_ENDPOINT0_SIZE 64

// Classes utilizadas pelo firmware
#define CFG_TUD_CDC 1
#define CFG_TUD_HID 1

// Classes não utilizadas
#define CFG_TUD_MSC             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0
#define CFG_TUD_VIDEO           0
#define CFG_TUD_VIDEO_STREAMING 0
#define CFG_TUD_NCM             0
#define CFG_TUD_AUDIO           0
#define CFG_TUD_BTH             0
#define CFG_TUD_DFU             0
#define CFG_TUD_DFU_RUNTIME     0

// Buffers iniciais conservadores
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 256
#define CFG_TUD_CDC_EP_BUFSIZE 64
#define CFG_TUD_HID_EP_BUFSIZE 64
// NOLINTEND
