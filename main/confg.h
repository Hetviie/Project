/* header file that contains configurations and pins for uart */

/* configuration parameters */
#define MY_UART_BAUD_RATE  115200          /* uart baud rate */
#define MY_UART_PORT_NUM   UART_NUM_0      /* uart port number */
#define BUF_SIZE           1024            /* buffer size of receiving buffer */
#define INTR_FLAG           0               /* default interrupt flag */

/* UART pins */
#define MY_TXD  43                      /* tx pin */
#define MY_RXD  44                      /* rx pin */
#define MY_RTS  UART_PIN_NO_CHANGE      /* no RTS */
#define MY_CTS  UART_PIN_NO_CHANGE      /* no CTS */

