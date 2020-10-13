//
// Created by Nick Brook on 14/03/2020.
//

#include <sys/param.h>
#include "gtest/gtest.h"
#include "xmodem.h"

#define PRINT_WIRE_DATA 0

#if PRINT_WIRE_DATA
#define PRINT_WIRE(...) printf(__VA_ARGS__)
#else
#define PRINT_WIRE(...)
#endif

#define WIRE_BUFFER_SIZE 300
static uint8_t tx_wireBuffer[WIRE_BUFFER_SIZE];
static size_t tx_wireBufferStart = 0;
static size_t tx_wireBufferEnd = 0;
static uint8_t rx_wireBuffer[WIRE_BUFFER_SIZE];
static size_t rx_wireBufferStart = 0;
static size_t rx_wireBufferEnd = 0;
static pthread_t sendThread, receiveThread;
int xmodem_InByte(unsigned short timeout) {
    bool isSend = pthread_self() == sendThread;
    uint8_t *wireBuffer = isSend ? rx_wireBuffer : tx_wireBuffer;
    size_t *wireBufferStart = isSend ? &rx_wireBufferStart : &tx_wireBufferStart;
    size_t *wireBufferEnd = isSend ? &rx_wireBufferEnd : &tx_wireBufferEnd;
    for(int i = 0; i < timeout / 10; i++) {
        if(*wireBufferEnd != *wireBufferStart) {
            uint8_t ret = wireBuffer[*wireBufferStart];
            *wireBufferStart += 1;
            if(*wireBufferStart == WIRE_BUFFER_SIZE) {
                *wireBufferStart = 0;
            }
            PRINT_WIRE("%s op R byte 0x%02X\n", isSend ? "T" : "R", ret);
            return ret;
        }
        static const struct timespec spec = {
                .tv_sec = 0,
                .tv_nsec = 10000000L
        };
        nanosleep(&spec, NULL);
    }
    PRINT_WIRE("%s op failed to receive byte after delay %dms\n", isSend ? "T" : "R", timeout);
    return -10;
}
void xmodem_OutByte(unsigned char c) {
    bool isSend = pthread_self() == sendThread;
    uint8_t *wireBuffer = isSend ? tx_wireBuffer : rx_wireBuffer;
    size_t *wireBufferEnd = isSend ? &tx_wireBufferEnd : &rx_wireBufferEnd;
    PRINT_WIRE("%s op T byte 0x%02X\n", isSend ? "T" : "R", (unsigned char)c);
    wireBuffer[*wireBufferEnd] = (unsigned char)c;
    *wireBufferEnd += 1;
    if(*wireBufferEnd == WIRE_BUFFER_SIZE) {
        *wireBufferEnd = 0;
    }
}

static uint8_t dataToTransfer[WIRE_BUFFER_SIZE];
static int sendDataSize = 0;
static int sendBufferSize = 0;
static int sendOffset = 0;
static int sendResult = 0;
static uint8_t receiveBuffer[((WIRE_BUFFER_SIZE/128) + 2) * 128];
static uint8_t receiveOutput[sizeof(dataToTransfer)];
static int receiveBufferSize = 0;
static int receiveTotalSize = 0;
static int receiveOffset = 0;
static int receiveResult = 0;

__attribute__((constructor)) void init(void) {
    xmodemInByte = xmodem_InByte;
    xmodemOutByte = xmodem_OutByte;
    for(int i = 0; i < sizeof(dataToTransfer); i++) {
        dataToTransfer[i] = (uint8_t)i;
    }
}

static unsigned char const * getBuffer(int * size) {
    *size = MIN(sendBufferSize, sendDataSize - sendOffset);
    unsigned char const *ret = dataToTransfer + sendOffset;
    sendOffset += *size;
    return ret;
}

void * sendFunc(void* ptr) {
    sendOffset = 0;
    sendResult = xmodemTransmit(getBuffer);
    return NULL;
}

int bufferFull(void) {
    if(receiveTotalSize < receiveOffset + receiveBufferSize) {
        assert(false);
    }
    memcpy(receiveOutput + receiveOffset, receiveBuffer, MIN(receiveBufferSize, sizeof(receiveOutput) - receiveOffset));
    receiveOffset += receiveBufferSize;
    return receiveTotalSize < receiveOffset + receiveBufferSize ? 0 : 1;
}

void * receiveFunc(void* ptr) {
    receiveOffset = 0;
    receiveResult = xmodemReceive(receiveBuffer, receiveBufferSize, bufferFull);
    return NULL;
}

class xmodemTests : public ::testing::Test {
    void SetUp() override {
        tx_wireBufferStart = 0;
        tx_wireBufferEnd = 0;
        rx_wireBufferStart = 0;
        rx_wireBufferEnd = 0;
    }

    void TearDown() override {

    }
};

TEST_F(xmodemTests, testSendReceiveSuccessFullBufferShortData) {
    sendDataSize = 100;
    sendBufferSize = sizeof(dataToTransfer);
    receiveBufferSize = sizeof(receiveBuffer);
    receiveTotalSize = sizeof(receiveBuffer);
    ::pthread_create(&sendThread, nullptr, sendFunc, nullptr);
    ::pthread_create(&receiveThread, nullptr, receiveFunc, nullptr);
    ::pthread_join(sendThread, nullptr);
    ::pthread_join(receiveThread, nullptr);

    ASSERT_EQ(sendResult, sendDataSize);
    ASSERT_EQ(receiveResult, sendDataSize);
    if(receiveResult > receiveOffset) {
        memcpy(receiveOutput + receiveOffset, receiveBuffer, receiveResult - receiveOffset);
    }
    ASSERT_EQ(memcmp(receiveOutput, dataToTransfer, sendDataSize), 0);
}

TEST_F(xmodemTests, testSendReceiveSuccessFullBufferFullData) {
    sendDataSize = sizeof(dataToTransfer);
    sendBufferSize = sizeof(dataToTransfer);
    receiveBufferSize = sizeof(receiveBuffer);
    receiveTotalSize = sizeof(receiveBuffer);

  ::pthread_create(&sendThread, nullptr, sendFunc, nullptr);
  ::pthread_create(&receiveThread, nullptr, receiveFunc, nullptr);
  ::pthread_join(sendThread, nullptr);
  ::pthread_join(receiveThread, nullptr);

    ASSERT_EQ(sendResult, sendDataSize);
    ASSERT_EQ(receiveResult, sendDataSize);
    if(receiveResult > receiveOffset) {
        memcpy(receiveOutput + receiveOffset, receiveBuffer, receiveResult - receiveOffset);
    }
    ASSERT_EQ(memcmp(receiveOutput, dataToTransfer, sendDataSize), 0);
}

TEST_F(xmodemTests, testSendReceiveSuccessShortBufferShortData) {
    sendDataSize = 100;
    sendBufferSize = 50;
    receiveBufferSize = 50;
    receiveTotalSize = sizeof(receiveBuffer);

    ::pthread_create(&sendThread, nullptr, sendFunc, nullptr);
    ::pthread_create(&receiveThread, nullptr, receiveFunc, nullptr);
    ::pthread_join(sendThread, nullptr);
    ::pthread_join(receiveThread, nullptr);

    ASSERT_EQ(sendResult, sendDataSize);
    ASSERT_EQ(receiveResult, sendDataSize);
    if(receiveResult > receiveOffset) {
        memcpy(receiveOutput + receiveOffset, receiveBuffer, receiveResult - receiveOffset);
    }
    ASSERT_EQ(memcmp(receiveOutput, dataToTransfer, sendDataSize), 0);
}

TEST_F(xmodemTests, testSendReceiveSuccessShortBufferFullData) {
    sendDataSize = sizeof(dataToTransfer);
    sendBufferSize = 50;
    receiveBufferSize = 50;
    receiveTotalSize = sizeof(receiveBuffer);

    ::pthread_create(&sendThread, nullptr, sendFunc, nullptr);
    ::pthread_create(&receiveThread, nullptr, receiveFunc, nullptr);
    ::pthread_join(sendThread, nullptr);
    ::pthread_join(receiveThread, nullptr);

    ASSERT_EQ(sendResult, sendDataSize);
    ASSERT_EQ(receiveResult, sendDataSize);
    if(receiveResult > receiveOffset) {
        memcpy(receiveOutput + receiveOffset, receiveBuffer, receiveResult - receiveOffset);
    }
    ASSERT_EQ(memcmp(receiveOutput, dataToTransfer, sendDataSize), 0);
}

TEST_F(xmodemTests, testSendReceiveFailBufferFull) {
    sendDataSize = sizeof(dataToTransfer);
    sendBufferSize = sizeof(dataToTransfer);
    receiveBufferSize = 100;
    receiveTotalSize = 100;

    ::pthread_create(&sendThread, nullptr, sendFunc, nullptr);
    ::pthread_create(&receiveThread, nullptr, receiveFunc, nullptr);
    ::pthread_join(sendThread, nullptr);
    ::pthread_join(receiveThread, nullptr);

    ASSERT_EQ(sendResult, xmodemErrorCancelledByRemote);
    ASSERT_EQ(receiveResult, xmodemErrorBufferFull);
}
