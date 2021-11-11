#ifndef __HOST_LLD_H_
#define __HOST_LLD_H_

//this header is for DMA Transfer
#include "xparameters.h"
#include "../nvme/nvme.h"

//HOST_DMA_STATUS g_hostDmaStatus;
//HOST_DMA_ASSIST_STATUS g_hostDmaAssistStatus;

// not used in our firmware 
/*void set_auto_tx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

void set_auto_rx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

unsigned int check_auto_tx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);

unsigned int check_auto_rx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);*/

void SIM_H2C_DMA(unsigned int lba , unsigned int databuffer_index);
void SIM_C2H_DMA(unsigned int lba , unsigned int databuffer_index);
void H2C_DMA_PRP2DATA( u64 prpEntry, unsigned int databuffer_index, unsigned int dataLengthForSlice);
void C2H_DMA_PRP2DATA( u64 prpEntry, unsigned int databuffer_index, unsigned int dataLengthForSlice);
//extern HOST_DMA_STATUS g_hostDmaStatus;
//extern HOST_DMA_ASSIST_STATUS g_hostDmaAssistStatus;


#endif	//__HOST_LLD_H_
