#ifndef __HOST_LLD_H_
#define __HOST_LLD_H_

//this header is for DMA Transfer
#include "xparameters.h"
#include "../nvme/nvme.h"

typedef struct _HOST_DMA_FIFO_CNT_REG
{
	union {
		unsigned int dword;
		struct
		{
			unsigned char directDmaRx;
			unsigned char directDmaTx;
			unsigned char autoDmaRx;
			unsigned char autoDmaTx;
		};
	};
} HOST_DMA_FIFO_CNT_REG;
/*
typedef struct _HOST_DMA_CMD_FIFO_REG
{
	union {
		unsigned int dword[4];
		struct
		{
			unsigned int devAddr;
			unsigned int pcieAddrH;
			unsigned int pcieAddrL;
			struct
			{
				unsigned int dmaLen				:13;
				unsigned int autoCompletion		:1;
				unsigned int cmd4KBOffset		:9;
				unsigned int cmdSlotTag			:7;
				unsigned int dmaDirection		:1;
				unsigned int dmaType			:1;
			};
		};
	};
} HOST_DMA_CMD_FIFO_REG;
*/
typedef struct _HOST_DMA_STATUS
{
	HOST_DMA_FIFO_CNT_REG fifoHead;
	HOST_DMA_FIFO_CNT_REG fifoTail;
	unsigned int directDmaTxCnt;
	unsigned int directDmaRxCnt;
	unsigned int autoDmaTxCnt;
	unsigned int autoDmaRxCnt;
} HOST_DMA_STATUS;

HOST_DMA_STATUS g_hostDmaStatus;
//HOST_DMA_ASSIST_STATUS g_hostDmaAssistStatus;

// not used in our firmware 
/*void set_auto_tx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

void set_auto_rx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

unsigned int check_auto_tx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);

unsigned int check_auto_rx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);*/

void SIM_H2C_DMA(unsigned int lba , unsigned int databuffer_index);
void SIM_C2H_DMA(unsigned int lba , unsigned int databuffer_index);
void H2C_DMA_PRP2DATA( u64 prpEntry, unsigned int databuffer_index, unsigned int dataLengthForSlice,u64 offset);
void C2H_DMA_PRP2DATA( u64 prpEntry, unsigned int databuffer_index, unsigned int dataLengthForSlice,u64 offset);
//extern HOST_DMA_STATUS g_hostDmaStatus;
//extern HOST_DMA_ASSIST_STATUS g_hostDmaAssistStatus;


#endif	//__HOST_LLD_H_
