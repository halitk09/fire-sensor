#include "uart_rx.h"

#include "main.h"

#include <string.h>

#define UART_RX_RING_SIZE 256U

static uint8_t s_rx_byte = 0U;
static uint8_t s_ring[UART_RX_RING_SIZE];
static volatile uint16_t s_head = 0U;
static volatile uint16_t s_tail = 0U;

extern UART_HandleTypeDef huart1;

void UartRx_Init(void)
{
  s_head = 0U;
  s_tail = 0U;
  (void)memset(s_ring, 0, sizeof(s_ring));
  (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
}

void UartRx_OnByteReceived(uint8_t byte)
{
  uint16_t next = (uint16_t)((s_head + 1U) % UART_RX_RING_SIZE);
  if (next != s_tail)
  {
    s_ring[s_head] = byte;
    s_head = next;
  }
}

uint8_t UartRx_GetLine(char *out, uint16_t out_len)
{
  uint16_t idx = 0U;
  uint8_t got_line = 0U;

  if ((out == 0) || (out_len < 2U))
  {
    return 0U;
  }

  while (s_tail != s_head)
  {
    char c = (char)s_ring[s_tail];
    s_tail = (uint16_t)((s_tail + 1U) % UART_RX_RING_SIZE);

    if ((c == '\r') || (c == '\n'))
    {
      if (idx > 0U)
      {
        got_line = 1U;
        break;
      }
      continue;
    }

    if (idx < (uint16_t)(out_len - 1U))
    {
      out[idx++] = c;
    }
  }

  if (got_line)
  {
    out[idx] = '\0';
    return 1U;
  }

  return 0U;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if ((huart != NULL) && (huart->Instance == USART1))
  {
    UartRx_OnByteReceived(s_rx_byte);
    (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
  }
}
