// Firmware para H-shifter (Arduino Leonardo) - C/C++ Puro com Registradores AVR
// Implementação nativa em ATmega32U4 sem abstrações Arduino
// Lê 4 entradas do câmbio e apresenta 10 botões virtuais ao PC como gamepad.
// Compilação: avr-gcc com flags para ATmega32U4 16MHz

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

// ============================================================================
// CONSTANTES DO SISTEMA
// ============================================================================

#define BAUD 115200
#define UBRR_VAL (F_CPU / (16UL * BAUD) - 1)

// ============================================================================
// ESTRUTURAS USB HID
// ============================================================================

// Descritor de Configuração USB (simplificado para Device + Config + Interface + Endpoint)
using USB_ConfigDescriptor_t = struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t MaxPower;
};

using USB_InterfaceDescriptor_t = struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
};

using USB_EndpointDescriptor_t = struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
};

using USB_HIDDescriptor_t = struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdHID;
  uint8_t bCountryCode;
  uint8_t bNumDescriptors;
  uint8_t bDescriptorType2;
  uint16_t wDescriptorLength;
};

// Buffer HID Report (2 bytes para 16 botões)
using HID_GamepadReport_t = struct {
  uint8_t button_state_low;
  uint8_t button_state_high;
};


// ============================================================================
// DEFINIÇÕES DE PINOS (ATmega32U4 - PORTD, PORTB, PORTC, PORTE, PORTF)
// ============================================================================

// Pinos físicos do Arduino Leonardo:
//   PIN_BTN4  : PD4 (Digital 4)
//   PIN_BTN5  : PC6 (Digital 5)
//   PIN_BTN6  : PD7 (Digital 6)
//   PIN_BTN7  : PE6 (Digital 7)
//   PIN_BTN8  : PB4 (Digital 8)
//   PIN_BTN9  : PB5 (Digital 9)
//   PIN_BTN10 : PB6 (Digital 10)
//   PIN_BTN11 : PB7 (Digital 11)

#define PIN_BTN4_PORT   PORTD
#define PIN_BTN4_PIN    PIND
#define PIN_BTN4_DDR    DDRD
#define PIN_BTN4_BIT    4

#define PIN_BTN5_PORT   PORTC
#define PIN_BTN5_PIN    PINC
#define PIN_BTN5_DDR    DDRC
#define PIN_BTN5_BIT    6

#define PIN_BTN6_PORT   PORTD
#define PIN_BTN6_PIN    PIND
#define PIN_BTN6_DDR    DDRD
#define PIN_BTN6_BIT    7

#define PIN_BTN7_PORT   PORTE
#define PIN_BTN7_PIN    PINE
#define PIN_BTN7_DDR    DDRE
#define PIN_BTN7_BIT    6

#define PIN_BTN8_PORT   PORTB
#define PIN_BTN8_PIN    PINB
#define PIN_BTN8_DDR    DDRB
#define PIN_BTN8_BIT    4

#define PIN_BTN9_PORT   PORTB
#define PIN_BTN9_PIN    PINB
#define PIN_BTN9_DDR    DDRB
#define PIN_BTN9_BIT    5

#define PIN_BTN10_PORT  PORTB
#define PIN_BTN10_PIN   PINB
#define PIN_BTN10_DDR   DDRB
#define PIN_BTN10_BIT   6

#define PIN_BTN11_PORT  PORTB
#define PIN_BTN11_PIN   PINB
#define PIN_BTN11_DDR   DDRB
#define PIN_BTN11_BIT   7

// ============================================================================
// MACROS PARA MANIPULAÇÃO DE REGISTRADORES
// ============================================================================

#define SET_BIT(port, bit)     ((port) |= (1 << (bit)))
#define CLR_BIT(port, bit)     ((port) &= ~(1 << (bit)))
#define TST_BIT(port, bit)     (((port) >> (bit)) & 1)
#define TOG_BIT(port, bit)     ((port) ^= (1 << (bit)))

// ============================================================================
// ENUMERAÇÃO DE BOTÕES
// ============================================================================

enum class ControllerButtons {
  GEAR_1 = 0,
  GEAR_2 = 1,
  GEAR_3 = 2,
  GEAR_4 = 3,
  GEAR_5 = 4,
  GEAR_6 = 5,
  GEAR_R = 6,
  SW_SPLIT = 7,
  SW_RANGE = 8,
  BTN_ENGINE_BRAKE = 9
};

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

volatile uint16_t gamepad_button_state = 0;
volatile uint16_t gamepad_button_state_prev = 0xFFFF;
volatile uint8_t usb_ready = 0;
volatile uint32_t system_ticks = 0;
bool handle_connected = false;

// ============================================================================
// INICIALIZAÇÃO UART (PARA DEBUG)
// ============================================================================

void uart_init(void) {
  // Configurar UBRR para 115200 baud
  UBRR1 = UBRR_VAL;
  
  // Habilitar TX e RX
  UCSR1B = (1 << TXEN1) | (1 << RXEN1);
  
  // 8 bits de dados, 1 stop bit, sem paridade
  UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
}

void uart_putchar(char c) {
  while (!(UCSR1A & (1 << UDRE1))) {/*  */}
  UDR1 = c;
}

void uart_puts(const char *str) {
  while (*str) {
    uart_putchar(*str++);
  }
}

// ============================================================================
// INICIALIZAÇÃO USB (REGISTRADORES BÁSICOS)
// ============================================================================

void usb_init(void) {
  // Habilitar Pad USB e Regulator
  UHWCON = (1 << UVREGE);
  
  // Configurar oscilador USB de 48MHz
  PLLCSR = (1 << PINDIV) | (1 << PLLE);
  
  // Esperar PLL estar pronta
  while (!(PLLCSR & (1 << PLOCK))) {/*  */}
  
  // Habilitar controlador USB
  USBCON = (1 << USBE) | (1 << OTGPADE);
  
  // Reset USB
  USBCON |= (1 << FRZCLK);
  USBCON &= ~(1 << FRZCLK);
  
  // Configurar modo Device e ativar endpoint 0
  UDCON &= ~(1 << DETACH);
  USBINT = 0xFF; // Limpar pending interrupts
  UDINT = 0xFF;  // Limpar pending device interrupts
  
  // Habilitar interrupção de reset USB
  UDIEN = (1 << EORSTI);
  
  usb_ready = 1;
}

// ============================================================================
// INTERRUPÇÃO DE RESET USB
// ============================================================================

ISR(USB_COM_vect) {
  if (UDINT & (1 << EORSTI)) {
    UDINT = ~(1 << EORSTI);
    
    // Reset endpoint 0
    UENUM = 0;
    UECONX = (1 << EPEN);
    UECFG0X = (0 << EPTYPE1) | (0 << EPTYPE0); // Control endpoint
    UECFG1X = (1 << EPSIZE0) | (0 << EPBK0);  // 64-byte endpoint, single bank
  }
}

// ============================================================================
// TIMER 1 PARA TICKS DO SISTEMA (1ms)
// ============================================================================

void timer1_init(void) {
  // Mode 4: CTC (Clear Timer on Compare)
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // Prescaler 64
  OCR1A = 249; // (16MHz / 64) / 1000 - 1 = 249 para 1ms
  TIMSK1 = (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  system_ticks = system_ticks + 1;
}

// ============================================================================
// INICIALIZAÇÃO DE PINOS
// ============================================================================

void gpio_init(void) {
  // Configurar botões como entrada com pull-up
  CLR_BIT(PIN_BTN4_DDR, PIN_BTN4_BIT);
  SET_BIT(PIN_BTN4_PORT, PIN_BTN4_BIT);
  
  CLR_BIT(PIN_BTN5_DDR, PIN_BTN5_BIT);
  SET_BIT(PIN_BTN5_PORT, PIN_BTN5_BIT);
  
  CLR_BIT(PIN_BTN6_DDR, PIN_BTN6_BIT);
  SET_BIT(PIN_BTN6_PORT, PIN_BTN6_BIT);
  
  CLR_BIT(PIN_BTN7_DDR, PIN_BTN7_BIT);
  SET_BIT(PIN_BTN7_PORT, PIN_BTN7_BIT);
  
  CLR_BIT(PIN_BTN8_DDR, PIN_BTN8_BIT);
  SET_BIT(PIN_BTN8_PORT, PIN_BTN8_BIT);
  
  CLR_BIT(PIN_BTN9_DDR, PIN_BTN9_BIT);
  SET_BIT(PIN_BTN9_PORT, PIN_BTN9_BIT);
  
  CLR_BIT(PIN_BTN10_DDR, PIN_BTN10_BIT);
  SET_BIT(PIN_BTN10_PORT, PIN_BTN10_BIT);
  
  CLR_BIT(PIN_BTN11_DDR, PIN_BTN11_BIT);
  SET_BIT(PIN_BTN11_PORT, PIN_BTN11_BIT);
}

// ============================================================================
// LEITURA DE PINOS
// ============================================================================

static inline uint8_t read_pin_btn4(void) {
  return !TST_BIT(PIN_BTN4_PIN, PIN_BTN4_BIT);
}

static inline uint8_t read_pin_btn5(void) {
  return !TST_BIT(PIN_BTN5_PIN, PIN_BTN5_BIT);
}

static inline uint8_t read_pin_btn6(void) {
  return !TST_BIT(PIN_BTN6_PIN, PIN_BTN6_BIT);
}

static inline uint8_t read_pin_btn7(void) {
  return !TST_BIT(PIN_BTN7_PIN, PIN_BTN7_BIT);
}

static inline uint8_t read_pin_btn8(void) {
  return !TST_BIT(PIN_BTN8_PIN, PIN_BTN8_BIT);
}

static inline uint8_t read_pin_btn9(void) {
  return !TST_BIT(PIN_BTN9_PIN, PIN_BTN9_BIT);
}

static inline uint8_t read_pin_btn10(void) {
  return !TST_BIT(PIN_BTN10_PIN, PIN_BTN10_BIT);
}

static inline uint8_t read_pin_btn11(void) {
  return !TST_BIT(PIN_BTN11_PIN, PIN_BTN11_BIT);
}

// ============================================================================
// ENVIO DE DADOS PELO ENDPOINT 1 (INTERRUPT)
// ============================================================================

void usb_send_gamepad_report(uint16_t button_state) {
  HID_GamepadReport_t report;
  report.button_state_low = (uint8_t)(button_state & 0xFF);
  report.button_state_high = (uint8_t)((button_state >> 8) & 0xFF);
  
  // Selecionar endpoint 1
  UENUM = 1;
  
  // Verificar se endpoint está pronto
  if (!(UEINTX & (1 << TXINI))) {
    return;
  }
  
  // Enviar dados
  UEDATX = report.button_state_low;
  UEDATX = report.button_state_high;
  
  // Marcar como ready para transmissão
  UEINTX &= ~(1 << TXINI);
}

// ============================================================================
// FUNÇÃO PRINCIPAL SETUP
// ============================================================================

void setup(void) {
  // Desabilitar interrupts
  cli();
  
  // Inicializar UART para debug
  uart_init();
  uart_puts("H-Shifter Firmware Init\r\n");
  
  // Inicializar GPIO
  gpio_init();
  uart_puts("GPIO Init OK\r\n");
  
  // Inicializar Timer 1
  timer1_init();
  uart_puts("Timer1 Init OK\r\n");
  
  // Aguardar estabilização
  for (uint16_t i = 0; i < 2000; i++) {
    _delay_ms(1);
  }
  
  // Detectar se manopla está conectada (AD5)
  // Leitura simples: se btn9 não está 0, provavelmente conectado
  uint16_t adc_readings = 0;
  for (uint8_t i = 0; i < 10; i++) {
    if (read_pin_btn9() == 0) {
      adc_readings++;
    }
    _delay_ms(10);
  }
  
  if (adc_readings > 3) {
    handle_connected = true;
    uart_puts("OK: Handle detected\r\n");
  } else {
    handle_connected = false;
    uart_puts("WARN: Truck shifter handle not connected\r\n");
  }
  
  // Inicializar USB
  usb_init();
  uart_puts("USB Init OK\r\n");
  
  // Habilitar interrupts globais
  sei();
  
  uart_puts("Setup Complete\r\n");
}

// ============================================================================
// FUNÇÃO PRINCIPAL LOOP
// ============================================================================

void main_loop(void) {
  static uint32_t last_send_time = 0;
  
  // Ler estado dos botões
  uint8_t btn4 = read_pin_btn4();
  uint8_t btn5 = read_pin_btn5();
  uint8_t btn6 = read_pin_btn6();
  uint8_t btn7 = read_pin_btn7();
  uint8_t btn8 = read_pin_btn8();
  uint8_t btn9 = handle_connected && read_pin_btn9();
  uint8_t btn10 = handle_connected && read_pin_btn10();
  uint8_t btn11 = handle_connected && read_pin_btn11();
  
  // Resetar estado dos botões
  uint16_t new_button_state = 0;
  uint8_t comb4_used = 0;
  uint8_t comb7_used = 0;
  
  // ========== LÓGICA DE COMBINAÇÕES DE MARCHAS ==========
  
  if (btn4 && btn5) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::GEAR_1));
    comb4_used = 1;
  }
  if (btn5 && btn7) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::GEAR_2));
    comb7_used = 1;
  }
  if (btn4 && btn6) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::GEAR_5));
    comb4_used = 1;
  }
  if (btn6 && btn7) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::GEAR_6));
    comb7_used = 1;
  }
  
  // Marchas centrais
  if (btn4 && !comb4_used) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::GEAR_3));
  }
  if (btn7 && !comb7_used) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::GEAR_4));
  }
  
  // Marcha ré
  if (btn8 && comb4_used) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::GEAR_R));
  }
  
  // ========== BOTÕES DA MANOPLA DE CAMINHÃO ==========
  if (btn9) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::SW_SPLIT));
  }
  if (btn10) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::SW_RANGE));
  }
  if (btn11) {
    new_button_state |= (1 << static_cast<uint8_t>(ControllerButtons::BTN_ENGINE_BRAKE));
  }
  
  // ========== ENVIAR ESTADO SE MUDOU ==========
  if ((new_button_state != gamepad_button_state_prev) || 
      (system_ticks - last_send_time >= 20)) {
    gamepad_button_state = new_button_state;
    gamepad_button_state_prev = new_button_state;
    usb_send_gamepad_report(new_button_state);
    last_send_time = system_ticks;
  }
}

// ============================================================================
// ENTRY POINT
// ============================================================================

int main(void) {
  setup();
  
  while (true) {
    main_loop();
    _delay_ms(10);
  }
  
  return 0;
}