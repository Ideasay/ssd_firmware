#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

//#define NVME_DEBUG
//#define ADDR_DEBUG
//#define MAIN_DEBUG
#define SLICE_DEBUG 	  //ReqTransNvmeToSlice
//#define LEVEL_DEBUG 	  //ReqTransSliceToLowLevel
//#define DPEN_DEBUG      //CheckBufDep
//#define BADBLOCK_DEBUG
//#define ERASE_DEBUG
#define BUF_DEBUG
//#define INIT_DEBUG
//#define GC_DEBUG
//#define REQ_DEBUG
//#define SCHD_DEBUG
//#define AD_DEBUG
//#define IO_DEBUG
//#define XDMA_DEBUG

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

#ifdef SLICE_DEBUG
#define SLICE_PRINT		xil_printf
#else
#define SLICE_PRINT(...)
#endif

#ifdef LEVEL_DEBUG
#define LEVEL_PRINT		xil_printf
#else
#define LEVEL_PRINT(...)
#endif

#ifdef DPEN_DEBUG
#define DPEN_PRINT		xil_printf
#else
#define DPEN_PRINT(...)
#endif

#ifdef BADBLOCK_DEBUG
#define BADBLOCK_PRINT  xil_printf
#else
#define BADBLOCK_PRINT(...)
#endif

#ifdef ERASE_DEBUG
#define ERASE_PRINT  xil_printf
#else
#define ERASE_PRINT(...)
#endif

#ifdef BUF_DEBUG
#define BUF_PRINT  xil_printf
#else
#define BUF_PRINT(...)
#endif

#ifdef INIT_DEBUG
#define INIT_PRINT  xil_printf
#else
#define INIT_PRINT(...)
#endif

#ifdef GC_DEBUG
#define GC_PRINT  xil_printf
#else
#define GC_PRINT(...)
#endif

#ifdef REQ_DEBUG
#define REQ_PRINT  xil_printf
#else
#define REQ_PRINT(...)
#endif

#ifdef SCHD_DEBUG
#define SCHD_PRINT  xil_printf
#else
#define SCHD_PRINT(...)
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


#include "../ftlMain/data_buffer.h"
#include "../ftlMain/garbage_collection.h"
#include "../ftlMain/request_allocation.h"
#include "../ftlMain/request_schedule.h"
#include "../ftlMain/request_transform.h"
#include "../ftlMain/address_translation.h"
#include "xparameters.h"

#define DRAM_START_ADDR					XPAR_DDR4_0_BASEADDR + 0x00100000//XPAR_MIG_0_BASEADDR //xparameters



#define MEMORY_SEGMENTS_START_ADDR		DRAM_START_ADDR
#define MEMORY_SEGMENTS_END_ADDR		DRAM_START_ADDR + 0x000FFFFF

#define NVME_MANAGEMENT_START_ADDR		MEMORY_SEGMENTS_END_ADDR + 0x00000001
#define NVME_MANAGEMENT_END_ADDR		NVME_MANAGEMENT_START_ADDR+ 0x000FFFFF

#define RESERVED0_START_ADDR			NVME_MANAGEMENT_END_ADDR + 0x00000001
#define RESERVED0_END_ADDR				RESERVED0_START_ADDR + 0x0FCFFFFF
 
#define FTL_MANAGEMENT_START_ADDR		RESERVED0_END_ADDR + 0x00000001


// Uncached & Unbuffered
//for data buffer
//here data buffer addr should be consistent with PRP_BUF_BASEADDR. to do
#define DATA_BUFFER_BASE_ADDR 					FTL_MANAGEMENT_START_ADDR
#define TEMPORARY_DATA_BUFFER_BASE_ADDR			(DATA_BUFFER_BASE_ADDR + AVAILABLE_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_DATA_REGION_OF_SLICE)
#define SPARE_DATA_BUFFER_BASE_ADDR				(TEMPORARY_DATA_BUFFER_BASE_ADDR + AVAILABLE_TEMPORARY_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_DATA_REGION_OF_SLICE)
#define TEMPORARY_SPARE_DATA_BUFFER_BASE_ADDR	(SPARE_DATA_BUFFER_BASE_ADDR + AVAILABLE_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_SPARE_REGION_OF_SLICE)
#define RESERVED_DATA_BUFFER_BASE_ADDR 			(TEMPORARY_SPARE_DATA_BUFFER_BASE_ADDR + AVAILABLE_TEMPORARY_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_SPARE_REGION_OF_SLICE)
//for nand request completion
#define COMPLETE_FLAG_TABLE_ADDR			FTL_MANAGEMENT_START_ADDR + 0x07000000
#define STATUS_REPORT_TABLE_ADDR			(COMPLETE_FLAG_TABLE_ADDR + sizeof(COMPLETE_FLAG_TABLE))
#define ERROR_INFO_TABLE_ADDR				(STATUS_REPORT_TABLE_ADDR + sizeof(STATUS_REPORT_TABLE))
#define TEMPORARY_PAY_LOAD_ADDR				(ERROR_INFO_TABLE_ADDR+ sizeof(ERROR_INFO_TABLE))
// cached & buffered
// for buffers
#define DATA_BUFFER_MAP_ADDR		 		FTL_MANAGEMENT_START_ADDR + 0x08000000
#define DATA_BUFFFER_HASH_TABLE_ADDR		(DATA_BUFFER_MAP_ADDR + sizeof(DATA_BUF_MAP))
#define TEMPORARY_DATA_BUFFER_MAP_ADDR 		(DATA_BUFFFER_HASH_TABLE_ADDR + sizeof(DATA_BUF_HASH_TABLE))
// for map tables
#define LOGICAL_SLICE_MAP_ADDR				(TEMPORARY_DATA_BUFFER_MAP_ADDR + sizeof(TEMPORARY_DATA_BUF_MAP))
#define VIRTUAL_SLICE_MAP_ADDR				(LOGICAL_SLICE_MAP_ADDR + sizeof(LOGICAL_SLICE_MAP))
#define VIRTUAL_BLOCK_MAP_ADDR				(VIRTUAL_SLICE_MAP_ADDR + sizeof(VIRTUAL_SLICE_MAP))
#define PHY_BLOCK_MAP_ADDR					(VIRTUAL_BLOCK_MAP_ADDR + sizeof(VIRTUAL_BLOCK_MAP))
#define BAD_BLOCK_TABLE_INFO_MAP_ADDR		(PHY_BLOCK_MAP_ADDR + sizeof(PHY_BLOCK_MAP))
#define VIRTUAL_DIE_MAP_ADDR				(BAD_BLOCK_TABLE_INFO_MAP_ADDR + sizeof(BAD_BLOCK_TABLE_INFO_MAP))
// for GC victim selection
#define GC_VICTIM_MAP_ADDR					(VIRTUAL_DIE_MAP_ADDR + sizeof(VIRTUAL_DIE_MAP))
// for request pool
#define REQ_POOL_ADDR						(GC_VICTIM_MAP_ADDR + sizeof(GC_VICTIM_MAP))
// for dependency table
#define ROW_ADDR_DEPENDENCY_TABLE_ADDR		(REQ_POOL_ADDR + sizeof(REQ_POOL))
// for request scheduler
#define DIE_STATE_TABLE_ADDR				(ROW_ADDR_DEPENDENCY_TABLE_ADDR + sizeof(ROW_ADDR_DEPENDENCY_TABLE))
#define RETRY_LIMIT_TABLE_ADDR				(DIE_STATE_TABLE_ADDR + sizeof(DIE_STATE_TABLE))
#define WAY_PRIORITY_TABLE_ADDR 			(RETRY_LIMIT_TABLE_ADDR + sizeof(RETRY_LIMIT_TABLE))

#define FTL_MANAGEMENT_END_ADDR				((WAY_PRIORITY_TABLE_ADDR + sizeof(WAY_PRIORITY_TABLE))- 1)

//****** nvme segment for simulation
#define NVME_REQ_SIM_ADDR                   (FTL_MANAGEMENT_END_ADDR + 1)

#define NVME_DATA_SIM_ADDR				    (NVME_REQ_SIM_ADDR + 10*sizeof(NVME_COMMAND))
//*******/
#define RESERVED1_START_ADDR                (NVME_DATA_SIM_ADDR + 10*BYTES_PER_DATA_REGION_OF_SLICE)
//#define RESERVED1_START_ADDR                (FTL_MANAGEMENT_END_ADDR + 10*BYTES_PER_DATA_REGION_OF_SLICE)
/***********************
 * ourNVMe Segement begin
 * ******************/
#define MEM_PAGE_WIDTH                  (0xc)  //better to get the value from hardware
#define LBA_SIZE 				        (0x1000)
//#define XDMA_CROSSBAR_ADDR              (0x0000000100000000)
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
/***********************
 * ourNVMe Segement end
 * ******************/

#define RESERVED1_END_ADDR					XPAR_MIG_0_HIGHADDR
#define DRAM_END_ADDR						XPAR_MIG_0_HIGHADDR

#endif /* MEMORY_MAP_H_ */
