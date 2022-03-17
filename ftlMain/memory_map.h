#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_




#include "metadata_management.h"
#include "xparameters.h"

#define DRAM_START_ADDR					XPAR_DDR4_0_BASEADDR + 0x00100000//XPAR_MIG_0_BASEADDR //xparameters

#define CH0_META_DATA_ADDR              DRAM_START_ADDR                 	//size=0x2000
#define CH1_META_DATA_ADDR              CH0_META_DATA_ADDR + 0x2000     	//size=0x2000
#define TOTAL_META_DATA_ADDR            CH1_META_DATA_ADDR + 0x2000     	//size=0x4000
#define GEOMETRY_DATA_ADDR              TOTAL_META_DATA_ADDR + 0x4000   	//size=0x1000
#define PREDEFINED_DATA_ADDR            GEOMETRY_DATA_ADDR + 0x1000     	//size=0x2000
#define DSMPRP_DATA_ADDR                PREDEFINED_DATA_ADDR + 0x2000   	//size=0x2000
#define LBA_LIST_ADDR                   DSMPRP_DATA_ADDR + 0x2000       	//size=0x1000
#define VECTOR_RESET_CHUNK_DES_ADDR		LBA_LIST_ADDR + 0x1000				//size=0x1000
#define CHUNK_LOG_NOTIFICATION_ENTRY	VECTOR_RESET_CHUNK_DES_ADDR + 0x1000//size=0x1000
#define PL_AD_PRP_BUF_BASEADDR	        CHUNK_LOG_NOTIFICATION_ENTRY + 0x1000
/**************************************************************
 ******************* ourNVMe Segment begin *******************
 **************************************************************/
#define MEM_PAGE_WIDTH                  (0xc)  //better to get the value from hardware
#define PRP_PAGE_SIZE                   (1<<MEM_PAGE_WIDTH)
#define LBA_SIZE 				        (0x2000)
#define FAKE_LBA_SIZE					(0x1000)

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
