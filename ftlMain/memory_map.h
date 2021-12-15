#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_


//#define NVME_DEBUG
//#define ADDR_DEBUG
//#define MAIN_DEBUG
//#define LEVEL_DEBUG 	  //ReqTransSliceToLowLevel
//#define INIT_DEBUG
//#define REQ_DEBUG
#define AD_DEBUG
//#define IO_DEBUG
//#define XDMA_DEBUG
#define META_DEBUG


#ifdef NVME_DEBUG
#define NVME_PRINT		xil_printf
#else
#define NVME_PRINT(...)
#endif

#ifdef ADDR_DEBUG
#define ADDR_PRINT		xil_printf
#else
#define ADDR_PRINT(...)
#endif

#ifdef MAIN_DEBUG
#define MAIN_PRINT		xil_printf
#else
#define MAIN_PRINT(...)
#endif

#ifdef LEVEL_DEBUG
#define LEVEL_PRINT		xil_printf
#else
#define LEVEL_PRINT(...)
#endif

#ifdef INIT_DEBUG
#define INIT_PRINT  xil_printf
#else
#define INIT_PRINT(...)
#endif

#ifdef REQ_DEBUG
#define REQ_PRINT  xil_printf
#else
#define REQ_PRINT(...)
#endif

#ifdef AD_DEBUG
#define AD_PRINT  xil_printf
#else
#define AD_PRINT(...)
#endif

#ifdef IO_DEBUG
#define IO_PRINT  xil_printf
#else
#define IO_PRINT(...)
#endif

#ifdef XDMA_DEBUG
#define XDMA_PRINT  xil_printf
#else
#define XDMA_PRINT(...)
#endif

#ifdef META_DEBUG
#define META_PRINT  xil_printf
#else
#define META_PRINT(...)
#endif

#include "metadata_management.h"
#include "xparameters.h"

#define DRAM_START_ADDR					XPAR_DDR4_0_BASEADDR + 0x00100000//XPAR_MIG_0_BASEADDR //xparameters

#define CH0_META_DATA_ADDR              DRAM_START_ADDR
#define CH1_META_DATA_ADDR              CH0_META_DATA_ADDR + 0x2000
#define TOTAL_META_DATA_ADDR            CH1_META_DATA_ADDR + 0x2000
#define GEOMETRY_DATA_ADDR              TOTAL_META_DATA_ADDR + 0x4000

/**************************************************************
 ******************* ourNVMe Segement begin *******************
 **************************************************************/
#define MEM_PAGE_WIDTH                  (0xc)  //better to get the value from hardware
#define LBA_SIZE 				        (0x1000)

// XDMA internal registers base address, XPAR_NVME_CONTROLLER_1004_0_BASEADDR
#define XDMA_REG_BASEADDR  				(XPAR_NVME_CONTROLLER_0_BASEADDR)
// PL-side user registers base address, XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR
#define USER_REG_BASEADDR  				(XPAR_BRAM_0_BASEADDR)
// PL-side SQ-entry buffer base address, XPAR_AXI_BRAM_CTRL_1_S_AXI_BASEADDR
#define PL_SQ_ENTRY_BUF_BASEADDR		(XPAR_BRAM_1_BASEADDR)

// PL-side SQ-entry buffer size(number of SQ entries)
#define PL_SQ_ENTRY_NUM 				(8)
// PL-side CQ-entry buffer base address, XPAR_AXI_BRAM_CTRL_1_S_AXI_BASEADDR
#define PL_CQ_ENTRY_BUF_BASEADDR		(XPAR_BRAM_1_BASEADDR)
// PL-side CQ-entry buffer size(number of CQ entries)
#define PL_CQ_ENTRY_NUM					(8)

// PL-side SQ-data buffer base address, XPAR_AXI_BRAM_CTRL_2_S_AXI_BASEADDR
#define PL_SQ_DATA_BUF_BASEADDR			(0xa0000000+0x1000)
// PL-side SQ-data buffer size(byte)
#define PL_SQ_DATA_BUF_SIZE				(4096)

// PL-side CQ-data buffer base address, XPAR_AXI_BRAM_CTRL_2_S_AXI_BASEADDR
#define PL_CQ_DATA_BUF_BASEADDR			(PL_SQ_DATA_BUF_BASEADDR+0x1000)
// PL-side CQ-data buffer size(byte)
#define PL_CQ_DATA_BUF_SIZE				(4096)

#define PL_IO_READ_BUF_BASEADDR			(PL_CQ_DATA_BUF_BASEADDR+0x2000)
#define PL_IO_WRITE_BUF_BASEADDR	    (PL_IO_READ_BUF_BASEADDR+0x2000)
#define PL_IO_PRP_BUF_BASEADDR	        (PL_IO_WRITE_BUF_BASEADDR+0x4000)
#define PL_IO_END                       (PL_IO_PRP_BUF_BASEADDR+0x2000)
/**************************************************************
 ******************* ourNVMe Segement end *********************
 **************************************************************/
#define RESERVED1_START_ADDR			    PL_IO_END
#define RESERVED1_END_ADDR					XPAR_MIG_0_HIGHADDR
#define DRAM_END_ADDR						XPAR_MIG_0_HIGHADDR

#endif /* MEMORY_MAP_H_ */
