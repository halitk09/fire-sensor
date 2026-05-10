# fire_sensor — teknik özet
-Proje Dosya Yapısı-
fire_sensor/
├── .cproject, .project, .mxproject     # CubeIDE / Eclipse
├── fire_sensor.ioc                     # CubeMX
├── fire_sensor.launch
├── README.md
├── STM32G474RETX_FLASH.ld
├── STM32G474RETX_RAM.ld
├── .gitignore
│
├── Core/
│   ├── Inc/
│   │   ├── app.h, app_config.h, app_types.h
│   │   ├── app_state.h, app_err.h
│   │   ├── sensor_service.h, ble_transport.h, uart_rx.h
│   │   ├── main.h, stm32g4xx_it.h, stm32g4xx_hal_conf.h, stm32g4xx_nucleo_conf.h
│   └── Src/
│       ├── main.c                     
│       ├── app.c                       
│       ├── app_state.c                
│       ├── app_err.c                   
│       ├── sensor_service.c            
│       ├── ble_transport.c            
│       ├── uart_rx.c                   
│       ├── stm32g4xx_it.c              
│       ├── stm32g4xx_hal_msp.c
│       ├── system_stm32g4xx.c, syscalls.c, sysmem.c
│   └── Startup/startup_stm32g474retx.s
│
├── Docs/  
│   ├── at_ble_reference.csv
│   ├── fault_codes.csv
│   ├── pin_assignments.csv
│   └── ble_at_emulator_gui.py        
│
└── Drivers/                           
    ├── CMSIS/
    ├── STM32G4xx_HAL_Driver/
    └── BSP/STM32G4xx_Nucleo/
-Proje Amacı-

CO ve ortam sıcaklığını periyodik ölçmek, eşiklere göre bir sistem durumu üretmek ve bu bilgiyi **BLE reklamı** ile dışarı vermek. STM32 sensörleri okur; BLE harici bir modülde çalışır ve **UART üzerinden AT komutları** ile yönetilir. İş tanımı örneği: süper döngü, durum makinesi, düşük güç (WFI), zaman aşımı ve hata yönetimi.

-Kullanılan donanımlar-

| Bileşen | Not | Ürün / doküman |
|--------|-----|----------------|
| **Ana kart** | CubeMX ile üretilmiş proje; **NUCLEO-G474RE** (STM32G474RET6). | [NUCLEO-G474RE](https://www.st.com/en/evaluation-tools/nucleo-g474re.html) — [STM32G474RE datasheet](https://www.st.com/resource/en/datasheet/stm32g474re.pdf) |
| **CO sensörü** | DFRobot **SEN0466** analog çıkış → MCU **ADC1** kanalı. | [Ürün (SKU SEN0466)](https://www.dfrobot.com/product-2508.html) — [Wiki — SEN0465…SEN0476 gaz sensörleri](https://wiki.dfrobot.com/sku_sen0465tosen0476_gravity_gas_sensor_calibrated_i2c_uart) |
| **Sıcaklık** | **TMP117**, I2C adresi 0x48. | [TMP117](https://www.ti.com/product/TMP117) — [Datasheet PDF](https://www.ti.com/lit/ds/symlink/tmp117.pdf) |
| **BLE** | **USART1** ile AT komutları; firmware tarafında **ESP-AT** (veya aynı komut kümesini veren modül) varsayılır. Modül kartı (ESP32 vb.) ayrı seçilir; MCU yalnızca UART üzerinden konuşur. | [ESP-AT BLE komutları](https://docs.espressif.com/projects/esp-at/en/release-v3.3.0.0/esp32/AT_Command_Set/BLE_AT_Commands.html) |
| **Referans (MCU çevre birimleri)** | ADC, clock | [RM0440](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) — [HAL/LL UM DM00610707](https://www.st.com/resource/en/user_manual/dm00610707-description-of-stm32g4-hal-and-lowlayer-drivers-stmicroelectronics.pdf) |

-Pin ataması-

Özet tablo: **`docs/pin_assignments.csv`** (MCU sinyali, fiziksel pin: USART1 TX/RX, I2C1 SCL/SDA, ADC1 girişi).

-Yazılım Mimarisi-
-Akış Şeması-
<img width="4543" height="16384" alt="image" src="https://github.com/user-attachments/assets/479fa77f-1c8f-472f-ae39-117f23ea30c9" />


**Ana akış (`main.c` + `app.c`)**  
Açılışta `App_Init()`: `AppErr_Clear`, `AppState_Init`, `SensorService_Init`, `BleTransport_Init`. Sonsuz döngüde `App_Run()` çağrılır; ardından `HAL_PWR_EnterSLEEPMode(..., PWR_SLEEPENTRY_WFI)`. 
`App_Run` içinde üç periyotlu iş vardır:

- ~**20 ms**: `BleTransport_Process` — UART satırları, AT yanıtı, zaman aşımı / yeniden deneme.
- ~**1000 ms**: `SensorService_ReadCoPpm` / `ReadTmp117Celsius`, sayaçlar, yalnızca her iki okuma da başarılıysa `AppState_Evaluate`.
- ~**1000 ms**: `BleTransport_SendAdv` — güncel ölçüm ve durum adv kuyruğuna yazılır.

**Uygulama durumları (`AppState_t`, `app_state.c`)**

| Durum | Ne zaman | İş mantığı özeti |
|--------|------------|------------------|
| **NORMAL** | Ölçümler eşik altında veya alarmdan çıkış sonrası. | Alarm koşulu yokken `warning_condition` false ise burada kalınır. |
| **WARNING** | CO veya sıcaklık **uyarı** eşiğini aşmış; alarm için gerekli ardışık örnek henüz dolmamış; veya alarmdan düşerken hâlâ uyarı bandında. | `AppState_Evaluate` her 1 s örnekte güncellenir. |
| **ALARM** | Alarm eşiği **ardışık** `APP_CFG_ALARM_ENTER_CNT_MAX` örnek boyunca sağlanmış. | CO > alarm ppm **veya** sıcaklık > alarm °C (`app_config.h` içerisinde bu değerler belirtilmiştir). |
| **ERROR** | `AppState_ForceError`: BLE fatal veya sensör okuma üst üste başarısız. | `AppState_Evaluate` ERROR iken değişmez; reklamda hata kodu taşınır. |

**BLE katmanı (`ble_transport.c`, dosya içi enum’lar)**

| İletişim state | Anlam |
|----------------|--------|
| **IDLE** | Komut göndermeye hazır veya yanıt işlendi. |
| **WAIT_RESPONSE** | Son AT için `OK` / `ERROR` veya süre dolması bekleniyor. |
| **ERROR** (BLE içi) | Yeniden deneme tükendi; `s_fatal_error` set; `app.c` bunu `AppState_ForceError` ile uygulama **ERROR** durumuna bağlar. |

**Faz sırası (`BlePhase_t`)**  
`AT+BLEINIT=2` → `AT+BLEADVPARAM=...` → `AT+BLEADVDATA="<hex>"` → `AT+BLEADVSTART`. `OK` ile faz ilerler; `BLEADVSTART` sonrası döngü tekrar `SET_ADV_DATA` aşamasına döner. Hata veya süre aşımında aynı faz komutu `APP_CFG_BLE_MAX_RETRY` sınırına göre yeniden gönderilir; sınır aşılınca fatal ve `AppErr_Set` + `AppState_ForceError`.

-ADC okumaları-

**Sürücü**  
`HAL_ADC_Start` → `HAL_ADC_PollForConversion` (timeout `APP_CFG_ADC_POLL_TIMEOUT_MS`) → `HAL_ADC_GetValue` → `HAL_ADC_Stop`. Çözünürlük **12 bit** (`main.c` ADC init); tam ölçek kodu `APP_CFG_ADC_FULL_SCALE` (4095) ve referans `APP_CFG_CO_ADC_VREF_V` (3.3 V) `sensor_service.c` içinde kullanılır.

**Gerilim**  
`Vout = adc_raw * Vref / ADC_FULL_SCALE`

**SEN0466 doğrusal ppm** (`ConvertRawToPpm`)  
Eğim: `REF_PPM / (Vout1@ref - Vout0)`.  
`ppm = eğim * (Vout - Vout0)`, ardından 0 … `APP_CFG_CO_SEN0466_MAX_PPM` kırpma ve yuvarlama.

**Örnek (varsayılan sabitlerle)**  
`adc_raw = 818` alınsın.  
`Vout = 818 * 3.3 / 4095 ≈ 0,659 V`  
`eğim = 200 / (0,9 - 0,6) ≈ 666,7 ppm/V`  
`ppm ≈ 666,7 * (0,659 - 0,6) ≈ 39` → yaklaşık **39 ppm** .

**Filtre**  
CO için tamsayı **EMA**: `yeni = (önceki*(den-num) + anlık*num) / den`; varsayılan `num/den = 1/4` → her adımda ağırlıklı olarak **%25 yeni örnek**. Sıcaklıkta `LowPassF32` ile `alpha = 0,25`.

**Kaynaklar**  
- RM0440 — ADC bölümü: https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf  
- HAL ADC: https://www.st.com/resource/en/user_manual/dm00610707-description-of-stm32g4-hal-and-lowlayer-drivers-stmicroelectronics.pdf  
- SEN0466 volt–ppm (tabloda SEN0466 satırı): https://wiki.dfrobot.com/sku_sen0465tosen0476_gravity_gas_sensor_calibrated_i2c_uart  

-Warning ve Alert-

Kodda **Alarm** (kullanıcı “alert” dediği) ve **Warning** ayrı eşiklerle tanımlıdır (`app_config.h` varsayılanları):

**Uyarı (WARNING)**  
`CO > APP_CFG_CO_WARNING_PPM` **veya** `sıcaklık > APP_CFG_TEMP_WARNING_C` — alarm koşulu yokken bu doğruysa durum WARNING (varsayılan: 35 ppm, 45 °C).

**Alarm (ALARM)**  
`CO > APP_CFG_CO_ALARM_PPM` **veya** `sıcaklık > APP_CFG_TEMP_ALARM_C` — bu koşul **üst üste** `APP_CFG_ALARM_ENTER_CNT_MAX` (varsayılan **3**) örnek boyunca sürdüyse ALARM (varsayılan: 50 ppm, 60 °C).

**Alarmdan çıkış**  
Hem `CO < APP_CFG_CO_EXIT_PPM` hem `sıcaklık < APP_CFG_TEMP_EXIT_C` — bu çıkış koşulu **ardışık** `APP_CFG_ALARM_EXIT_CNT_MAX` (varsayılan **5**) örnek sürerse WARNING veya NORMAL’a düşülür (varsayılan çıkış: 45 ppm, 55 °C).

-Fault Durumları-

**`AppErrCode_t` (`app_err.c`, `app_types.h`)**  
`AppErr_Set` yalnızca şu an kod **NONE** iken ve yeni kod **NONE değilse** yazar; yani **ilk kaydedilen hata** sonradan üzerine yazılmaz.

| Kod | Kısa koşul |
|-----|------------|
| 0 | Hata yok. |
| 1 | BLE: modül `ERROR` veya yeniden deneme sonrası fatal. |
| 2 | BLE: `OK` gelmeden süre doldu. |
| 3 | TMP117 I2C okuma, üst üste `APP_CFG_SENSOR_HAL_FAIL_CNT_MAX` başarısız. |
| 4 | CO ADC okuma, üst üste aynı eşikte başarısız. |

Özet tablo ve dosya yolu: **`docs/fault_codes.csv`**

-Kullanılan iletişim protokolleri-

- **I2C1** — TMP117 register okuma (`HAL_I2C_Mem_Read`).  
- **USART1** — BLE modülü ile **metin tabanlı AT** (`\r\n` sonlu komutlar; yanıt satırı `OK` / `ERROR`). Alım **kesme + ring buffer** (`uart_rx.c`, `HAL_UART_RxCpltCallback`).  
- **BLE reklam yükü** — 17 bayt ham AD yapısı, `AT+BLEADVDATA` içinde **ASCII hex** string olarak gönderilir; byte byte çözümlenmesi -> **`docs/at_ble_reference.csv`**.

-ADC Mock verileri ve Python gui ile uart doğrulama-

- **Ekran görüntüsü:**
<img width="736" height="511" alt="fire_sensor_python_uart_test" src="https://github.com/user-attachments/assets/3c2c83ec-5834-4935-995c-52c28eae54de" />


- **Log:** `log.txt` *(`Docs/pythonguilog.txt` ile ulaşılabilir.)*  
- **Mock ADC:** Doğrulama sırasında CO kanalına sabit veya rampalı `adc_raw` / gerilim profili verilerek davranış kontrol edilmiştir; **bu repoda mock kodu yoktur ayrı bir zip ile iletilecektir -> repo içerisindeki fire_sensor_with_mockadcsensor_pythongui.rar zipi ile ulaşılabilir**.
