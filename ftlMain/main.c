//////////////////////////////////////////////////////////////////////////////////
// main.c for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//				  Kibin Park <kbpark@enc.hanyang.ac.kr>
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
//			 Kibin Park <kbpark@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: Main
// File Name: main.c
//
// Version: v1.0.2
//
// Description:
//   - initializes caches, MMU, exception handler
//   - calls nvme_main function
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.2
//   - An address region (0x0020_0000 ~ 0x179F_FFFF) is used to uncached & nonbuffered region
//   - An address region (0x1800_0000 ~ 0x3FFF_FFFF) is used to cached & buffered region
//
// * v1.0.1
//   - Paging table setting is modified for QSPI or SD card boot mode
//     * An address region (0x0010_0000 ~ 0x001F_FFFF) is used to place code, data, heap and stack sections
//     * An address region (0x0010_0000 ~ 0x001F_FFFF) is setted a cached&bufferd region
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_exception.h"
#include "xil_io.h"
#include "sleep.h"


#include "xil_cache.h"
//#include "xil_exception.h"
//#include "xil_mmu.h"
#include "xparameters.h"
//#include "xscugic_hw.h"
//#include "xscugic.h"
//#include "xil_printf.h"

//for cosmos.
/*#include "nvme/nvme.h"
#include "nvme/host_lld.h"*/
//for simulation
//#include "generate.h"

/************************
 *  nvme header begin
 * **********************/
#include "../ourNVMe/nvme_admin_controller.h"
#include "../ourNVMe/debug.h"
#include "memory_map.h"
#include "../ourNVMe/nvme_structs.h"

/************************
 *  nvme header end
 * **********************/

int main()
{	
	//init platform
	u64 a;
    init_platform();
    a=  0-(1<<12) ;
    xil_printf("%x\n\r", a);
    xil_printf("======NVMe Test Start========\n\r");
    Xil_DCacheDisable();
	Xil_ICacheDisable();

	// Paging table set
	#define MB (1024*1024)
	enable_xdma_channels();
    init_nvme_controller(1);
    while(xdma_msi_enable_get() == 0){
    	usleep(100);
    }

	//init FTL & clear whole disk
	xil_printf("!!! Wait until FTL reset complete !!! \r\n");

	InitFTL();

	xil_printf("\r\nFTL reset complete!!! \r\n");

    eraseblock_60h_d0h(NSC_0_BASEADDR,1,0x7f800);
    eraseblock_60h_d0h(NSC_1_BASEADDR,1,0x7f800);
    xil_printf("erase two channels complete! \r\n");

	//for state machine init
	unsigned int exeLlr;
	volatile NVME_CONTEXT g_nvmeTask;
	NVME_STATE_E state = NVME_STATE_DISABLED;
	g_nvmeTask.status = state;

	//for sq & cq
	nvme_cmd_t *nvmeCmd;
	nvme_sq_entry_t admin_sq_entry;
	nvme_sq_entry_t io_sq_entry;
	int flag;
	while(1)
	{
		exeLlr = 1;
		switch(state)
		{
			//COSMOS case NVME_TASK_WAIT_CC_EN	
			case NVME_STATE_DISABLED:{
				if(get_reg_cc_en() == 1){
					state = NVME_STATE_ENABLED;
					g_nvmeTask.status = state;
					xil_printf("NVMe Reg CC_EN is set\n\r");
				}
				//not break, just fall through
			}
			//new added state
			case(NVME_STATE_ENABLED): {
				state = NVME_STATE_RUNNING;
				g_nvmeTask.status = state;
				//init the SQ and CQ
				init_nvme_controller(0);
				set_csts_rdy(1);
     			//xdma_msix_vector_print();
				xil_printf("PS Set CSTS RDY \n\r");
				//not break, just fall through
			}
			//COSMOS case NVME_TASK_RUNNING
			case(NVME_STATE_RUNNING): {
				if(get_reg_cc_en() == 0){
					state = NVME_STATE_DISABLED;
					g_nvmeTask.status = state;
					init_nvme_controller(1);
					set_csts_rdy(0);
					xil_printf("Controller Reset \n\r");
				} else if(get_reg_cc_shn()){
					state = NVME_STATE_SHUTDOWN;
					g_nvmeTask.status = state;
					//init_nvme_controller(1);
					set_csts_shst(2);
					xil_printf("Controller Shutdown \n\r");
				} else {
					//to do
					//for fetching instctions and decoding
					u32 read_admin_sq = nvme_read_sq_entry(&admin_sq_entry);
					u32 read_io_sq    = nvme_read_io_sq_entry(&io_sq_entry);
					nvme_main_process(read_admin_sq,read_io_sq,admin_sq_entry,io_sq_entry,nvmeCmd);
				}
				break;
			}
			//COSMOS case NVME_TASK_SHUTDOWN
			case(NVME_STATE_SHUTDOWN):{
			//to do. this might be related to CQ!! in COSMOS
				if(get_reg_cc_en() == 0){
					state = NVME_STATE_DISABLED;
					g_nvmeTask.status = state;
					set_csts_shst(0);
					set_csts_rdy(0);
					xil_printf("Controller Restart \n\r");
				}
				break;
			}
			default: {
				state = NVME_STATE_DISABLED;
				g_nvmeTask.status = state;
			}
			//xil_printf("======NVMe Test End========\n\r");
    	cleanup_platform();
    	return 0;
	}
	