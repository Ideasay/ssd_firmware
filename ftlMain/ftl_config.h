
#ifndef FTL_CONFIG_H_
#define FTL_CONFIG_H_

#include "nsc_driver.h"
#include "xparameters.h"
#include "nvme/nvme.h"


#define NVME_SIM        1
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

#define	BYTES_PER_NVME_BLOCK		4096
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


void InitFTL();
void InitChCtlReg();
void InitNandArray();
void CheckConfigRestriction();

extern unsigned int storageCapacity_L;
extern V2FMCRegisters* chCtlReg[USER_CHANNELS];

#endif /* FTL_CONFIG_H_ */
