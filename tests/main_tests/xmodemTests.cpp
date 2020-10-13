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
#define XMODEM_BUFFER_SIZE 128
static uint8_t tx_wireBuffer[WIRE_BUFFER_SIZE];
static size_t tx_wireBufferStart = 0;
static size_t tx_wireBufferEnd = 0;
static uint8_t rx_wireBuffer[WIRE_BUFFER_SIZE];
static size_t rx_wireBufferStart = 0;
static size_t rx_wireBufferEnd = 0;
static pthread_t sendThread, receiveThread;
static double lossRate = 0;
static double corruptionRate = 0;
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
static bool isUnlucky(double rate) {
    uint32_t r = (uint32_t)random();
    return r < (uint32_t)((double)UINT32_MAX) * rate;
}
void xmodem_OutByte(unsigned char c) {
    bool isSend = pthread_self() == sendThread;
    uint8_t *wireBuffer = isSend ? tx_wireBuffer : rx_wireBuffer;
    size_t *wireBufferEnd = isSend ? &tx_wireBufferEnd : &rx_wireBufferEnd;
    bool corrupt = false, lost = false;
    if(lossRate > 0) {
        lost = isUnlucky(lossRate);
    }
    if(corruptionRate > 0) {
        corrupt = isUnlucky(corruptionRate);
    }
    if(lost) {
        PRINT_WIRE("%s op T byte 0x%02X LOST\n", isSend ? "T" : "R", c);
    } else {
        if(corrupt) {
            c = (unsigned char)random();
        }
        PRINT_WIRE("%s op T byte 0x%02X%s\n", isSend ? "T" : "R", c, corrupt ? "" : " CORRUPT");
        wireBuffer[*wireBufferEnd] = c;
        *wireBufferEnd += 1;
        if(*wireBufferEnd == WIRE_BUFFER_SIZE) {
            *wireBufferEnd = 0;
        }
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

unsigned char * getRxBuffer(int *size) {
    if(receiveTotalSize < receiveOffset + receiveBufferSize) {
        assert(false);
    }
    if(receiveOffset < 0) {
        receiveOffset = 0;
    } else {
        memcpy(receiveOutput + receiveOffset, receiveBuffer, MIN(receiveBufferSize, sizeof(receiveOutput) - receiveOffset));
        receiveOffset += receiveBufferSize;
        if(receiveTotalSize < receiveOffset + receiveBufferSize) {
            return NULL;
        }
    }
    *size = receiveBufferSize;
    return receiveBuffer;
}

void * receiveFunc(void* ptr) {
    receiveOffset = -1;
    receiveResult = xmodemReceive(getRxBuffer);
    return NULL;
}

class xmodemTests : public ::testing::Test {
    void SetUp() override {
        tx_wireBufferStart = 0;
        tx_wireBufferEnd = 0;
        rx_wireBufferStart = 0;
        rx_wireBufferEnd = 0;
        lossRate = 0;
        for(int i = 0; i < sizeof(dataToTransfer); i++) {
            dataToTransfer[i] = (uint8_t)i;
        }
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

TEST_F(xmodemTests, testSendReceiveSuccessXmodemFullBufferEnd0) {
    sendDataSize = XMODEM_BUFFER_SIZE;
    dataToTransfer[XMODEM_BUFFER_SIZE-3] = 0;
    dataToTransfer[XMODEM_BUFFER_SIZE-2] = 0;
    dataToTransfer[XMODEM_BUFFER_SIZE-1] = 0;
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

TEST_F(xmodemTests, testSendReceiveSuccessFullBufferFullDataLoss) {
    lossRate = 1.0 / (2.0 * 128.0);
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

TEST_F(xmodemTests, testSendReceiveSuccessFullBufferFullDataCorruption) {
    corruptionRate = 1.0 / (2.0 * 128.0);
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

TEST_F(xmodemTests, testSendReceiveSuccessFullBufferFullDataLossAndCorruption) {
    lossRate = 1.0 / (4.0 * 128.0);
    corruptionRate = 1.0 / (4.0 * 128.0);
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
