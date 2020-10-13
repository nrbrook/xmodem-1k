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
    xmodemErrorUnexpectedResponse = -5,
    xmodemErrorBufferFull = -6
} xmodemError;

/**
 * Receive data
 * @param dest The destination address for received data
 * @param destsz The capacity of the buffer at the destination address
 * @param bufferFullCallback A callback which is called when received data has filled the destination buffer. The
 *                           application can process/store the data and return 1 to continue with xmodem receive or 0
 *                           to fail with buffer full error. If 1 is returned, the pointer at `dest` will be overwritten
 *                           with new data.
 * @return If 0 or positive, the length of received data. If negative, a xmodemError
 */
int xmodemReceive(unsigned char *dest, int destsz, int (*bufferFullCallback)(void));

/**
 * Transmit data
 * @param getBufferCallback A callback used to obtain a data buffer from the application. Should return a pointer to a
 *                          buffer (or NULL if no more data to send) and set the size pointer to the size of the
 *                          returned buffer.
 * @return If positive, the size of data sent; if negative, an error code defined in the
 *         xmodemError enum.
 */
int xmodemTransmit(unsigned char const * (*getBufferCallback)(int *size));

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
extern void (*xmodemOutByte)(unsigned char c);

#ifdef __cplusplus
}
#endif

#endif //XMODEM_H
