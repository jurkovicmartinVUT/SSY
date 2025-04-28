
# MPC-SSY

# LWM -> MQTT gateway

Finální verze projektu je umístěna ve složce **Project_final**.

Projekt byl realizován na mikrokontroléru **ATmega256RFR2**.

## Cíl
Cílem projektu je vytvořit gateway pro protokol **LWM**, která bude obsah (payload) z LWM zpráv odesílat pomocí **MQTT**. Zároveň bude přijímat MQTT zprávy pro konfiguraci.

### Breakdown
Implementace v rámci projektu:

- **UART** -> zejména pro pohodlnější debugování
- **WizNet 5500** -> podpora Ethernetu (pro MQTT)
- **SPI** -> komunikace s w5500 chipem
- **LWM** stack -> fungování LWM protokolu
- **MQTT** stack -> fungování MQTT protokolu
- **DHCP** stack -> síťová konfigurace s využitím DHCP

## Výchozí projekty
Z níže uvedených implementací bylo vycházeno při řešení tohoto projektu.

- MSSY cvičení 5 -> LWM + UART
- https://github.com/maxxir/m1284p_wiz5500/tree/master/22_m1284p_WIZNET_MQTT -> MQTT + WizNet
- https://github.com/maxxir/m1284p_wiz5500/tree/master/04_m1284p_WIZNET_loopback_DHCP -> DHCP (+ WizNet)

## Vlastní implementace

### Výchozí bod
Výchozím bodem při řešení byt projekt z 5. laboratorního cvičení s implementovaným **LWM** stackem a funkčním **UARTem**. Funkcionality UARTU bylo využito pro přesměrování standardního výstupu programu = "printf" výpisy jsou odesílány přes UART.

### WizNet 5500
Modul WizNet 5500 dovoluje komunikaci s využitím technologie **Ethernet**. Drivery pro tento modul byly převzaty z implementace [MQTT](https://github.com/maxxir/m1284p_wiz5500/tree/master/22_m1284p_WIZNET_MQTT). Využité řešení ovšem bylo vytvořeno pro jinou řadu mikrokontroléru (konkrétně ATMEGA 1284p) a proto bylo nutné provést **úpravy nastavení SPI komunikace** (WizNet komunikuje s mikrokontrolérem přes SPI).

*Upravené zapojení pinů pro SPI komunkaci na EXT2*

| W5500 Pin | Pin na desce |
|-----------|--------------|
| SCLK      | PB1          |
| SCS       | PD4          |
| MOSI      | PB2          |
| MISO      | PB3          |
| INT       | NC (nepodstatné)|
| RST       | NC (nepodstatné)|

### MQTT
Implementace MQTT byla převzata ze stejného projektu, jako [WizNet](https://github.com/maxxir/m1284p_wiz5500/tree/master/22_m1284p_WIZNET_MQTT).

### DHCP
Řešení pro využití [DHCP](https://github.com/maxxir/m1284p_wiz5500/tree/master/04_m1284p_WIZNET_loopback_DHCP) se nachází ve stejném github repozitáři, jako MQTT implementace = Ethernet (WizNet) byl nakonfigurován shodným způsobem. Do řešení projektu tedy byla importována pouze podpora DHCP.

### Obecná logika
Obecnou logiku programu bych popsal následovně.

V první fázi proběhne **základní inicializace**:
- UART
- LWM
- Watchdog
- Časovač
- A/D převodník

Následuje výpis vybraných vlastností programu. Poté proběhne **inicializace w5500** a **inicializace proměnných** pro další chod programu:
- Rozhodnutí použití statické / dynamické síťové konfigurace
- MQTT
- Časovače pro jedntlivé části programu

Nyní se v programu nachází **hlavní smyčka**, ve které se provádí:
- Obsluha LWM událostí
- DHCP konfigurace (s výpisem síťového konfigurace)
- Spuštění lokální TCP/UDP testovací aplikace
- MQTT připojení k brokerovi + odeslání subscribe zprávy
- Pravidelná kontrola času běhu a volné paměti

Při přijmu LWM zprávy je její obsah odeslán na UART (výpis) + je z této zprávy přečtena délka obsahu. Samotný obsah + jeho délka jsou vloženy do MQTT publish zprávy.

### Hlavní konfigurace
Hlavní konfigurace probíhá v následujících souborech:
- config.h -> UART a LWM
- stack/hal/drivers/atmega256rfr2/inc/spi.h -> SPI
- globals.h -> trochu od všeho
- globals.c -> IP (včetně IP MQTT brokera)
- main.c -> trochu od všeho

*UART + LWM konfigurace*
```
#define UART_SPEED 115200

// Address must be set to 0 for the first device, and to 1 for the second one.
#define APP_ADDR                  0
#define APP_PANID                 0x80
#define APP_ENDPOINT              3
#define APP_FLUSH_TIMER_INTERVAL  20
```

*SPI konfigurace (spi.h)*
```
#define SCK            1  /* - Output: SPI Serial Clock (SCLK) - ATMEGA644/1284 PORTB, PB1 */
#define MOSI           2  /* - Output: SPI Master out - slave in (MOSI) -  ATMEGA644/1284 PORTB, PB2 */
#define MISO           3  /* - Input:  SPI Master in - slave out (MISO) -  ATMEGA644/1284 PORTB, PB3 */
#define CSN            4  /*SPI - SS*, PD4/

#define WIZNET_CS       4       /* PD4 Output as CS for Wiznet ETHERNET*/
#define SPI_WIZNET_ENABLE()  ( PORTD &= ~BV(WIZNET_CS) )
#define SPI_WIZNET_DISABLE() ( PORTD |=  BV(WIZNET_CS) )
```

*SPI konfigurace (spi.c) (až moc jsem se natrápil, než jsem přišel na to, že je třeba změnit port i tady :))*
```
// CS PIN for FLASH
DDRD	|= _BV(WIZNET_CS); // CS to OUT && Disable
SPI_WIZNET_DISABLE();
```

*IP konfigurace (využita v případě statické konfigurace)*
```
wiz_NetInfo netInfo = { .mac  = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}, // Mac address
		.ip   = {192, 168, 53, 201},         // IP address
		.sn   = {255, 255, 255, 0},         // Subnet mask
		.dns =  {8,8,8,8},			  // DNS address (google dns)
		.gw   = {192, 168, 53, 1}, // Gateway address
		//.dhcp = NETINFO_STATIC};    //Static IP configuration
		.dhcp = NETINFO_DHCP};
uint8_t MQTT_targetIP[4] = {192, 168, 0, 100};      // IP брокера MQTT
```

## Závěr
Jsem si vědom faktu, že kód projektu je do vysoké míry nepřehledný z důvodu importování vélkého množství skriptů bez nějakého velkého formátování nebo čištění. Současně se může na první pohled zdát vzájemná provázanost skriptů špatně napsána (includy). Já s tímto tvrzením naprosto souhlasím, ale v současné podobě šel projekt zkompilovat. Při snaze o includy, které by respektovaly skutečnou strukturu projektu, kompilace skončila chybou právě v těchto includech. Důvod tohoto chování se skrývá někde v atmel studiu.

### Funkčnost
Projekt, jako celek, není zcela odladěn a funkčnost nebyla zcela vyzkoušena.

*Co bylo úspěšně odzkoušeno*
- UART
- LWM příjem dat (byl jsem schopen zachytit zprávy vysílané kolegy v laboratoři)
- WizNet 5500 -> komunikace chipu s mikrokontrolérem -> Ethernet
- Síťová konfigurace -> statická / dynamická

*Co nebylo zcela otestováno/odladěno*
- MQTT připojení k brokerovi -> mikrokontrolér se obecně o připojení pokoušel, ale k žádnému konkrétnímu spojení nedošlo
- Skutečná funkčnost přeposílání LWM payloadu do MQTT zpráv -> implementační kód je přítomen, ale nebyl otestován
- Koexistence DHCP a MQTT -> nebylo zcela odladěno fungování těchto 2 protokolů současně -> podařilo se mi docítil buď DHCP konfigurace nebo konektivity MQTT -> problémem by mohlo být špatné časování inicializatí těchto dvou protokolů (snažil jsem se řešit, ale neúspěšně)

*LWM -> MQTT*
```
static bool appDataInd(NWK_DataInd_t *ind)
{
for (uint8_t i = 0; i < ind->size; i++)
	HAL_UartWriteByte(ind->data[i]);

// Send the payload to the MQTT
lwm_mqtt_send(ind->data, ind->size);

return true;
}
```

### Co si odnáším
Projekt mi přinest alespoň základní představu o SPI komunikaci a MQTT protokolu, což nebylo náplní standarních cvičení. Jelikož nemám nějaké zásadní zkušenosti s programováním mikrokontrolérů, jsem rád, že jsem si toto programování mohl v rámci výuky vyzkoušet a jak se říká "trochu osahat". Dojmy to ve mně zanechalo takové, že programování mikrokontrolérů s největší pravděpodobností nebude zrovna můj šálek kávy. Debugování takového programu bylo ve většině případů značně nemilé. I přes mou snahu dát projekt dohromady, ho nemůžu označit za 100 % funkční, ale určitě jsem uvítal zkušenost tohoto charakteru.
