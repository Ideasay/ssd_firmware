/**
* NVMeCHA: NVMe Controller featuring Hardware Acceleration
* Copyright (C) 2021 State Key Laboratory of ASIC and System, Fudan University
* Contributed by Yunhui Qiu
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Hardware Parameters, including:
 *   Register addresses, BRAM buffer addresses...
 */

//1022 delete sq reset , cq reset and related functions(reg_rw and nvme_admin_cmd)
//1022 add IO logic regs addr
//1022 add code in nvme_main_process() in nvme_admin_controller.c
//1022 nvme transmitter u32 g_io_sq_buf_rptr = 0; // SQ buffer read pointer
//1022 nvme transmitter u32 g_io_cq_buf_wptr = 0; // CQ buffer write pointer
//1022 nvme transmitter u32 nvme_read_io_sq_entry(nvme_sq_entry_t* sq_entry);
//1022 reg rw u32 get_io0_sq_buf_wptr();
//1023 nvme io cmds.h cpy from zxj
//1023 debug.h cpy from zxj
//1023 nvme io cmds.c cpy from zxj
//1023 add #include "nvme_io_cmds.h" to nvme admin controller.h
/* add to nvme structs.h
 //Opcode for IO Commands
typedef enum _NVME_IO_OPCODE_E
{
	NVME_IO_OPCODE_FLUSH							= 0x00,
	NVME_IO_OPCODE_WRITE							= 0x01,
	NVME_IO_OPCODE_READ								= 0x02,
	NVME_IO_OPCODE_WRITE_UNCORRECTABLE				= 0x04,
	NVME_IO_OPCODE_COMPARE							= 0x05,
	NVME_IO_OPCODE_MANAGEMENT						= 0x09,
}NVME_IO_OPCODE_E;
*/
//1024 nvme io cmd .c .hfucs
//1024 reg rw.c .h set io dsc info
//1024 nvme transmitter set io dsc funcs

#ifndef __HW_PARAMS__
#define __HW_PARAMS__

#include "xparameters.h"
#include "../ftlMain/memory_map.h"

// XDMA internal registers offset address
#define XDMA_H2C_CHAN_0_CTRL_ADDR    (0x0004)
#define XDMA_C2H_CHAN_0_CTRL_ADDR    (0x1004)
#define XDMA_USR_IRQ_EN_ADDR         (0x2004)
#define XDMA_CHANNEL_IRQ_EN_ADDR     (0x2010)
#define XDMA_USR_IRQ_VECTOR_0_ADDR	 (0x2080)
#define XDMA_MSI_ENABLE_ADDR         (0x3014)
#define XDMA_MSIX_VECTOR_TABLE_ADDR  (0x8000)
#define XDMA_MSIX_PBA_ADDR           (0x8FE0)

#define XDMA_CHAN_ENABLE_VALUE       (0x0fffe7f)

// PL-side user registers offset address
#define ADDR_ASQ_DSC_CTL      		 (0x0000)
#define ADDR_ASQ_DSC_LEN      		 (0x0004)
#define ADDR_ASQ_DSC_ADDRL    		 (0x0008)
#define ADDR_ASQ_DSC_ADDRH    		 (0x000C)
#define ADDR_ASQ_DSC_DST_ADDRL       (0x0010)
#define ADDR_ASQ_DSC_DST_ADDRH       (0x0014)

#define ADDR_ACQ_DSC_CTL      		 (0x0018)
#define ADDR_ACQ_DSC_LEN      		 (0x001C)
#define ADDR_ACQ_DSC_ADDRL    		 (0x0020)
#define ADDR_ACQ_DSC_ADDRH    		 (0x0024)
#define ADDR_ACQ_DSC_SRC_ADDRL       (0x0028)
#define ADDR_ACQ_DSC_SRC_ADDRH       (0x002C)

#define ADDR_REG_CC_EN        		 (0x0030)
#define ADDR_CSTS_RDY         		 (0x0034)
#define ADDR_REG_CC_SHN        		 (0x0038)
#define ADDR_CSTS_SHST         		 (0x003C)

#define ADDR_ASQ_BUF_WPTR	  		 (0x0040)
#define ADDR_ACQ_BUF_RPTR     		 (0x0044)
#define ADDR_H2C_DMA_STATUS   		 (0x0048)
#define ADDR_C2H_DMA_STATUS   		 (0x004C)
//#define ADDR_SQ_RESET         		 (0x0070)0
//#define ADDR_CQ_RESET         		 (0x0074)

#define ADDR_IOH2CD_DSC_CTL          (0x0050)
#define ADDR_IOH2CD_DSC_LEN          (0x0054)
#define ADDR_IOH2CD_DSC_ADDRL        (0x0058)
#define ADDR_IOH2CD_DSC_ADDRH        (0x005C)
#define ADDR_IOH2CD_DSC_DST_ADDRL    (0x0060)
#define ADDR_IOH2CD_DSC_DST_ADDRH    (0x0064)

#define ADDR_IOH2CP_DSC_CTL          (0x0068)
#define ADDR_IOH2CP_DSC_LEN          (0x006C)
#define ADDR_IOH2CP_DSC_ADDRL        (0x0070)
#define ADDR_IOH2CP_DSC_ADDRH        (0x0074)
#define ADDR_IOH2CP_DSC_DST_ADDRL    (0x0078)
#define ADDR_IOH2CP_DSC_DST_ADDRH    (0x007C)


#define ADDR_IOC2HD_DSC_CTL          (0x0080)
#define ADDR_IOC2HD_DSC_LEN          (0x0084)
#define ADDR_IOC2HD_DSC_ADDRL        (0x0088)
#define ADDR_IOC2HD_DSC_ADDRH        (0x008C)
#define ADDR_IOC2HD_DSC_SRC_ADDRL    (0x0090)
#define ADDR_IOC2HD_DSC_SRC_ADDRH    (0x0094)

#define ADDR_IOSQ_BUF_WPTR           (0x0098)
#define ADDR_IOCQ_BUF_RPTR           (0x009C)
#define ADDR_IODMA_DONE              (0x00A0)//{29'h0,io_c2h_data_dma_trans_done,io_h2c_prp_dma_trans_done,io_h2c_data_dma_trans_done};

#define ADDR_IOCQ_IRQ_ENABLE  		 (0x00A4)
#define ADDR_IO_QUEUE         		 (0x00B0)


#endif
