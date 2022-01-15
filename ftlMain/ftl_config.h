//write fail Write Next Unit
//Write fail Chunk Early Close
#ifndef FTL_CONFIG_H_
#define FTL_CONFIG_H_

#include "../nsc_driver/nsc_driver.h"
#include "xparameters.h"
#include "../nvme/nvme_structs.h"
#include "../nvme/debug.h"

#define WS_OPT_DATA     2  //2 plane
#define MW_CUNITS_DATA  2  //equal to  WS_OPT_DATA
#define MULTI_RESET_EN  1
//checks NSC connection, initializes base address
#ifdef	XPAR_NANDFLASHCONTROLLER_1_BASEADDR
#define NSC_1_CONNECTED	1
#define NSC_1_BASEADDR	XPAR_NANDFLASHCONTROLLER_1_BASEADDR
#define NSC_1_MASK      0x02
#else
#define NSC_1_CONNECTED	0
#define NSC_1_BASEADDR	0
#define NSC_1_MASK      0x00
#endif
#ifdef	XPAR_NANDFLASHCONTROLLER_0_BASEADDR
#define NSC_0_CONNECTED	1
#define NSC_0_BASEADDR	XPAR_NANDFLASHCONTROLLER_0_BASEADDR
#define NSC_0_MASK      0x01
#else
#define NSC_0_CONNECTED	0
#define NSC_0_BASEADDR	0
#define NSC_0_MASK      0x00
#endif

//number of connected (=AXI mapped) NSC
#define NSC_TOTAL_MASK  NSC_1_MASK | NSC_0_MASK
#define NUMBER_OF_CONNECTED_CHANNEL (NSC_1_CONNECTED + NSC_0_CONNECTED)


//--------------------------------
//NAND flash memory specifications
//--------------------------------

#define	BYTES_PER_DATA_REGION_OF_NAND_ROW		8192
#define	BYTES_PER_SPARE_REGION_OF_NAND_ROW		448
#define BYTES_PER_NAND_ROW						(BYTES_PER_DATA_REGION_OF_NAND_ROW + BYTES_PER_SPARE_REGION_OF_NAND_ROW)

#define	ROWS_PER_SLC_BLOCK			128
//#define	ROWS_PER_MLC_BLOCK			256
//zheng
//4096 0
#define	MAIN_BLOCKS_PER_LUN			4080
#define EXTENDED_BLOCKS_PER_LUN		16
#define TOTAL_BLOCKS_PER_LUN		(MAIN_BLOCKS_PER_LUN + EXTENDED_BLOCKS_PER_LUN)

#define	MAIN_ROWS_PER_SLC_LUN		(ROWS_PER_SLC_BLOCK * MAIN_BLOCKS_PER_LUN)
//#define	MAIN_ROWS_PER_MLC_LUN		(ROWS_PER_MLC_BLOCK * MAIN_BLOCKS_PER_LUN)

#define	LUNS_PER_DIE				1

#define	MAIN_BLOCKS_PER_DIE			(MAIN_BLOCKS_PER_LUN * LUNS_PER_DIE)
#define TOTAL_BLOCKS_PER_DIE		(TOTAL_BLOCKS_PER_LUN * LUNS_PER_DIE)

#define BAD_BLOCK_MARK_PAGE0		0										//first row of a block
#define BAD_BLOCK_MARK_PAGE1		(ROWS_PER_SLC_BLOCK - 1)				//last row of a block
#define BAD_BLOCK_MARK_BYTE0 		0										//first byte of data region of the row
#define BAD_BLOCK_MARK_BYTE1 		(BYTES_PER_DATA_REGION_OF_NAND_ROW)		//first byte of spare region of the row




//------------------------------------
//NAND storage controller specifications
//------------------------------------

//supported maximum channel/way structure
#define	NSC_MAX_CHANNELS				(NUMBER_OF_CONNECTED_CHANNEL)
#define	NSC_MAX_WAYS					1

//row -> page
#define	BYTES_PER_DATA_REGION_OF_PAGE			8192
#define BYTES_PER_SPARE_REGION_OF_PAGE			448		//last 8 byte used by ECC engine (CRC function)
// (BYTES_PER_SPARE_REGION_OF_NAND_ROW - BYTES_PER_SPARE_REGION_OF_PAGE) bytes are used by ECC engine (Parity data)
#define	PAGES_PER_SLC_BLOCK			(ROWS_PER_SLC_BLOCK)
//#define	PAGES_PER_MLC_BLOCK			(ROWS_PER_MLC_BLOCK)

//ECC encoder/decoder specification
#define ECC_CHUNKS_PER_PAGE				32
#define BIT_ERROR_THRESHOLD_PER_CHUNK	20
#define ERROR_INFO_WORD_COUNT 			11


//------------------------------
//NVMe Controller Specifications
//------------------------------

#define	BYTES_PER_NVME_BLOCK		8192
//#define NVME_BLOCKS_PER_PAGE		(BYTES_PER_DATA_REGION_OF_PAGE / BYTES_PER_NVME_BLOCK)



//------------------
//FTL configurations
//------------------

#define	SLC_MODE				1
//#define	MLC_MODE				2

//************************************************************************
#define	BITS_PER_FLASH_CELL		SLC_MODE	//user configurable factor
//#define	USER_BLOCKS_PER_LUN		4096		//user configurable factor
//zheng 4096
#define	USER_BLOCKS_PER_LUN		4080		//user configurable factor
#define	USER_CHANNELS			(NUMBER_OF_CONNECTED_CHANNEL)		//user configurable factor
#define	USER_WAYS				1			//user configurable factor
//************************************************************************

#define	BYTES_PER_DATA_REGION_OF_SLICE		8192		//slice is a mapping unit of FTL
#define	BYTES_PER_SPARE_REGION_OF_SLICE		448

#define SLICES_PER_PAGE				(BYTES_PER_DATA_REGION_OF_PAGE / BYTES_PER_DATA_REGION_OF_SLICE)	//a slice directs a page, full page mapping
#define NVME_BLOCKS_PER_SLICE		(BYTES_PER_DATA_REGION_OF_SLICE / BYTES_PER_NVME_BLOCK)
#define PRP_PAGES_PER_SLICE		    (BYTES_PER_DATA_REGION_OF_SLICE / PRP_PAGE_SIZE)

#define	USER_DIES					(USER_CHANNELS * USER_WAYS)

#define	USER_PAGES_PER_BLOCK		(PAGES_PER_SLC_BLOCK * BITS_PER_FLASH_CELL)
#define	USER_PAGES_PER_LUN			(USER_PAGES_PER_BLOCK * USER_BLOCKS_PER_LUN)
#define	USER_PAGES_PER_DIE			(USER_PAGES_PER_LUN * LUNS_PER_DIE)
#define	USER_PAGES_PER_CHANNEL		(USER_PAGES_PER_DIE * USER_WAYS)
#define	USER_PAGES_PER_SSD			(USER_PAGES_PER_CHANNEL * USER_CHANNELS)

#define	SLICES_PER_BLOCK			(USER_PAGES_PER_BLOCK * SLICES_PER_PAGE)
#define	SLICES_PER_LUN				(USER_PAGES_PER_LUN * SLICES_PER_PAGE)
#define	SLICES_PER_DIE				(USER_PAGES_PER_DIE * SLICES_PER_PAGE)
#define	SLICES_PER_CHANNEL			(USER_PAGES_PER_CHANNEL * SLICES_PER_PAGE)
#define	SLICES_PER_SSD				(USER_PAGES_PER_SSD * SLICES_PER_PAGE)

#define	USER_BLOCKS_PER_DIE			(USER_BLOCKS_PER_LUN * LUNS_PER_DIE)
#define	USER_BLOCKS_PER_CHANNEL		(USER_BLOCKS_PER_DIE * USER_WAYS)
#define	USER_BLOCKS_PER_SSD			(USER_BLOCKS_PER_CHANNEL * USER_CHANNELS)

#define	MB_PER_BLOCK						((BYTES_PER_DATA_REGION_OF_SLICE * SLICES_PER_BLOCK) / (1024 * 1024))
#define MB_PER_SSD							(USER_BLOCKS_PER_SSD * MB_PER_BLOCK)
#define MB_PER_MIN_FREE_BLOCK_SPACE			(USER_DIES * MB_PER_BLOCK)
#define MB_PER_METADATA_BLOCK_SPACE			(USER_DIES * MB_PER_BLOCK)
#define MB_PER_OVER_PROVISION_BLOCK_SPACE	((USER_BLOCKS_PER_SSD / 10) * MB_PER_BLOCK)
//------------------
// OCSSD PARAM
//------------------
#define CHUNK_NUM_PER_PU 256
#define LB_ADDR_LENGTH 7
#define CHUNK_ADDR_LENGTH 8
#define PU_ADDR_LENGTH 0    //nvme struct h
#define GROUP_ADDR_LENGTH 1
#define LEFT_LENGTH 32 - GROUP_ADDR_LENGTH - PU_ADDR_LENGTH - CHUNK_ADDR_LENGTH - LB_ADDR_LENGTH
void InitFTL();
void InitNandArray();

extern unsigned int storageCapacity_L;

#endif /* FTL_CONFIG_H_ */

//---------------------
//      DEBUG
//---------------------

//!!!!!!!!!!!!!!!!!!!!!!!
#define ERASE_ALL_DEBUG 		//Erase All !!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!

#define NVME_DEBUG
#define ADDR_DEBUG
#define MAIN_DEBUG
#define LEVEL_DEBUG 	  //ReqTransSliceToLowLevel
#define INIT_DEBUG
#define REQ_DEBUG
#define AD_DEBUG
#define IO_DEBUG
#define XDMA_DEBUG
#define META_DEBUG
#define SLICE_DEBUG
#define RESET_DEBUG
#define VECTOR_DEBUG

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef ERASE_ALL_DEBUG
#define ERASE_ALL  EraseAll
#else
#define ERASE_ALL(...)
#endif
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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


#ifdef SLICE_DEBUG
#define SLICE_PRINT  xil_printf
#else
#define SLICE_PRINT(...)
#endif

#ifdef RESET_DEBUG
#define RESET_PRINT  xil_printf
#else
#define RESET_PRINT(...)
#endif

#ifdef VECTOR_DEBUG
#define VECTOR_PRINT  xil_printf
#else
#define VECTOR_PRINT(...)
#endif

///////////////////////



