/*
 * LWM_MSSY.c
 *
 * Created: 6.4.2017 15:42:46
 * Author : Krajsa
 */ 

#include <avr/io.h>
/*- Includes ---------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "hal.h"
#include "phy.h"
#include "sys.h"
#include "nwk.h"
#include "sysTimer.h"
#include "halBoard.h"
#include "halUart.h"
#include "main.h"

#include "spi.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <compat/deprecated.h>  //sbi, cbi etc..
#include "avr/wdt.h" // WatchDog

#include "globals.h" //Global definitions for project

#include "stdbool.h"
#include "stack/eth/socket.h"
#include "stack/eth/wizchip_conf.h"
#include "stack/app/loopback/loopback.h"

#include "stack/mqtt/mqtt_interface.h"
#include "stack/mqtt/MQTTClient.h"

/*- Definitions ------------------------------------------------------------*/
#ifdef NWK_ENABLE_SECURITY
#define APP_BUFFER_SIZE     (NWK_MAX_PAYLOAD_SIZE - NWK_SECURITY_MIC_SIZE)
#else
#define APP_BUFFER_SIZE     NWK_MAX_PAYLOAD_SIZE
#endif

#define _MAIN_DEBUG_

//***********Prologue for fast WDT disable & and save reason of reset/power-up: BEGIN
uint8_t mcucsr_mirror __attribute__ ((section (".noinit")));

// This is for fast WDT disable & and save reason of reset/power-up
void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init3")));
void get_mcusr(void)
{
  mcucsr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}
//***********Prologue for fast WDT disable & and save reason of reset/power-up: END

/*- Types ------------------------------------------------------------------*/
typedef enum AppState_t
{
	APP_STATE_INITIAL,
	APP_STATE_IDLE,
} AppState_t;

/*- Prototypes -------------------------------------------------------------*/
static void appSendData(void);

/*- Variables --------------------------------------------------------------*/
static AppState_t appState = APP_STATE_INITIAL;
static SYS_Timer_t appTimer;
static NWK_DataReq_t appDataReq;
static bool appDataReqBusy = false;
static uint8_t appDataReqBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBufferPtr = 0;

FILE uart_str = FDEV_SETUP_STREAM( HAL_UartPrintChar , NULL, _FDEV_SETUP_RW);


#define TICK_PER_SEC 1000UL
volatile unsigned long _millis; // for millis tick !! Overflow every ~49.7 days

//*********Program metrics
const char compile_date[] PROGMEM    = __DATE__;     // Mmm dd yyyy - ???? ??????????
const char compile_time[] PROGMEM    = __TIME__;     // hh:mm:ss - ????? ??????????
const char str_prog_name[] PROGMEM   = "\r\nAtMega1284p v1.2a Static IP MQTT && Loop-back WIZNET_5500 ETHERNET 07/04/2019\r\n"; // Program name

#if defined(__AVR_ATmega128__)
const char PROGMEM str_mcu[] = "ATmega128"; //CPU is m128
#elif defined (__AVR_ATmega2560__)
const char PROGMEM str_mcu[] = "ATmega2560"; //CPU is m2560
#elif defined (__AVR_ATmega2561__)
const char PROGMEM str_mcu[] = "ATmega2561"; //CPU is m2561
#elif defined (__AVR_ATmega328P__)
const char PROGMEM str_mcu[] = "ATmega328P"; //CPU is m328p
#elif defined (__AVR_ATmega32U4__)
const char PROGMEM str_mcu[] = "ATmega32u4"; //CPU is m32u4
#elif defined (__AVR_ATmega644P__)
const char PROGMEM str_mcu[] = "ATmega644p"; //CPU is m644p
#elif defined (__AVR_ATmega1284P__)
const char PROGMEM str_mcu[] = "ATmega1284p"; //CPU is m1284p
#else
const char PROGMEM str_mcu[] = "Unknown CPU"; //CPU is unknown
#endif

//******************* MQTT: BEGIN
#define SOCK_MQTT       2
// Receive Buffer
#define MQTT_BUFFER_SIZE	512     // 2048
uint8_t mqtt_readBuffer[MQTT_BUFFER_SIZE];
volatile uint16_t mes_id;

#define PUBLISH_ANALOG_0         "sta/analog/0"
#define PUBLISH_DIGITAL_0         "sta/digital/0"
#define PUBLISH_AVR_DEBUG         "/w5500_avr_dbg"

/*- Implementations --------------------------------------------------------*/


/*-LWM----------------------------------------------------------------------*/
static void appDataConf(NWK_DataReq_t *req)
{
appDataReqBusy = false;
(void)req;
}


static void appSendData(void)
{
if (appDataReqBusy || 0 == appUartBufferPtr)
return;

memcpy(appDataReqBuffer, appUartBuffer, appUartBufferPtr);

appDataReq.dstAddr = 1-APP_ADDR;
appDataReq.dstEndpoint = APP_ENDPOINT;
appDataReq.srcEndpoint = APP_ENDPOINT;
appDataReq.options = NWK_OPT_ENABLE_SECURITY;
appDataReq.data = appDataReqBuffer;
appDataReq.size = appUartBufferPtr;
appDataReq.confirm = appDataConf;
NWK_DataReq(&appDataReq);

appUartBufferPtr = 0;
appDataReqBusy = true;
}


void HAL_UartBytesReceived(uint16_t bytes)
{
for (uint16_t i = 0; i < bytes; i++)
{
uint8_t byte = HAL_UartReadByte();

if (appUartBufferPtr == sizeof(appUartBuffer))
appSendData();

if (appUartBufferPtr < sizeof(appUartBuffer))
appUartBuffer[appUartBufferPtr++] = byte;
}

SYS_TimerStop(&appTimer);
SYS_TimerStart(&appTimer);
}


static void appTimerHandler(SYS_Timer_t *timer)
{
appSendData();
(void)timer;
}


static bool appDataInd(NWK_DataInd_t *ind)
{
for (uint8_t i = 0; i < ind->size; i++)
HAL_UartWriteByte(ind->data[i]);
return true;
}


static void appInit(void)
{
NWK_SetAddr(APP_ADDR);
NWK_SetPanId(APP_PANID);
PHY_SetChannel(APP_CHANNEL);
#ifdef PHY_AT86RF212
PHY_SetBand(APP_BAND);
PHY_SetModulation(APP_MODULATION);
#endif
PHY_SetRxState(true);

NWK_OpenEndpoint(APP_ENDPOINT, appDataInd);

HAL_BoardInit();

appTimer.interval = APP_FLUSH_TIMER_INTERVAL;
appTimer.mode = SYS_TIMER_INTERVAL_MODE;
appTimer.handler = appTimerHandler;
}


static void APP_TaskHandler(void)
{
switch (appState)
{
case APP_STATE_INITIAL:
{
appInit();
appState = APP_STATE_IDLE;
} break;

case APP_STATE_IDLE:
break;

default:
break;
}
}



/*-MQTT------------------------------------------------------------------------*/

void messageArrived(MessageData* md)
{
	char _topic_name[64] = "\0";
	char _message[128] = "\0";

	MQTTMessage* message = md->message;
	MQTTString* topic = md->topicName;
	strncpy(_topic_name, topic->lenstring.data, topic->lenstring.len);
	strncpy(_message, message->payload, message->payloadlen);
	PRINTF("<<MQTT Sub: [%s] %s", _topic_name , _message);

	//md->topicName->
	/*
	  for (uint8_t i = 0; i < md->topicName->lenstring.len; i++)
		putchar(*(md->topicName->lenstring.data + i));

	  printf(" (%.*s)\r\n", (int32_t)message->payloadlen, (char*)message->payload);
	 */
}

void mqtt_pub(Client* mqtt_client, char * mqtt_topic, char * mqtt_msg, int mqtt_msg_len)
{
	static uint32_t mqtt_pub_count = 0;
	static uint8_t mqtt_err_cnt = 0;
	int32_t mqtt_rc;

	wdt_reset();

	PRINTF(">>MQTT pub msg ?%lu ", ++mqtt_pub_count);
	MQTTMessage pubMessage;
	pubMessage.qos = QOS0;
	pubMessage.id = mes_id++;
	pubMessage.payloadlen = (size_t)mqtt_msg_len;
	pubMessage.payload = mqtt_msg;
	mqtt_rc = MQTTPublish(mqtt_client, mqtt_topic , &pubMessage);
	//Analize MQTT publish result (for MQTT failover mode)
	if (mqtt_rc == SUCCESSS)
	{
		mqtt_err_cnt  = 0;
		PRINTF(" - OK\r\n");
	}
	else
	{
		PRINTF(" - ERROR\r\n");
		//Reboot device after 20 continuous errors (~ 20sec)
		if(mqtt_err_cnt++ > 20)
		{
			PRINTF("Connection with MQTT Broker was lost!!\r\nReboot the board..\r\n");
			while(1);
		}
	}
}


/*-IDK--------------------------------------------------------------------------*/

//FUNC headers
static void avr_init(void);
void timer0_init(void);


//Wiznet FUNC headers
void print_network_information(void);

// RAM Memory usage test
int freeRam (void)
{
	extern int __heap_start, *__brkval;
	int v;
	int _res = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
	return _res;
}


//******************* MILLIS ENGINE: BEGIN
//ISR (TIMER0_COMP_vect )
ISR (TIMER0_COMPA_vect)
{
	// Compare match Timer0
	// Here every 1ms
	_millis++; // INC millis tick
	// ???? ?????? ??? ? ???? ? ??????????
	// 500Hz FREQ OUT
	// LED_TGL;
}

unsigned long millis(void)
{
	unsigned long i;
	cli();
	// Atomic tick reading
	i = _millis;
	sei();
	return i;
}


/*-ADC-----------------------------------------------------------------------------*/

#ifndef ADC_DIV
//12.5MHz or over use this ADC reference clock
#define ADC_DIV (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0) //:128 ADC Prescaler
#endif

#ifndef ADC_REF
// vcc voltage ref default
#define ADC_REF (1<<REFS0)
#endif

void adc_init(void)
{
	ADCSRA = 0;
	ADCSRA |= (ADC_DIV);    // ADC reference clock
	ADMUX |= (ADC_REF);     // Voltage reference
	ADCSRA |= (1<<ADEN);    // Turn on ADC
	ADCSRA |= (1<<ADSC);    // Do an initial conversion because this one is the
	// slowest and to ensure that everything is up
	// and running
}

uint16_t adc_read(uint8_t channel)
{
	ADMUX &= 0b11100000;                    //Clear the older channel that was read
	ADMUX |= channel;                //Defines the new ADC channel to be read
	ADCSRA |= (1<<ADSC);                //Starts a new conversion
	while(ADCSRA & (1<<ADSC));            //Wait until the conversion is done

	return ADCW;                    //Returns the ADC value of the chosen channel
}

/*-WIZCHIP----------------------------------------------------------------------------*/
#define SOCK_TCPS       0
#define SOCK_UDPS       1
#define PORT_TCPS		5000
#define PORT_UDPS       3000

#define ETH_MAX_BUF_SIZE	LOOPBACK_DATA_BUF_SIZE

unsigned char ethBuf0[ETH_MAX_BUF_SIZE];
unsigned char ethBuf1[ETH_MAX_BUF_SIZE];

void cs_sel() {
	SPI_WIZNET_ENABLE();
}

void cs_desel() {
	SPI_WIZNET_DISABLE();
}

uint8_t spi_rb(void) {
	uint8_t rbuf;
	//HAL_SPI_Receive(&hspi1, &rbuf, 1, HAL_MAX_DELAY);
	SPI_READ(rbuf);
	return rbuf;
}

void spi_wb(uint8_t b) {
	//HAL_SPI_Transmit(&hspi1, &b, 1, HAL_MAX_DELAY);
	SPI_WRITE(b);
}

void spi_rb_burst(uint8_t *buf, uint16_t len) {
	//HAL_SPI_Receive_DMA(&hspi1, buf, len);
	//while(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_RX);
	for (uint16_t var = 0; var < len; var++) {
		SPI_READ(*buf++);
	}
}

void spi_wb_burst(uint8_t *buf, uint16_t len) {
	//HAL_SPI_Transmit_DMA(&hspi1, buf, len);
	//while(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX);
	for (uint16_t var = 0; var < len; var++) {
		SPI_WRITE(*buf++);
	}
}

void IO_LIBRARY_Init(void) {
	uint8_t bufSize[] = {2, 2, 2, 2, 2, 2, 2, 2};

	reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
	reg_wizchip_spi_cbfunc(spi_rb, spi_wb);
	reg_wizchip_spiburst_cbfunc(spi_rb_burst, spi_wb_burst);

	wizchip_init(bufSize, bufSize);
	wizchip_setnetinfo(&netInfo);
	//wizchip_setinterruptmask(IK_SOCK_0);
}

/*-TIMER------------------------------------------------------------------------*/

// Timer0
// 1ms IRQ
// Used for millis() timing
void timer0_init(void)
{
	/*
	 *
	 * For M128
	TCCR0 = (1<<CS02)|(1<<WGM01); //TIMER0 SET-UP: CTC MODE & PS 1:64
	OCR0 = 249; // 1ms reach for clear (16mz:64=>250kHz:250-=>1kHz)
	TIMSK |= 1<<OCIE0;	 //IRQ on TIMER0 output compare
	 */
	//For M664p
	TCCR0A = (1<<WGM01); //TIMER0 SET-UP: CTC MODE
	TCCR0B = (1<<CS01)|(1<<CS00); // PS 1:64
	OCR0A = 249; // 1ms reach for clear (16mz:64=>250kHz:250-=>1kHz)
	TIMSK0 |= 1<<OCIE0A;	 //IRQ on TIMER0 output compareA
}


/*-AVR------------------------------------------------------*/

static void avr_init(void)
{
	// Initialize device here.
	// WatchDog INIT
	wdt_enable(WDTO_8S);  // set up wdt reset interval 2 second
	wdt_reset(); // wdt reset ~ every <2000ms

	timer0_init();// Timer0 millis engine init

	// Initial UART Peripheral
	/*
	 *  Initialize uart11 library, pass baudrate and AVR cpu clock
	 *  with the macro
	 *  uart1_BAUD_SELECT() (normal speed mode )
	 *  or
	 *  uart1_BAUD_SELECT_DOUBLE_SPEED() ( double speed mode)
	 */
/*
#if	(UART_BAUD_RATE == 115200)
	uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) ); // To works without error on 115200 bps/F_CPU=16Mhz
#else
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
#endif
	// Define Output/Input Stream
	stdout = &uart0_stdout;
*/
	//ADC init
	adc_init();
	adc_read(0); //Dummy read


	led1_conf();
	led1_low();// LED1 is OFF


	sw1_conf();//SW1 internal pull-up

	sei(); //re-enable global interrupts

	return;
}


void print_network_information(void)
{

	uint8_t tmpstr[6] = {0,};
	ctlwizchip(CW_GET_ID,(void*)tmpstr); // Get WIZCHIP name
	PRINTF("\r\n=======================================\r\n");
	PRINTF(" WIZnet chip:  %s \r\n", tmpstr);
	PRINTF("=======================================\r\n");

	wiz_NetInfo gWIZNETINFO;
	wizchip_getnetinfo(&gWIZNETINFO);
	if (gWIZNETINFO.dhcp == NETINFO_STATIC)
	PRINTF("STATIC IP\r\n");
	else
	PRINTF("DHCP IP\r\n");
	PRINTF("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n\r",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
	PRINTF("IP address : %d.%d.%d.%d\n\r",gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
	PRINTF("SM Mask	   : %d.%d.%d.%d\n\r",gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
	PRINTF("Gate way   : %d.%d.%d.%d\n\r",gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
	PRINTF("DNS Server : %d.%d.%d.%d\n\r",gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
}

void test_w5500_version(void) {
	uint8_t version;

	// Select the W5500 (CS low)
	cs_sel();

	// Read the version register (address 0x00 for W5500 version)
	version = spi_rb();  // Read version register

	// Deselect the W5500 (CS high)
	cs_desel();

	// Print the version register value
	printf("W5500 Version: %d\n", version);
}

// Function to test SPI communication
void test_spi_communication(void) {
	uint8_t test_data = 0xAA;  // Test data

	// Select the W5500 (CS low)
	cs_sel();

	// Write the test data
	SPI_WRITE(test_data);

	// Read back the data (just to check if the SPI is working)
	uint8_t read_back = spi_rb();

	// Deselect the W5500 (CS high)
	cs_desel();

	// Print the read value
	printf("Read back value: 0x%02X\n", read_back);
}


// W5500 reset function
void w5500_reset(void) {
	// Disable chip select (CS = High) to ensure no SPI communication during reset
	cs_sel();

	// Perform the reset (this may be specific to your chip and the reset procedure)
	// Example: Writing 0xFF to reset or another known reset sequence for W5500
	spi_wb(0xFF);  // Adjust based on actual reset command for W5500
	_delay_ms(10);    // Optional delay for reset stabilization (adjust timing as needed)

	// Re-enable chip select (CS = Low) to begin communication again
	cs_desel();
}

/*-MAIN------------------------------------------------------------------------*/
int main(void)
{
	SYS_Init();
	HAL_UartInit(38400);
	stdout = &uart_str;

	avr_init();
	spi_init();
	w5500_reset();
	test_w5500_version();
	test_spi_communication();
	return 0;

	// Print program metrics
	PRINTF("%S", str_prog_name);// ???????? ?????????
	PRINTF("Compiled at: %S %S\r\n", compile_time, compile_date);// ????? ???? ??????????
	PRINTF(">> MCU is: %S; CLK is: %luHz\r\n", str_mcu, F_CPU);// MCU Name && FREQ
	PRINTF(">> Free RAM is: %d bytes\r\n", freeRam());

	//Wizchip WIZ5500 Ethernet initialize
	IO_LIBRARY_Init();
	print_network_information();

	//Short Blink LED 3 times on startup
	/*unsigned char i = 3;
		while(i--)
		{
			led1_high();
			_delay_ms(100);
			led1_low();
			_delay_ms(400);
			wdt_reset();
		}*/

	//Find MQTT broker and connect with it
	uint8_t mqtt_buf[100];
	int32_t mqtt_rc = 0;
	Network mqtt_network;
	Client mqtt_client;
	mqtt_network.my_socket = SOCK_MQTT;

	// ????? ?????????? IP ???? ?? DNS-?????, IP ???? ????? ? ??????? targetIP
	//DNS_init(1, tempBuffer);
	//DNS_run(gWIZNETINFO.dns, "test.mosquitto.org", targetIP);

	PRINTF(">>Trying connect to MQTT broker: %d.%d.%d.%d ..\r\n", MQTT_targetIP[0], MQTT_targetIP[1], MQTT_targetIP[2], MQTT_targetIP[3]);
	NewNetwork(&mqtt_network);
	ConnectNetwork(&mqtt_network, MQTT_targetIP, 1883);
	MQTTClient(&mqtt_client, &mqtt_network, 1000, mqtt_buf, 100, mqtt_readBuffer, MQTT_BUFFER_SIZE);

	//Connection to MQTT broker
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.willFlag = 0;
	data.MQTTVersion = 4;//3;
	data.clientID.cstring = (char*)"w5500_avr_client";
	data.username.cstring = (char*)"user1234";
	data.password.cstring = (char*)"\0";
	data.keepAliveInterval = 60;
	data.cleansession = 1;
	mqtt_rc = MQTTConnect(&mqtt_client, &data);
	if (mqtt_rc == SUCCESSS)
	{
		PRINTF("++MQTT Connected SUCCESS: %ld\r\n", mqtt_rc);
	}
	else
	{
		PRINTF("--MQTT Connected ERROR: %ld\r\n", mqtt_rc);
		while(1);//Reboot the board
	}

	// Subscribe to all topics
	char SubString[] = "/#";// Subscribe for all that begin from "/"
	mqtt_rc = MQTTSubscribe(&mqtt_client, SubString, QOS0, messageArrived);
	PRINTF("Subscribed (%s) %d\r\n", SubString, mqtt_rc);


	uint32_t timer_link_1sec = millis();
	uint32_t timer_uptime_60sec = millis();
	uint32_t timer_mqtt_pub_1sec = millis();


/*-MAIN LOOP------------------------------------------------------------------------------------------*/

	while (1)
	{
	SYS_TaskHandler();
	HAL_UartTaskHandler();
	APP_TaskHandler();

	//Here at least every 1sec
	wdt_reset(); // WDT reset at least every sec

	//Use Hercules Terminal to check loopback tcp:5000 and udp:3000
	//https://www.hw-group.com/software/hercules-setup-utility
	loopback_tcps(SOCK_TCPS,ethBuf0,PORT_TCPS);
	loopback_udps(SOCK_UDPS,ethBuf0,PORT_UDPS);

	// MQTT pub event every 1 sec
	if((millis()-timer_mqtt_pub_1sec)> 1000)
	{
		//here every 1 sec
		timer_mqtt_pub_1sec = millis();
		static uint8_t mqtt_10sec_cnt =0;
		static char _msg[64] = "\0";
		static int _len;

		//Every 1sec send status POT A6 (ADC input)
		_len = SPRINTF(_msg, "%u", adc_read(6));
		if(_len > 0)
		{
			mqtt_pub(&mqtt_client, PUBLISH_ANALOG_0, _msg, _len);
		}

		// && SW1 (GPIO input)
		uint16_t val = sw1_read()?0:!0;
		_len = SPRINTF(_msg, "%u", val);
		if(_len > 0)
		{
			mqtt_pub(&mqtt_client, PUBLISH_DIGITAL_0, _msg, _len);
		}

		//Every 10sec public message: "Uptime: xxx sec; Free RAM: xxxxx bytes" to "/w5500_avr_dbg"
		if(++mqtt_10sec_cnt>9)
		{
			mqtt_10sec_cnt = 0;
			_len = SPRINTF(_msg, "Uptime: %lu sec; Free RAM: %d bytes\r\n", millis()/1000, freeRam());
			if(_len > 0)
			{
				mqtt_pub(&mqtt_client, PUBLISH_AVR_DEBUG, _msg, _len);
			}
		}

	}

	// MQTT broker connection and sub receive
	MQTTYield(&mqtt_client, 100);//~100msec blocking here


	if((millis()-timer_link_1sec)> 1000)
	{
		//here every 1 sec
		timer_link_1sec = millis();
		if(wizphy_getphylink() == PHY_LINK_ON)
		{
			led1_high();
		}
		else
		{
			led1_low();
		}
	}

	if((millis()-timer_uptime_60sec)> 60000)
	{
		//here every 60 sec
		timer_uptime_60sec = millis();
	#ifdef CHK_RAM_LEAKAGE
				//Printout RAM usage every 1 minute
   				PRINTF(">> Free RAM is: %d bytes\r\n", freeRam());
	#endif

	#ifdef CHK_UPTIME
				//Printout RAM usage every 1 minute
   				PRINTF(">> Uptime %lu sec\r\n", millis()/1000);
	#endif
			}
		}
	return 0;
}
