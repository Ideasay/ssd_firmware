//////////////////////////////////////////////////////////////////////////////////
// ftl_config.c for Cosmos+ OpenSSD
// Copyright (c) 2017 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
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
// Engineer: Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: Flash Translation Layer Configuration Manager
// File Name: ftl_config.c
//
// Version: v1.0.0
//
// Description:
//   - initialize flash translation layer
//	 - check configuration options
//	 - initialize NAND device
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////


#include <assert.h>
#include "xil_printf.h"
#include "memory_map.h"

unsigned int storageCapacity_L;

void InitFTL()
{
	//CheckConfigRestriction();
	InitNandArray();
	InitMetaData();

	storageCapacity_L = 2*CHUNK_NUM_PER_PU*128*8192;

	INIT_PRINT("[ storage capacity %d MB ]\r\n", storageCapacity_L / ((1024*1024) ));
	INIT_PRINT("[ ftl configuration complete. ]\r\n");
}


void InitNandArray()
{
	uint32_t base_addr;
	uint32_t chNo;
	uint32_t wayNo;
	for(chNo=0; chNo<USER_CHANNELS; ++chNo)
		for(wayNo=0; wayNo<USER_WAYS; ++wayNo)
		{
			if(chNo == 0)
			{
				base_addr = NSC_0_BASEADDR;
			}
			else if(chNo == 1)
			{
				base_addr = NSC_1_BASEADDR;
			}
			reset_ffh(base_addr, wayNo+1);
			setfeature_efh(base_addr, wayNo+1, 0x15000000);
		}

	INIT_PRINT("[ NAND device reset complete. ]\r\n");
}

