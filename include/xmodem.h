//
// Created by Nick Brook on 06/12/2019.
//

#ifndef XMODEM_H
#define XMODEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XMODEM_MAXRETRANS
#define XMODEM_MAXRETRANS 25
#endif

//#define XMODEM_TRANSMIT_1K

typedef enum {
    xmodemErrorCancelledByRemote = -1,
    xmodemErrorNoSync = -2,
    xmodemErrorTooManyRetries = -3,
    xmodemErrorTransmitError = -4,
    xmodemErrorUnexpectedResponse = -5
} xmodemError;

/**
 * Receive data
 * @param dest The destination address for received data
 * @param destsz The capacity of the buffer at the destination address
 * @return If 0 or positive, the length of received data. If negative, a xmodemError
 */
int xmodemReceive(unsigned char *dest, int destsz);

/**
 * Transmit data
 * @param src The source of the data to send
 * @param srcsz The size of data to send
 * @return The length of sent data
 */
int xmodemTransmit(unsigned char const *src, int srcsz);

/**
 * Get a received byte
 * @param timeout The timeout, in ms
 * @return The character received, if negative a xmodemError for failure
 */

extern int (*xmodemInByte)(unsigned short timeout);
/**
 * Send a byte
 * @param c The byte to send
 */
extern void (*xmodemOutByte)(char c);

#ifdef __cplusplus
}
#endif

#endif //XMODEM_H
