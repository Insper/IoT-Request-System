#include "asf.h"
#include "main.h"
#include <string.h>
#include "bsp/include/nm_bsp.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "util.h"

/************************************************************************/
/* WIFI                                                                 */
/************************************************************************/

/** IP address of host. */
uint32_t gu32HostIp = 0;

/** TCP client socket handlers. */
static SOCKET tcp_client_socket = -1;

/** Receive buffer definition. */
static uint8_t g_receivedBuffer[MAIN_WIFI_M2M_BUFFER_SIZE] = {0};
static uint8_t g_sendBuffer[MAIN_WIFI_M2M_BUFFER_SIZE] = {0};

/** Wi-Fi status variable. */
static bool gbConnectedWifi = false;

/** Get host IP status variable. */
/** Wi-Fi connection state */
static uint8_t wifi_connected;

/** Instance of HTTP client module. */
static bool gbHostIpByName = false;

/** TCP Connection status variable. */
static bool gbTcpConnection = false;
static bool gbTcpConnected = false;

/** Server host name. */
static char server_host_name[] = MAIN_SERVER_NAME;

#define LED_PIO_ID ID_PIOC
#define LED_PIO PIOC
#define LED_PIN 8
#define LED_IDX_MASK (1 << LED_PIN)

#define BUT_PIO      PIOA
#define BUT_PIO_ID   ID_PIOA
#define BUT_IDX      11
#define BUT_IDX_MASK (1 << BUT_IDX)
#define BUT_PRIOR 4

#define AFEC_POT AFEC1
#define AFEC_POT_ID ID_AFEC1
#define AFEC_POT_CHANNEL 1 // Canal do pino PC13

SemaphoreHandle_t xSemaphoreButton;

typedef struct
{
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t seccond;
} calendar;

calendar rtc_initial;

/************************************************************************/
/* AFEC                                                               */
/************************************************************************/
QueueHandle_t xQueueADC, xQueuePlot;

typedef struct {
	uint value;
} adcData;

static void AFEC_pot_Callback(void){
  adcData adc;
  adc.value = afec_channel_get_value(AFEC_POT, AFEC_POT_CHANNEL);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(xQueueADC, &adc, &xHigherPriorityTaskWoken);
  
}

static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel, afec_callback_t callback){
  /*************************************
  * Ativa e configura AFEC
  *************************************/
  /* Ativa AFEC - 0 */
  afec_enable(afec);

  /* struct de configuracao do AFEC */
  struct afec_config afec_cfg;

  /* Carrega parametros padrao */
  afec_get_config_defaults(&afec_cfg);

  /* Configura AFEC */
  afec_init(afec, &afec_cfg);

  /* Configura trigger por software */
  afec_set_trigger(afec, AFEC_TRIG_SW);

  /*** Configuracao específica do canal AFEC ***/
  struct afec_ch_config afec_ch_cfg;
  afec_ch_get_config_defaults(&afec_ch_cfg);
  afec_ch_cfg.gain = AFEC_GAINVALUE_0;
  afec_ch_set_config(afec, afec_channel, &afec_ch_cfg);

  /*
  * Calibracao:
  * Because the internal ADC offset is 0x200, it should cancel it and shift
  down to 0.
  */
  afec_channel_set_analog_offset(afec, afec_channel, 0x200);

  /***  Configura sensor de temperatura ***/
  struct afec_temp_sensor_config afec_temp_sensor_cfg;

  afec_temp_sensor_get_config_defaults(&afec_temp_sensor_cfg);
  afec_temp_sensor_set_config(afec, &afec_temp_sensor_cfg);
  
  /* configura IRQ */
  afec_set_callback(afec, afec_channel,	callback, 1);
  NVIC_SetPriority(afec_id, 4);
  NVIC_EnableIRQ(afec_id);
}

/************************************************************************/
/* TC                                                               */
/************************************************************************/
void TC1_Handler(void){
	volatile uint32_t ul_dummy;

	ul_dummy = tc_get_status(TC0, 1);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/* Selecina canal e inicializa conversão */
	afec_channel_enable(AFEC_POT, AFEC_POT_CHANNEL);
	afec_start_software_conversion(AFEC_POT);
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	pmc_enable_periph_clk(ID_TC);

	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	NVIC_SetPriority((IRQn_Type) ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);

	tc_start(TC, TC_CHANNEL);
}


/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_WIFI_STACK_SIZE      (6*4096/sizeof(portSTACK_TYPE))
#define TASK_WIFI_PRIORITY        (1)
#define TASK_PROCESS_STACK_SIZE   (4*4096/sizeof(portSTACK_TYPE))
#define TASK_PROCESS_PRIORITY     (0)

SemaphoreHandle_t xSemaphore;
QueueHandle_t xQueueMsg;

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

TaskHandle_t xHandleWifi = NULL;

/************************************************************************/
/* HOOKs                                                                */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName){
  printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
  for (;;) {  }
}

extern void vApplicationIdleHook(void){}

extern void vApplicationTickHook(void){}

extern void vApplicationMallocFailedHook(void){
  configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type)
{
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.seccond);
}

/************************************************************************/
/* callbacks                                                            */
/************************************************************************/

void but_callback(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreButton, &xHigherPriorityTaskWoken);
}

/**
* \brief Callback function of IP address.
*
* \param[in] hostName Domain name.
* \param[in] hostIp Server IP.
*
* \return None.
*/
static void resolve_cb(uint8_t *hostName, uint32_t hostIp)
{
  gu32HostIp = hostIp;
  gbHostIpByName = true;
  printf("resolve_cb: %s IP address is %d.%d.%d.%d\r\n\r\n", hostName,
  (int)IPV4_BYTE(hostIp, 0), (int)IPV4_BYTE(hostIp, 1),
  (int)IPV4_BYTE(hostIp, 2), (int)IPV4_BYTE(hostIp, 3));
}

/**
* \brief Callback function of TCP client socket.
*
* \param[in] sock socket handler.
* \param[in] u8Msg Type of Socket notification
* \param[in] pvMsg A structure contains notification informations.
*
* \return None.
*/
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
  /* Check for socket event on TCP socket. */
  if (sock == tcp_client_socket) {

    switch (u8Msg) {
      case SOCKET_MSG_CONNECT:
      {
        printf("socket_msg_connect\n");
        if (gbTcpConnection) {
          tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
          if (pstrConnect && pstrConnect->s8Error >= SOCK_ERR_NO_ERROR) {
            printf("socket_cb: connect ok \n");
            gbTcpConnected = true;
            } else {
            printf("socket_cb: connect error!\r\n");
            gbTcpConnection = false;
            gbTcpConnected = false;
            close(tcp_client_socket);
            tcp_client_socket = -1;
          }
        }
      }
      break;

      case SOCKET_MSG_RECV:
      {
        char *pcIndxPtr;
        char *pcEndPtr;

        tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
        if (pstrRecv && pstrRecv->s16BufferSize > 0) {
          xQueueSend(xQueueMsg, &pstrRecv, 10);
          xSemaphoreGive( xSemaphore );
          }  else {
          //printf("socket_cb: recv error!\r\n");
          close(tcp_client_socket);
          tcp_client_socket = -1;
        }
      }
      break;

      default:
      break;
    }
  }
}

/**
* \brief Callback to get the Wi-Fi status update.
*
* \param[in] u8MsgType Type of Wi-Fi notification.
* \param[in] pvMsg A pointer to a buffer containing the notification parameters.
*
* \return None.
*/
char identifier[20];
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
  switch (u8MsgType) {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
      tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
      if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
        printf("wifi_cb: M2M_WIFI_CONNECTED\r\n");
        m2m_wifi_request_dhcp_client();
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
        printf("wifi_cb: M2M_WIFI_DISCONNECTED\r\n");
        gbConnectedWifi = false;
        wifi_connected = 0;
      }

      break;
    }

    case M2M_WIFI_REQ_DHCP_CONF:
    {
	  vTaskDelay(10);
      uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
      printf("wifi_cb: IP address is %u.%u.%u.%u\r\n",
      pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
	  sprintf(identifier, "ID=%u.%u.%u.%u", pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
      wifi_connected = M2M_WIFI_CONNECTED;
      break;
    }

    case M2M_WIFI_RESP_GET_SYS_TIME:
    /*Initial first callback will be provided by the WINC itself on the first communication with NTP */
    {
      tstrSystemTime *strSysTime_now = (tstrSystemTime *)pvMsg;

      /* Print the hour, minute and second.
      * GMT is the time at Greenwich Meridian.
      */
	  rtc_initial.year = strSysTime_now->u16Year;
	  rtc_initial.month = strSysTime_now->u8Month;
	  rtc_initial.week = 0;
	  rtc_initial.day = 0;
	  rtc_initial.hour = strSysTime_now->u8Hour-3;
	  rtc_initial.minute = strSysTime_now->u8Minute;
	  rtc_initial.seccond = strSysTime_now->u8Second;
	  
      printf("socket_cb: Year: %d, Month: %d, The GMT time is %u:%02u:%02u\r\n",
      strSysTime_now->u16Year,
      strSysTime_now->u8Month,
      strSysTime_now->u8Hour ,    /* hour (86400 equals secs per day) */
      strSysTime_now->u8Minute,  /* minute (3600 equals secs per minute) */
      strSysTime_now->u8Second); /* second */
      break;
    }

    default:
    {
      break;
    }
  }
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

void format_get(char *p, char s[]) {
	sprintf(p,"GET %s HTTP/1.1\r\n Accept: */*\r\n\r\n", s);
}

static void task_process(void *pvParameters) {

  printf("task process created \n");
  vTaskDelay(1000);
  
  adcData adc;
  xQueueADC  = xQueueCreate( 200, sizeof( adcData ) );
  
  // configura ADC e TC para controlar a leitura
  config_AFEC_pot(AFEC_POT, AFEC_POT_ID, AFEC_POT_CHANNEL, AFEC_pot_Callback);
  TC_init(TC0, ID_TC1, 1, 1);
  
  RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
  
  uint32_t hour;
  uint32_t minute;
  uint32_t second;

  uint msg_counter = 0;
  tstrSocketRecvMsg *p_recvMsg;

  enum states {
    WAIT = 0,
    GET,
	POST,
    ACK,
    MSG,
    TIMEOUT,
    DONE,
  };
  
  int contentLength;
  char analogico[15];
  char digital[10];
  char tempo[15];
  char POSTDATA[50];
  char led = 0;

  enum states state = WAIT;

  while(1){ 

    switch(state){
      case WAIT:
      // aguarda task_wifi conectar no wifi e socket estar pronto
      printf("STATE: WAIT \n");
      while(gbTcpConnection == false && tcp_client_socket >= 0){
        vTaskDelay(10);
       }
      state = POST;
      break;

      case GET:
      printf("STATE: GET \n");
      //sprintf((char *)g_sendBuffer, MAIN_PREFIX_BUFFER);
	  format_get(g_sendBuffer, "/status");
      send(tcp_client_socket, g_sendBuffer, strlen((char *)g_sendBuffer), 0);
      state = ACK;
      break;
	  
	  case POST:
	  if(xQueueReceive( xQueueADC, &(adc), 100)){
		  sprintf(analogico,"POT=%d", (int) adc.value);
	  }
	  
	  if (xSemaphoreTake(xSemaphoreButton, (TickType_t)0) == pdTRUE)
	  {
		  led = !led;
	  }
	  
	  sprintf(digital, "LED=%d", led);
	  
	  rtc_get_time(RTC, &hour, &minute, &second);
	  sprintf(tempo, "tempo=%02d:%02d:%02d", hour, minute, second);
	  
	  sprintf(POSTDATA, "%s&%s&%s&%s", digital, tempo, identifier, analogico);
	  printf("postdata: %s\n", POSTDATA);
	  
	  printf("STATE: POST \n");
	  contentLength = strlen(POSTDATA);
	  sprintf((char *)g_sendBuffer, "POST /status HTTP/1.1\nContent-Type: application/x-www-form-urlencoded\nContent-Length: %d\n\n%s",
	  contentLength, POSTDATA);
	  send(tcp_client_socket, g_sendBuffer, strlen((char *)g_sendBuffer), 0);
	  state = ACK;
	  break;

      case ACK:
      printf("STATE: ACK \n");
      memset(g_receivedBuffer, 0, MAIN_WIFI_M2M_BUFFER_SIZE);
      recv(tcp_client_socket, &g_receivedBuffer[0], MAIN_WIFI_M2M_BUFFER_SIZE, 0);

      if(xQueueReceive(xQueueMsg, &p_recvMsg, 5000) == pdTRUE){
        printf(STRING_LINE);
        printf(p_recvMsg->pu8Buffer);
        printf(STRING_EOL);  printf(STRING_LINE);
        state = MSG;
      }
      else {
        state = TIMEOUT;
      };
      break;

      case MSG:
      printf("STATE: MSG \n");
      memset(g_receivedBuffer, 0, MAIN_WIFI_M2M_BUFFER_SIZE);
      recv(tcp_client_socket, &g_receivedBuffer[0], MAIN_WIFI_M2M_BUFFER_SIZE, 0);

      if(xQueueReceive(xQueueMsg, &p_recvMsg, 5000) == pdTRUE){
        printf(STRING_LINE);
        printf(p_recvMsg->pu8Buffer);
        printf(STRING_EOL);  
		printf(STRING_LINE);
        state = DONE;
      }
      else {
        state = TIMEOUT;
      };
      break;

      case DONE:
      printf("STATE: DONE \n");

      state = WAIT;
      break;

      case TIMEOUT:
      state = WAIT;
      break;

      default: state = WAIT;
    }
  }
}

static void task_wifi(void *pvParameters) {
  tstrWifiInitParam param;
  struct sockaddr_in addr_in;

  xSemaphore = xSemaphoreCreateCounting(20,0);
  xQueueMsg = xQueueCreate(10, sizeof(tstrSocketRecvMsg));

  /* Initialize the BSP. */
  nm_bsp_init();

  /* Initialize Wi-Fi parameters structure. */
  memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

  /* Initialize Wi-Fi driver with data and status callbacks. */
  param.pfAppWifiCb = wifi_cb;
  int8_t ret = m2m_wifi_init(&param);
  if (M2M_SUCCESS != ret) {
    printf("main: m2m_wifi_init call error!(%d)\r\n", ret);
    while (1) { }
  }

  /* Initialize socket module. */
  socketInit();

  /* Register socket callback function. */
  registerSocketCallback(socket_cb, resolve_cb);

  /* Connect to router. */
  printf("main: connecting to WiFi AP %s...\r\n", (char *)MAIN_WLAN_SSID);
  m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);

  /* formata ip */
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = _htons(MAIN_SERVER_PORT);
  inet_aton(MAIN_SERVER_NAME, &addr_in.sin_addr);

  printf(STRING_LINE);

  while(1){
    vTaskDelay(50);
    m2m_wifi_handle_events(NULL);

    if (wifi_connected == M2M_WIFI_CONNECTED) {
      /* Open client socket. */
      if (tcp_client_socket < 0) {
        printf(STRING_LINE);
        printf("socket init \n");
        if ((tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          printf("main: failed to create TCP client socket error!\r\n");
          continue;
        }

        /* Connect server */
        printf("socket connecting\n");
        if (connect(tcp_client_socket, (struct sockaddr *)&addr_in,
        sizeof(struct sockaddr_in)) != SOCK_ERR_NO_ERROR) {
          close(tcp_client_socket);
          tcp_client_socket = -1;
          printf("main: error connect to socket\n");
          }else{
          gbTcpConnection = true;
        }
      }
    }
  }
}

void LEDs_init(void){
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_IDX_MASK, 0, 0, 0);
	
	xSemaphoreButton = xSemaphoreCreateBinary();

	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_configure(BUT_PIO, PIO_INPUT, BUT_IDX_MASK, PIO_PULLUP);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_IDX_MASK, PIO_IT_FALL_EDGE, but_callback);
	pio_enable_interrupt(BUT_PIO, BUT_IDX_MASK);
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4); // Prioridade 4

	if (xSemaphoreButton == NULL)
		printf("falha em criar o semaforo \n");
};

int main(void)
{
  /* Initialize the board. */
  sysclk_init();
  board_init();
  

  /* Initialize the UART console. */
  configure_console();
  LEDs_init();
  printf(STRING_HEADER);

  xTaskCreate(task_wifi, "Wifi", TASK_WIFI_STACK_SIZE, NULL, TASK_WIFI_PRIORITY, &xHandleWifi);
  xTaskCreate(task_process, "process", TASK_PROCESS_STACK_SIZE, NULL, TASK_PROCESS_PRIORITY,  NULL );

  vTaskStartScheduler();


  while(1) {
	  };
  return 0;
}
