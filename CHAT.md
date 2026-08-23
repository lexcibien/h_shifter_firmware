Vou localizar como `Adafruit_USBD_HID` é usado no projeto e quais APIs o TinyUSB nativo do Pico SDK expõe, para separar o que é substituição direta do que exige uma pequena camada própria.

Searched for regex `Adafruit_USBD_HID|TinyUSBDevice|usbHid|tud_hid|hid_report` (`**/src/**`), 15 results

Search complete: Mapear APIs TinyUSB

Searched codebase for "h_shifter_firmware HidReport.cpp/.h CMakeLists.txt Arduino TinyUSB APIs", no results

Searched codebase for "TinyUSB Pico SDK equivalent APIs begin mounted ready setPollInterval setReportDescriptor sendReport detach attach", no results

Searched for files matching `**/HidReport.cpp`, 1 match

Searched for files matching `**/HidReport.h`, 1 match

Searched for files matching `**/CMakeLists.txt`, 1 match

Searched for regex `Adafruit_USBD_HID|TinyUSBDevice`, 8 results

Searched for regex `begin|mounted|ready|setPollInterval|setReportDescriptor|sendReport|detach|attach`, 38 results

Searched for files matching `**/pico-sdk/**/examples/**/*hid*`, no matches

Searched codebase for "TinyUSBDevice begin mounted ready setPollInterval setReportDescriptor sendReport detach attach implementation", 7 results

Read

Read

Searched for files matching `**/pico-sdk/**/tinyusb/**/*hid*`, no matches

Searched for files matching `**/pico-sdk/**/examples/**/*usbd*`, no matches

Searched for text `TinyUSBDevice`, 7 results

Searched for text `Adafruit_USBD_HID`, 1 result

Read

Searched for files matching `**/pico-sdk/**/tinyusb/**/device/**/*hid*`, no matches

Searched for files matching `**/pico-sdk/**/tinyusb/**/common/**/*hid*`, no matches

Searched for regex `setPollInterval\(|setReportDescriptor\(|sendReport\(`, 3 results

Searched for text `TinyUSBDevice`, 393 results

Searched for files matching `**/pico-sdk/**/examples/**/*usbd_hid*`, no matches

Searched for files matching `**/pico-sdk/**/tinyusb/**/include/**/*usbd*hid*`, no matches

Searched for text `USBD_HID`, 1 result

Read memory

O caminho decisivo já apareceu: o projeto está ligado diretamente a `tinyusb_device`, mas o cabeçalho ainda declara o wrapper `Adafruit_USBD_HID`. Vou conferir agora as assinaturas nativas (`tud_init`, callbacks HID, `tud_hid_ready`, `tud_hid_report`) e os detalhes de montagem USB no SDK instalado.

Ver APIs nativas TinyUSB

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

Read

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

Searched for files matching `h_shifter_firmware` · `build/pico-sdk/**/*{hid,usb}*.h`, no matches

Searched for files matching `h_shifter_firmware` · `build/pico-sdk/**/examples/**`, no matches

Read

Searched for regex `tud_hid_|tud_mounted|tud_init|tud_task` (`**/*.{h,c,cpp}`), 705 results

Searched for regex `Adafruit_USBD_HID|TinyUSBDevice|usbHid` (`h_shifter_firmware` · `src/**`), 13 results

Search complete: Localizar TinyUSB HID nativo

Searched codebase for "Pico SDK TinyUSB device HID headers examples APIs tud_init tud_task tud_mounted tud_hid_ready tud_hid_report tud_hid_descriptor_report_cb tud_disconnect tud_connect", 8 results

Searched for files matching `**/pico-sdk/**/tud*.{h,c}`, no matches

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

Searched for files matching `**/pico-sdk/**/device/**/hid*.{h,c}`, no matches

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

Searched codebase for "TinyUSB device HID tud_init tud_task tud_mounted tud_hid_ready tud_hid_report descriptor report callback disconnect connect", 8 results

Searched for files matching `**/pico-sdk/**/tud*.{h,c}`, no matches

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

Searched for files matching `**/pico-sdk/**/device/**/hid*.{h,c}`, no matches

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

Searched for files matching `**/pico-sdk/**`, no matches

Read

Read

Read

Read

Read

Read , lines 1 to 180

No matches found. Your search pattern might be excluded completely by either the search.exclude settings or .*ignore files.
If you believe that it should have results, you can check into the .*ignore files and the exclude setting (here are some excluded patterns for reference:[**/node_modules,**/bower_components,**/*.code-search]).
Then if you want to include those files you can call the tool again by setting "includeIgnoredFiles" to true.

No matches found.

Read , lines 35 to 150

Read , lines 1 to 150

Read , lines 1 to 130

Read , lines 1 to 80

Searched for regex `tud_descriptor_configuration_cb|tud_mount_cb|tud_umount_cb|tud_disconnect|tud_connect` (`h_shifter_firmware` · `.pio/packages/framework-arduinopico/pico-sdk/lib/tinyusb/src/{device,class}/**/*.{h,c}`), 16 results

Read , lines 1 to 80

Read , lines 55 to 115

Read , lines 70 to 145

Read , lines 1 to 15

Read , lines 1 to 120

Searched for regex `PICO_TINYUSB_PATH|TINYUSB_PATH|pico-sdk/lib/tinyusb|tinyusb_device` (`h_shifter_firmware` · `build/**`), 244 results

Searched for regex `PICO_SDK_PATH|PICO_TINYUSB_PATH` (`h_shifter_firmware` · `{CMakeLists.txt,pico_sdk_import.cmake,build/**}`), 23 results

Read , lines 120 to 175

Read , lines 130 to 179

Searched for regex `TUD_HID_DESCRIPTOR|TUD_CONFIG_DESCRIPTOR|TUD_HID_REPORT_DESC` (`h_shifter_firmware` · `.pio/packages/framework-arduinopico/pico-sdk/lib/tinyusb/src/class/hid/hid_device.h`), 14 results

Read , lines 130 to 230

No matches found.

Read

Read

Read

Read

No matches found.

No matches found.

No matches found.

Searched for regex `tud_disconnect|tud_connect|TinyUSBDevice.detach|TinyUSBDevice.attach` (`h_shifter_firmware` · `.pio/packages/framework-arduinopico/libraries/Adafruit_TinyUSB_Arduino/src/**`), 8 results

No matches found.

No matches found.

No matches found.

Read

Read

Na versão nativa do TinyUSB, você pode substituir as funções assim:

| Arduino TinyUSB | TinyUSB nativo |
| --- | --- |
| `TinyUSBDevice.isInitialized()` | `tud_inited()` ou uma flag própria |
| `TinyUSBDevice.begin(0)` | `tud_init(...)` ou `tusb_init(...)` |
| `TinyUSBDevice.task()` | `tud_task()` |
| `TinyUSBDevice.mounted()` | `tud_mounted()` |
| `usbHid.ready()` | `tud_hid_ready()` |
| `usbHid.sendReport(...)` | `tud_hid_report(...)` |
| `TinyUSBDevice.detach()` | `tud_disconnect()` |
| `TinyUSBDevice.attach()` | `tud_connect()` |

No seu `HidReport.cpp`, estas substituições seriam diretas:

```cpp
bool HidReport::mounted() {
  return tud_mounted();
}

bool HidReport::ready() {
  return tud_hid_ready();
}

void HidReport::send(const ShifterModel::ButtonState& buttonState) {
  // monta buttonReport...

  tud_hid_report(0, &buttonReport, sizeof(buttonReport));
}
```

No `main.cpp`, substitua:

```cpp
TinyUSBDevice.task();
```

por:

```cpp
tud_task();
```

## Funções que você terá de criar

O `Adafruit_USBD_HID` faz várias coisas automaticamente. No TinyUSB original, você precisa fornecer:

```cpp
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance);
```

Essa função deve retornar o seu `hidReportDescriptor`.

Também precisará criar os descritores USB:

```cpp
uint8_t const* tud_descriptor_device_cb(void);
uint8_t const* tud_descriptor_configuration_cb(uint8_t index);
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid);
```

Normalmente eles são montados usando:

```cpp
TUD_CONFIG_DESCRIPTOR(...)
TUD_HID_DESCRIPTOR(...)
```

## Funções sem substituição direta

Estas chamadas do Arduino não possuem um equivalente isolado:

```cpp
usbHid.setReportDescriptor(...);
usbHid.setPollInterval(2);
usbHid.begin();
```

Elas são substituídas pela combinação de:

1. `tud_hid_descriptor_report_cb()`;
2. descritor de configuração USB;
3. `TUD_HID_DESCRIPTOR(..., 2)`, onde `2` é o intervalo de polling;
4. inicialização da stack com `tud_init()` ou `tusb_init()`.

O intervalo de polling não é configurado em tempo de execução; ele fica gravado no descritor USB.

## Inicialização

A inicialização ficaria conceitualmente assim:

```cpp
tusb_rhport_init_t dev_init = {
  .role = TUSB_ROLE_DEVICE,
  .speed = TUSB_SPEED_AUTO
};

tusb_init(0, &dev_init);
```

A assinatura exata depende da versão do TinyUSB incluída pelo Pico SDK. Em versões mais antigas pode ser simplesmente:

```cpp
tud_init(0);
```

## Recomendação para sua estrutura

Você pode manter `HidReport`, mas remover:

```cpp
Adafruit_USBD_HID usbHid;
```

O `HidReport` ficaria responsável apenas por:

- montar o relatório;
- consultar `tud_mounted()`;
- consultar `tud_hid_ready()`;
- chamar `tud_hid_report()`.

Os callbacks USB e os descritores podem ficar em um novo arquivo, por exemplo:

```text
src/UsbDescriptors.cpp
src/UsbDescriptors.h
```

O `tusb_config.h` já habilita HID e possui buffer suficiente para seu relatório de 2 bytes. O principal trabalho adicional será criar os descritores USB e o callback `tud_hid_descriptor_report_cb()`.
