#ifndef UART_RX_H
#define UART_RX_H

#include <stdint.h>

void UartRx_Init(void);
uint8_t UartRx_GetLine(char *out, uint16_t out_len);
void UartRx_OnByteReceived(uint8_t byte);

#endif
