//////////////////////////////////////////////////////////////////////////////////
// host_lld.c for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// This file is part of Cosmos+ OpenSSD.
//
// Cosmos+ OpenSSD is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// Cosmos+ OpenSSD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cosmos+ OpenSSD; see the file COPYING.
// If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Company: ENC Lab. <http://enc.hanyang.ac.kr>
// Engineer: Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//			 Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: NVMe Low Level Driver
// File Name: host_lld.c
//
// Version: v1.1.0
//
// Description:
//   - defines functions to control the NVMe controller
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.1.0
//	 - DMA partial done check functions are added
//	 - DMA assist status is added to support DMA partial done check functions
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "debug.h"
#include "io_access.h"

#include "nvme.h"
#include "host_lld.h"

#include "../ftl_config.h"
#include "memory_map.h"
#include "../ourNVMe/nvme_transmitter.h"
extern NVME_CONTEXT g_nvmeTask;
//extern HOST_DMA_STATUS g_hostDmaStatus;
//HOST_DMA_ASSIST_STATUS g_hostDmaAssistStatus;

/* not used in our firmware
void set_auto_tx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion)
{
	HOST_DMA_CMD_FIFO_REG hostDmaReg;
	unsigned char tempTail;

	ASSERT(cmd4KBOffset < 256);
	
	g_hostDmaStatus.fifoHead.dword = IO_READ32(HOST_DMA_FIFO_CNT_REG_ADDR);
	while((g_hostDmaStatus.fifoTail.autoDmaTx + 1) % 256 == g_hostDmaStatus.fifoHead.autoDmaTx)
		g_hostDmaStatus.fifoHead.dword = IO_READ32(HOST_DMA_FIFO_CNT_REG_ADDR);

	hostDmaReg.devAddr = devAddr;

	hostDmaReg.dword[3] = 0;
	hostDmaReg.dmaType = HOST_DMA_AUTO_TYPE;
	hostDmaReg.dmaDirection = HOST_DMA_TX_DIRECTION;
	hostDmaReg.cmd4KBOffset = cmd4KBOffset;
	hostDmaReg.cmdSlotTag = cmdSlotTag;
	hostDmaReg.autoCompletion = autoCompletion;

	IO_WRITE32(HOST_DMA_CMD_FIFO_REG_ADDR, hostDmaReg.dword[0]);
	//IO_WRITE32((HOST_DMA_CMD_FIFO_REG_ADDR + 4), hostDmaReg.dword[1]);
	//IO_WRITE32((HOST_DMA_CMD_FIFO_REG_ADDR + 8), hostDmaReg.dword[2]);
	IO_WRITE32((HOST_DMA_CMD_FIFO_REG_ADDR + 12), hostDmaReg.dword[3]);

	tempTail = g_hostDmaStatus.fifoTail.autoDmaTx++;
	if(tempTail > g_hostDmaStatus.fifoTail.autoDmaTx)
		g_hostDmaAssistStatus.autoDmaTxOverFlowCnt++;

	g_hostDmaStatus.autoDmaTxCnt++;
}

void set_auto_rx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion)
{
	HOST_DMA_CMD_FIFO_REG hostDmaReg;
	unsigned char tempTail;

	ASSERT(cmd4KBOffset < 256);
	
	g_hostDmaStatus.fifoHead.dword = IO_READ32(HOST_DMA_FIFO_CNT_REG_ADDR);
	while((g_hostDmaStatus.fifoTail.autoDmaRx + 1) % 256 == g_hostDmaStatus.fifoHead.autoDmaRx)
		g_hostDmaStatus.fifoHead.dword = IO_READ32(HOST_DMA_FIFO_CNT_REG_ADDR);

	hostDmaReg.devAddr = devAddr;

	hostDmaReg.dword[3] = 0;
	hostDmaReg.dmaType = HOST_DMA_AUTO_TYPE;
	hostDmaReg.dmaDirection = HOST_DMA_RX_DIRECTION;
	hostDmaReg.cmd4KBOffset = cmd4KBOffset;
	hostDmaReg.cmdSlotTag = cmdSlotTag;
	hostDmaReg.autoCompletion = autoCompletion;

	IO_WRITE32(HOST_DMA_CMD_FIFO_REG_ADDR, hostDmaReg.dword[0]);
	//IO_WRITE32((HOST_DMA_CMD_FIFO_REG_ADDR + 4), hostDmaReg.dword[1]);
	//IO_WRITE32((HOST_DMA_CMD_FIFO_REG_ADDR + 8), hostDmaReg.dword[2]);
	IO_WRITE32((HOST_DMA_CMD_FIFO_REG_ADDR + 12), hostDmaReg.dword[3]);

	tempTail = g_hostDmaStatus.fifoTail.autoDmaRx++;
	if(tempTail > g_hostDmaStatus.fifoTail.autoDmaRx)
		g_hostDmaAssistStatus.autoDmaRxOverFlowCnt++;

	g_hostDmaStatus.autoDmaRxCnt++;
}

unsigned int check_auto_tx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex)
{
	//xil_printf("check_auto_tx_dma_partial_done \r\n");

	g_hostDmaStatus.fifoHead.dword = IO_READ32(HOST_DMA_FIFO_CNT_REG_ADDR);

	if(g_hostDmaStatus.fifoHead.autoDmaTx == g_hostDmaStatus.fifoTail.autoDmaTx)
		return 1;

	if(g_hostDmaStatus.fifoHead.autoDmaTx < tailIndex)
	{
		if(g_hostDmaStatus.fifoTail.autoDmaTx < tailIndex)
		{
			if(g_hostDmaStatus.fifoTail.autoDmaTx > g_hostDmaStatus.fifoHead.autoDmaTx)
				return 1;
			else
				if(g_hostDmaAssistStatus.autoDmaTxOverFlowCnt != (tailAssistIndex + 1))
					return 1;
		}
		else
			if(g_hostDmaAssistStatus.autoDmaTxOverFlowCnt != tailAssistIndex)
				return 1;

	}
	else if(g_hostDmaStatus.fifoHead.autoDmaTx == tailIndex)
		return 1;
	else
	{
		if(g_hostDmaStatus.fifoTail.autoDmaTx < tailIndex)
			return 1;
		else
		{
			if(g_hostDmaStatus.fifoTail.autoDmaTx > g_hostDmaStatus.fifoHead.autoDmaTx)
				return 1;
			else
				if(g_hostDmaAssistStatus.autoDmaTxOverFlowCnt != tailAssistIndex)
					return 1;
		}
	}

	return 0;
}

unsigned int check_auto_rx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex)
{
	//xil_printf("check_auto_rx_dma_partial_done \r\n");

	g_hostDmaStatus.fifoHead.dword = IO_READ32(HOST_DMA_FIFO_CNT_REG_ADDR);

	if(g_hostDmaStatus.fifoHead.autoDmaRx == g_hostDmaStatus.fifoTail.autoDmaRx)
		return 1;

	if(g_hostDmaStatus.fifoHead.autoDmaRx < tailIndex)
	{
		if(g_hostDmaStatus.fifoTail.autoDmaRx < tailIndex)
		{
			if(g_hostDmaStatus.fifoTail.autoDmaRx > g_hostDmaStatus.fifoHead.autoDmaRx)
				return 1;
			else
				if(g_hostDmaAssistStatus.autoDmaRxOverFlowCnt != (tailAssistIndex + 1))
					return 1;
		}
		else
			if(g_hostDmaAssistStatus.autoDmaRxOverFlowCnt != tailAssistIndex)
				return 1;

	}
	else if(g_hostDmaStatus.fifoHead.autoDmaRx == tailIndex)
		return 1;
	else
	{
		if(g_hostDmaStatus.fifoTail.autoDmaRx < tailIndex)
			return 1;
		else
		{
			if(g_hostDmaStatus.fifoTail.autoDmaRx > g_hostDmaStatus.fifoHead.autoDmaRx)
				return 1;
			else
				if(g_hostDmaAssistStatus.autoDmaRxOverFlowCnt != tailAssistIndex)
					return 1;
		}
	}

	return 0;
}
*/

void SIM_H2C_DMA( unsigned int lba , unsigned int databuffer_index)
{
	unsigned int datasim_no;
	datasim_no = (lba - 0x00000000)  + 1; //  / BYTES_PER_DATA_REGION_OF_SLICE
    unsigned int i;
    char * char_addr_ptr;
    char * databuffer_ptr;
    char_addr_ptr = (char*)(NVME_DATA_SIM_ADDR + (datasim_no-1) * BYTES_PER_DATA_REGION_OF_SLICE);
    databuffer_ptr = (char*)(DATA_BUFFER_BASE_ADDR + databuffer_index * BYTES_PER_DATA_REGION_OF_SLICE);
    for(i=0 ; i<BYTES_PER_DATA_REGION_OF_SLICE ; i++)
    {
    	*(databuffer_ptr+i) = *(char_addr_ptr+i);
    }
    xil_printf("!!! copy data to DDR data buffer No.%d complete !!! \r\n", databuffer_index);
    xil_printf("!!! first 4 bytes of the data buffer are 0x%08x \r\n", *((unsigned int*)databuffer_ptr));
    //unsigned char tempTail;
    g_hostDmaStatus.fifoTail.autoDmaRx++;
    //g_hostDmaStatus.fifoTail.autoDmaRx++;
    g_hostDmaStatus.autoDmaRxCnt++;
}

void SIM_C2H_DMA(unsigned int lba , unsigned int databuffer_index)
{
	unsigned int datasim_no;
	datasim_no = ((lba - 0x00000000)  + 1) * 2;
    unsigned int i;
    char * char_addr_ptr;
    char * databuffer_ptr;
    char_addr_ptr = (char*)(NVME_DATA_SIM_ADDR + (datasim_no-1) * BYTES_PER_DATA_REGION_OF_SLICE);
    databuffer_ptr = (char*)(DATA_BUFFER_BASE_ADDR + databuffer_index * BYTES_PER_DATA_REGION_OF_SLICE);
    for(i=0 ; i<BYTES_PER_DATA_REGION_OF_SLICE ; i++)
    {
    	*(char_addr_ptr+i) = *(databuffer_ptr+i);
    }
    xil_printf("!!! read data from DDR data buffer No.%d complete !!! \r\n", databuffer_index);
    xil_printf("!!! first 4 bytes of read data are 0x%08x \r\n", *((unsigned int*)char_addr_ptr));
    //unsigned char tempTail;
    g_hostDmaStatus.fifoTail.autoDmaRx++; //tempTail =
    g_hostDmaStatus.autoDmaTxCnt++;
}
void H2C_DMA_PRP2DATA( u64 prpEntry, unsigned int databuffer_index, unsigned int dataLengthForSlice)
{
	u64 addr_total = prpEntry;
	char * databuffer_ptr;
	databuffer_ptr = (char*)(DATA_BUFFER_BASE_ADDR + databuffer_index * BYTES_PER_DATA_REGION_OF_SLICE);
	write_ioD_h2c_dsc(addr_total,databuffer_ptr,dataLengthForSlice);// unit is byte!
	while((get_io_dma_status() & 0x1) == 0);
    xil_printf("!!! copy data to DDR data buffer No.%d complete !!! \r\n", databuffer_index);
    xil_printf("!!! first 4 bytes of the data buffer are 0x%08x \r\n", *((unsigned int*)databuffer_ptr));
    //unsigned char tempTail;
    g_hostDmaStatus.fifoTail.autoDmaRx++;
    //g_hostDmaStatus.fifoTail.autoDmaRx++;
    g_hostDmaStatus.autoDmaRxCnt++;
}

void C2H_DMA_PRP2DATA( u64 prpEntry, unsigned int databuffer_index, unsigned int dataLengthForSlice)
{
	u64 addr_total = prpEntry;
	char * databuffer_ptr;
	databuffer_ptr = (char*)(DATA_BUFFER_BASE_ADDR + databuffer_index * BYTES_PER_DATA_REGION_OF_SLICE);
	write_ioD_c2h_dsc(addr_total,databuffer_ptr,dataLengthForSlice);// unit is byte!
	while((get_io_dma_status() & 0x4) == 0);
    xil_printf("!!! read data from DDR data buffer No.%d complete !!! \r\n", databuffer_index);
    xil_printf("!!! first 4 bytes of read data are 0x%08x \r\n", *((unsigned int*)char_addr_ptr));
    //unsigned char tempTail;
    g_hostDmaStatus.fifoTail.autoDmaRx++; 
    g_hostDmaStatus.autoDmaTxCnt++;
}

