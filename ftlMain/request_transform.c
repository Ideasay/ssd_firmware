//////////////////////////////////////////////////////////////////////////////////
// request_transform.c for Cosmos+ OpenSSD
// Copyright (c) 2017 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//			      Sangjin Lee <sjlee@enc.hanyang.ac.kr>
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
// Module Name: Request Scheduler
// File Name: request_transform.c
//
// Version: v1.0.0
//
// Description:
//	 - transform request information
//   - check dependency between requests
//   - issue host DMA request to host DMA engine
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////


#include "xil_printf.h"
#include <assert.h>
#include "../nvme/nvme.h"
#include "../nvme/nvme_structs.h"
#include "../nvme/host_lld.h"
#include "../nvme/memory_map.h"
#include "ftl_config.h"
#include "generate.h"
#include "../nvme/nvme_io_cmds.h"


P_ROW_ADDR_DEPENDENCY_TABLE rowAddrDependencyTablePtr;

void InitDependencyTable()
{
	unsigned int blockNo, wayNo, chNo;
	rowAddrDependencyTablePtr = (P_ROW_ADDR_DEPENDENCY_TABLE)ROW_ADDR_DEPENDENCY_TABLE_ADDR;

	for(blockNo=0 ; blockNo<MAIN_BLOCKS_PER_DIE ; blockNo++)
	{
		for(wayNo=0 ; wayNo<USER_WAYS ; wayNo++)
		{
			for(chNo=0 ; chNo<USER_CHANNELS ; chNo++)
			{
				rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage = 0;
				rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedReadReqCnt = 0;
				rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedEraseReqFlag = 0;
			}
		}
	}
}

void ReqTransNvmeToSlice(unsigned int cmdSlotTag, unsigned int startLba, unsigned int nlb, unsigned int cmdCode, u64 prp1ForReq, u64 prp2ForReq, int prpNum, int start_offset, int data_length, nvme_cq_entry_t *cq_entry)
{
	unsigned int reqSlotTag, requestedNvmeBlock, tempNumOfNvmeBlock, transCounter, tempLsa, loop, nvmeBlockOffset, nvmeDmaStartIndex, reqCode;
	//new added for transforming req
	u64 prpCollectedForSlice[prpNum];
	int dataLengthForSlice[prpNum];
	int tempLastSliceOfNvme;
	int left_length;

	u32 i;
	u32 p , t,prp_array_transfer_count;
	u32 tmp_prp_num;
	u32 tmp_prp_num_for_cycle;
	//init the first prp
	int prpCnt = 0;
	u32 offset_mask = (1<<MEM_PAGE_WIDTH)-1;
	requestedNvmeBlock = nlb + 1;

	transCounter = 0;
	nvmeDmaStartIndex = 0;
	tempLsa = startLba / NVME_BLOCKS_PER_SLICE;
	loop = ((startLba % NVME_BLOCKS_PER_SLICE) + requestedNvmeBlock) / NVME_BLOCKS_PER_SLICE;
	//xil_printf("ReqTransNvmeToSlice here!\n\r");
	//xil_printf("startLba is 0x%x\n\r",startLba);
	//xil_printf("start_offset is 0x%x\n\r",start_offset);
	//xil_printf("prpNum is 0x%x\n\r",prpNum);
	//xil_printf("requestedNvmeBlock is 0x%x\n\r",requestedNvmeBlock);
	//xil_printf("loop is 0x%x\n\r",loop);
	if(cmdCode == IO_NVM_WRITE)
	{
		reqCode = REQ_CODE_WRITE;
		//xil_printf("WRITE (to slice)!\n\r");
	}
	else if(cmdCode == IO_NVM_READ)
	{
		reqCode = REQ_CODE_READ;
		//xil_printf("READ (to slice)!\n\r");
	}
	else
		assert(!"[WARNING] Not supported command code [WARNING]");
	if(prpNum == 1)
	{
		prpCnt = 1;
		prpCollectedForSlice[0] = prp1ForReq;
		dataLengthForSlice[0] = LBA_SIZE - start_offset;
		//xil_printf("prpCollectedForSlice[0] is 0x%x!\n\r",prpCollectedForSlice[0]);
		//xil_printf("dataLengthForSlice[0] is 0x%x!\n\r",dataLengthForSlice[0]);
	}
	else if(prpNum == 2)
	{
		prpCnt = 2;
		prpCollectedForSlice[0] = prp1ForReq;
		prpCollectedForSlice[1] = prp2ForReq & (0-(1<<12));
		dataLengthForSlice[0] = LBA_SIZE - start_offset;
		dataLengthForSlice[1] = data_length - dataLengthForSlice[0];
		//xil_printf("prpCollectedForSlice[0] is 0x%x!\n\r",prpCollectedForSlice[0]);
		//xil_printf("prpCollectedForSlice[1] is 0x%x!\n\r",prpCollectedForSlice[1]);
		//xil_printf("dataLengthForSlice[0] is 0x%x!\n\r",dataLengthForSlice[0]);
		//xil_printf("dataLengthForSlice[1] is 0x%x!\n\r",dataLengthForSlice[1]);
	}
	else
	{
		//add prp[0]
		prpCollectedForSlice[0] = prp1ForReq;
		prpCnt = 1;
		dataLengthForSlice[0] = LBA_SIZE - start_offset;
		left_length = data_length - dataLengthForSlice[0];
		int left_prp_num = prpNum-1;
		//next prp should be restored in prpCollectedForSlice array also.
		u64 next_prplist_addr = prp2ForReq;
		next_prplist_addr = next_prplist_addr & (0-(1<<2));
		//prp_list_start_offset = (u32)(prp[1]) & offset_mask;
		int prp_list_start_offset = (u32)(next_prplist_addr) & ((1<<MEM_PAGE_WIDTH)-1);
		int prp_offset = prp_list_start_offset >>3;
		int prp_max_num = 1<<(MEM_PAGE_WIDTH-3);

		for (;left_prp_num>0;)
		{
			if(left_prp_num + prp_offset>prp_max_num)
			{
				tmp_prp_num = prp_max_num - prp_offset;
				left_prp_num = left_prp_num - tmp_prp_num +1;
				tmp_prp_num_for_cycle = tmp_prp_num-1;
			}
			else
			{
				tmp_prp_num = left_prp_num;
				left_prp_num = 0;
				tmp_prp_num_for_cycle = tmp_prp_num;
			}

			//we store the prp list in ADDR PL_IO_PRP_BUF_BASEADDR also.
		    write_ioP_h2c_dsc(next_prplist_addr,PL_IO_PRP_BUF_BASEADDR,tmp_prp_num*8);
		    while((get_io_dma_status() & 0x2) == 0);

		    u32 rd_data[2];
		    u64 addr_total;
		    for( i=0;i<tmp_prp_num_for_cycle;i++)
		    {
			    rd_data[0]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8);
			    rd_data[1]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8+4);
			    addr_total=((u64)(rd_data[0]))+((u64)(rd_data[1])<<32);
				addr_total= addr_total & (0-(1<<12)) ;
			    prpCollectedForSlice[prpCnt] = addr_total;
				if(left_length>LBA_SIZE)
				{
					dataLengthForSlice[prpCnt] = LBA_SIZE;
				}
			    else
			    {
			    	dataLengthForSlice[prpCnt] = left_length;
			    }
				left_length = left_length - dataLengthForSlice[prpCnt++];//tmp_length
		    }
		    if(tmp_prp_num_for_cycle == tmp_prp_num-1)
		    {
			    rd_data[0]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8);
			    rd_data[1]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8+4);
			    next_prplist_addr = ((u64)(rd_data[0]))+((u64)(rd_data[1])<<32);
			    next_prplist_addr = next_prplist_addr & (0-(1<<2)) ;
			    prp_offset = (next_prplist_addr & offset_mask)>>3;
		    }
		}
	}
	prp_array_transfer_count = 0;
	//first transform
	nvmeBlockOffset = (startLba % NVME_BLOCKS_PER_SLICE);
	if(loop)
		tempNumOfNvmeBlock = NVME_BLOCKS_PER_SLICE - nvmeBlockOffset;
	else
		tempNumOfNvmeBlock = requestedNvmeBlock;

	reqSlotTag = GetFromFreeReqQ();

	reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_SLICE;
	reqPoolPtr->reqPool[reqSlotTag].reqCode = reqCode;
	reqPoolPtr->reqPool[reqSlotTag].nvmeCmdSlotTag = cmdSlotTag;
	reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = tempLsa;
	reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.startIndex = nvmeDmaStartIndex;
	reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.nvmeBlockOffset = nvmeBlockOffset;  //0,1   2��nvme block��Ӧһ���洢slice����������������еĵڼ���
	reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock = tempNumOfNvmeBlock;//2,1        ���δ���slice�а�����nvme block��Ŀ
	if(start_offset)
	{
		reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist = 1;
		t = nvmeBlockOffset;
		//p = 0;
		reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
	    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
	    t++;

		for(p = 1 ; p < tempNumOfNvmeBlock ; p++)//tempNumOfNvmeBlock=2,t=0 p=012 tempNumOfNvmeBlock=1,   p=01
		{
			reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
		    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
		    t++;
		}
		//p = tempNumOfNvmeBlock;
		reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
	    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = start_offset;//+++++++++++++++++++
	    prpCollectedForSlice[prp_array_transfer_count] = prpCollectedForSlice[prp_array_transfer_count] + start_offset;
	    dataLengthForSlice[prp_array_transfer_count] = dataLengthForSlice[prp_array_transfer_count] - start_offset;
	}
	else
	{
		reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist = 0;
		t = nvmeBlockOffset;
		for(p = 0 ; p < tempNumOfNvmeBlock; p++)
		{
			reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
		    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
		    t++;
		}
	}
	tempLastSliceOfNvme = reqSlotTag;
	reqPoolPtr->reqPool[tempLastSliceOfNvme].cqEn = 1;
	PutToSliceReqQ(reqSlotTag);

	tempLsa++;
	transCounter++;
	nvmeDmaStartIndex += tempNumOfNvmeBlock;

	//transform continue
	while(transCounter < loop)
	{
		nvmeBlockOffset = 0;
		tempNumOfNvmeBlock = NVME_BLOCKS_PER_SLICE;

		reqSlotTag = GetFromFreeReqQ();

		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_SLICE;
		reqPoolPtr->reqPool[reqSlotTag].reqCode = reqCode;
		reqPoolPtr->reqPool[reqSlotTag].nvmeCmdSlotTag = cmdSlotTag;
		reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = tempLsa;
		reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.startIndex = nvmeDmaStartIndex;
		reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.nvmeBlockOffset = nvmeBlockOffset;
		reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock = tempNumOfNvmeBlock;

		if(start_offset)
		{
			reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist = 1;
			t = nvmeBlockOffset;
			//p = 0;
			reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
		    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
		    t++;

			for(p = 1 ; p < tempNumOfNvmeBlock ; p++)//tempNumOfNvmeBlock=2,t=0 p=012 tempNumOfNvmeBlock=1,   p=01
			{
				reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
			    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
			    t++;
			}
			//p = tempNumOfNvmeBlock;
			reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
		    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = start_offset;//+++++++++++++++++++
		    prpCollectedForSlice[prp_array_transfer_count] = prpCollectedForSlice[prp_array_transfer_count] + start_offset;
		    dataLengthForSlice[prp_array_transfer_count] = dataLengthForSlice[prp_array_transfer_count] - start_offset;
		}
		else
		{
			reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist = 0;
			t = nvmeBlockOffset;
			for(p = 0 ; p < tempNumOfNvmeBlock; p++)
			{
				reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
			    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
			    t++;
			}
		}
		reqPoolPtr->reqPool[tempLastSliceOfNvme].cqEn = 0;
		tempLastSliceOfNvme = reqSlotTag;
		reqPoolPtr->reqPool[tempLastSliceOfNvme].cqEn = 1;
		PutToSliceReqQ(reqSlotTag);

		tempLsa++;
		transCounter++;
		nvmeDmaStartIndex += tempNumOfNvmeBlock;
	}


	//last transform
	nvmeBlockOffset = 0;
	tempNumOfNvmeBlock = (startLba + requestedNvmeBlock) % NVME_BLOCKS_PER_SLICE;
	if((tempNumOfNvmeBlock == 0) || (loop == 0))
		reqPoolPtr->reqPool[tempLastSliceOfNvme].cqEntry = cq_entry;
		return ;

	reqSlotTag = GetFromFreeReqQ();

	reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_SLICE;
	reqPoolPtr->reqPool[reqSlotTag].reqCode = reqCode;
	reqPoolPtr->reqPool[reqSlotTag].nvmeCmdSlotTag = cmdSlotTag;
	reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = tempLsa;
	reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.startIndex = nvmeDmaStartIndex;
	reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.nvmeBlockOffset = nvmeBlockOffset;
	reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock = tempNumOfNvmeBlock;
	if(start_offset)
	{
		reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist = 1;
		t = nvmeBlockOffset;
		//p = 0;
		reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
	    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
	    t++;

		for(p = 1 ; p < tempNumOfNvmeBlock ; p++)//tempNumOfNvmeBlock=2,t=0 p=012 tempNumOfNvmeBlock=1,   p=01
		{
			reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
		    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
		    t++;
		}
		//p = tempNumOfNvmeBlock;
		reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
	    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = start_offset;//+++++++++++++++++++
	    //prpCollectedForSlice[prp_array_transfer_count] = prpCollectedForSlice[prp_array_transfer_count] + start_offset;
	    //dataLengthForSlice[prp_array_transfer_count] = dataLengthForSlice[prp_array_transfer_count] - start_offset;
	}
	else
	{
		reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist = 0;
		t = nvmeBlockOffset;
		for(p = 0 ; p < tempNumOfNvmeBlock; p++)
		{
			reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t] = prpCollectedForSlice[prp_array_transfer_count];//+++++++++++++++++++
		    reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t] = dataLengthForSlice[prp_array_transfer_count++];//+++++++++++++++++++
		    t++;
		}
	}
	reqPoolPtr->reqPool[tempLastSliceOfNvme].cqEn = 0;
	tempLastSliceOfNvme = reqSlotTag;
	reqPoolPtr->reqPool[tempLastSliceOfNvme].cqEn = 1;
	reqPoolPtr->reqPool[tempLastSliceOfNvme].cqEntry = cq_entry;
	PutToSliceReqQ(reqSlotTag);
	//xil_printf("ReqTransNvmeToSlice end!\n\r");
}



void EvictDataBufEntry(unsigned int originReqSlotTag)
{
	unsigned int reqSlotTag, virtualSliceAddr, dataBufEntry;

	dataBufEntry = reqPoolPtr->reqPool[originReqSlotTag].dataBufInfo.entry;
	if(dataBufMapPtr->dataBuf[dataBufEntry].dirty == DATA_BUF_DIRTY)
	{
		reqSlotTag = GetFromFreeReqQ();
		virtualSliceAddr =  AddrTransWrite(dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr);

		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
		reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_WRITE;
		reqPoolPtr->reqPool[reqSlotTag].nvmeCmdSlotTag = reqPoolPtr->reqPool[originReqSlotTag].nvmeCmdSlotTag;
		reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ENTRY;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;
		reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = dataBufEntry;
		UpdateDataBufEntryInfoBlockingReq(dataBufEntry, reqSlotTag);
		reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = virtualSliceAddr;

		SelectLowLevelReqQ(reqSlotTag);

		dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_CLEAN;
	}
}

void DataReadFromNand(unsigned int originReqSlotTag)
{
	unsigned int reqSlotTag, virtualSliceAddr;

	virtualSliceAddr =  AddrTransRead(reqPoolPtr->reqPool[originReqSlotTag].logicalSliceAddr);

	if(virtualSliceAddr != VSA_FAIL)
	{
		reqSlotTag = GetFromFreeReqQ();

		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
		reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
		reqPoolPtr->reqPool[reqSlotTag].nvmeCmdSlotTag = reqPoolPtr->reqPool[originReqSlotTag].nvmeCmdSlotTag;
		reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = reqPoolPtr->reqPool[originReqSlotTag].logicalSliceAddr;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ENTRY;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;

		reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = reqPoolPtr->reqPool[originReqSlotTag].dataBufInfo.entry;
		UpdateDataBufEntryInfoBlockingReq(reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry, reqSlotTag);
		reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = virtualSliceAddr;

		SelectLowLevelReqQ(reqSlotTag);
	}
}


void ReqTransSliceToLowLevel()
{
	unsigned int reqSlotTag, dataBufEntry;
	//xil_printf("ReqTransSliceToLowLevel here!\n\r");
	while(sliceReqQ.headReq != REQ_SLOT_TAG_NONE)
	{
		reqSlotTag = GetFromSliceReqQ();
		if(reqSlotTag == REQ_SLOT_TAG_FAIL)
			return ;

		//allocate a data buffer entry for this request
		dataBufEntry = CheckDataBufHit(reqSlotTag);
		if(dataBufEntry != DATA_BUF_FAIL)
		{
			//data buffer hit
			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = dataBufEntry;
		}
		else
		{
			//data buffer miss, allocate a new buffer entry
			dataBufEntry = AllocateDataBuf();
			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = dataBufEntry;

			//clear the allocated data buffer entry being used by a previous request
			EvictDataBufEntry(reqSlotTag);

			//update meta-data of the allocated data buffer entry
			dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr;
			PutToDataBufHashList(dataBufEntry);

			if(reqPoolPtr->reqPool[reqSlotTag].reqCode  == REQ_CODE_READ)
				DataReadFromNand(reqSlotTag);
			else if(reqPoolPtr->reqPool[reqSlotTag].reqCode  == REQ_CODE_WRITE)
				if(reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock != NVME_BLOCKS_PER_SLICE) //for read modify write
					DataReadFromNand(reqSlotTag);
		}

		//transform this slice request to nvme request
		if(reqPoolPtr->reqPool[reqSlotTag].reqCode  == REQ_CODE_WRITE)
		{
			dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_DIRTY;
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_RxDMA;
		}
		else if(reqPoolPtr->reqPool[reqSlotTag].reqCode  == REQ_CODE_READ)
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_TxDMA;
		else
			assert(!"[WARNING] Not supported reqCode. [WARNING]");

		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NVME_DMA;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ENTRY;

		UpdateDataBufEntryInfoBlockingReq(dataBufEntry, reqSlotTag);
		SelectLowLevelReqQ(reqSlotTag);
	}
}

unsigned int CheckBufDep(unsigned int reqSlotTag)
{
	if(reqPoolPtr->reqPool[reqSlotTag].prevBlockingReq == REQ_SLOT_TAG_NONE)
		return BUF_DEPENDENCY_REPORT_PASS;
	else
		return BUF_DEPENDENCY_REPORT_BLOCKED;
}


unsigned int CheckRowAddrDep(unsigned int reqSlotTag, unsigned int checkRowAddrDepOpt)
{
	unsigned int dieNo,chNo, wayNo, blockNo, pageNo;

	if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr == REQ_OPT_NAND_ADDR_VSA)
	{
		dieNo = Vsa2VdieTranslation(reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
		chNo =  Vdie2PchTranslation(dieNo);
		wayNo = Vdie2PwayTranslation(dieNo);
		blockNo = Vsa2VblockTranslation(reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
		pageNo = Vsa2VpageTranslation(reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
	}
	else
		assert(!"[WARNING] Not supported reqOpt-nandAddress [WARNING]");

	if(reqPoolPtr->reqPool[reqSlotTag].reqCode == REQ_CODE_READ)
	{
		if(checkRowAddrDepOpt == ROW_ADDR_DEPENDENCY_CHECK_OPT_SELECT)
		{
			if(rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedEraseReqFlag)
				SyncReleaseEraseReq(chNo, wayNo, blockNo);

			if(pageNo < rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage)
				return ROW_ADDR_DEPENDENCY_REPORT_PASS;

			rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedReadReqCnt++;
		}
		else if(checkRowAddrDepOpt == ROW_ADDR_DEPENDENCY_CHECK_OPT_RELEASE)
		{
			if(pageNo < rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage)
			{
				rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedReadReqCnt--;
				return	ROW_ADDR_DEPENDENCY_REPORT_PASS;
			}
		}
		else
			assert(!"[WARNING] Not supported checkRowAddrDepOpt [WARNING]");
	}
	else if(reqPoolPtr->reqPool[reqSlotTag].reqCode == REQ_CODE_WRITE)
	{
		if(pageNo == rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage)
		{
			rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage++;

			return ROW_ADDR_DEPENDENCY_REPORT_PASS;
		}
	}
	else if(reqPoolPtr->reqPool[reqSlotTag].reqCode == REQ_CODE_ERASE)
	{
		if(rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage == reqPoolPtr->reqPool[reqSlotTag].nandInfo.programmedPageCnt)
			if(rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedReadReqCnt == 0)
			{
				rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage = 0;
				rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedEraseReqFlag = 0;

				return ROW_ADDR_DEPENDENCY_REPORT_PASS;
			}

		if(checkRowAddrDepOpt == ROW_ADDR_DEPENDENCY_CHECK_OPT_SELECT)
			rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedEraseReqFlag = 1;
		else if(checkRowAddrDepOpt == ROW_ADDR_DEPENDENCY_CHECK_OPT_RELEASE)
		{
			//pass, go to return
		}
		else
			assert(!"[WARNING] Not supported checkRowAddrDepOpt [WARNING]");
	}
	else
		assert(!"[WARNING] Not supported reqCode [WARNING]");

	return ROW_ADDR_DEPENDENCY_REPORT_BLOCKED;
}


unsigned int UpdateRowAddrDepTableForBufBlockedReq(unsigned int reqSlotTag)
{
	unsigned int dieNo, chNo, wayNo, blockNo, pageNo, bufDepCheckReport;

	if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr == REQ_OPT_NAND_ADDR_VSA)
	{
		dieNo = Vsa2VdieTranslation(reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
		chNo =  Vdie2PchTranslation(dieNo);
		wayNo = Vdie2PwayTranslation(dieNo);
		blockNo = Vsa2VblockTranslation(reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
		pageNo = Vsa2VpageTranslation(reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
	}
	else
		assert(!"[WARNING] Not supported reqOpt-nandAddress [WARNING]");

	if(reqPoolPtr->reqPool[reqSlotTag].reqCode == REQ_CODE_READ)
	{
		if(rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedEraseReqFlag)
		{
			SyncReleaseEraseReq(chNo, wayNo, blockNo);

			bufDepCheckReport = CheckBufDep(reqSlotTag);
			if(bufDepCheckReport == BUF_DEPENDENCY_REPORT_PASS)
			{
				if(pageNo < rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].permittedProgPage)
					PutToNandReqQ(reqSlotTag, chNo, wayNo);
				else
				{
					rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedReadReqCnt++;
					PutToBlockedByRowAddrDepReqQ(reqSlotTag, chNo, wayNo);
				}

				return ROW_ADDR_DEPENDENCY_TABLE_UPDATE_REPORT_SYNC;
			}
		}
		rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedReadReqCnt++;
	}
	else if(reqPoolPtr->reqPool[reqSlotTag].reqCode == REQ_CODE_ERASE)
		rowAddrDependencyTablePtr->block[chNo][wayNo][blockNo].blockedEraseReqFlag = 1;

	return ROW_ADDR_DEPENDENCY_TABLE_UPDATE_REPORT_DONE;
}



void SelectLowLevelReqQ(unsigned int reqSlotTag)
{
	unsigned int dieNo, chNo, wayNo, bufDepCheckReport, rowAddrDepCheckReport, rowAddrDepTableUpdateReport;

	bufDepCheckReport = CheckBufDep(reqSlotTag);
	if(bufDepCheckReport == BUF_DEPENDENCY_REPORT_PASS)
	{
		//here we trigger the dma between nvme and data buffer. and now we should replace it with our own logic.
		if(reqPoolPtr->reqPool[reqSlotTag].reqType  == REQ_TYPE_NVME_DMA)
		{
			IssueNvmeDmaReq(reqSlotTag);
			PutToNvmeDmaReqQ(reqSlotTag);
		}
		else if(reqPoolPtr->reqPool[reqSlotTag].reqType  == REQ_TYPE_NAND)
		{
			if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr == REQ_OPT_NAND_ADDR_VSA)
			{
				dieNo = Vsa2VdieTranslation(reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
				chNo =  Vdie2PchTranslation(dieNo);
				wayNo = Vdie2PwayTranslation(dieNo);
			}
			else if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr == REQ_OPT_NAND_ADDR_PHY_ORG)
			{
				chNo =  reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh;
				wayNo = reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay;
			}
			else
				assert(!"[WARNING] Not supported reqOpt-nandAddress [WARNING]");

			if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck == REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK)
			{
				rowAddrDepCheckReport = CheckRowAddrDep(reqSlotTag, ROW_ADDR_DEPENDENCY_CHECK_OPT_SELECT);

				if(rowAddrDepCheckReport == ROW_ADDR_DEPENDENCY_REPORT_PASS)
					PutToNandReqQ(reqSlotTag, chNo, wayNo);
				else if(rowAddrDepCheckReport == ROW_ADDR_DEPENDENCY_REPORT_BLOCKED)
					PutToBlockedByRowAddrDepReqQ(reqSlotTag, chNo, wayNo);
				else
					assert(!"[WARNING] Not supported report [WARNING]");
			}
			else if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck == REQ_OPT_ROW_ADDR_DEPENDENCY_NONE)
				PutToNandReqQ(reqSlotTag, chNo, wayNo);
			else
				assert(!"[WARNING] Not supported reqOpt [WARNING]");

		}
		else
			assert(!"[WARNING] Not supported reqType [WARNING]");
	}
	else if(bufDepCheckReport == BUF_DEPENDENCY_REPORT_BLOCKED)
	{
		if(reqPoolPtr->reqPool[reqSlotTag].reqType  == REQ_TYPE_NAND)
			if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck == REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK)
			{
				rowAddrDepTableUpdateReport = UpdateRowAddrDepTableForBufBlockedReq(reqSlotTag);

				if(rowAddrDepTableUpdateReport == ROW_ADDR_DEPENDENCY_TABLE_UPDATE_REPORT_DONE)
				{
					//pass, go to PutToBlockedByBufDepReqQ
				}
				else if(rowAddrDepTableUpdateReport == ROW_ADDR_DEPENDENCY_TABLE_UPDATE_REPORT_SYNC)
					return;
				else
					assert(!"[WARNING] Not supported report [WARNING]");
			}

		PutToBlockedByBufDepReqQ(reqSlotTag);
	}
	else
		assert(!"[WARNING] Not supported report [WARNING]");
}


void ReleaseBlockedByBufDepReq(unsigned int reqSlotTag)
{
	unsigned int targetReqSlotTag, dieNo, chNo, wayNo, rowAddrDepCheckReport;

	targetReqSlotTag = REQ_SLOT_TAG_NONE;
	if(reqPoolPtr->reqPool[reqSlotTag].nextBlockingReq != REQ_SLOT_TAG_NONE)
	{
		targetReqSlotTag = reqPoolPtr->reqPool[reqSlotTag].nextBlockingReq;
		reqPoolPtr->reqPool[targetReqSlotTag].prevBlockingReq = REQ_SLOT_TAG_NONE;
		reqPoolPtr->reqPool[reqSlotTag].nextBlockingReq = REQ_SLOT_TAG_NONE;
	}

	if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat == REQ_OPT_DATA_BUF_ENTRY)
	{
		if(dataBufMapPtr->dataBuf[reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry].blockingReqTail == reqSlotTag)
			dataBufMapPtr->dataBuf[reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry].blockingReqTail = REQ_SLOT_TAG_NONE;
	}
	else if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat == REQ_OPT_DATA_BUF_TEMP_ENTRY)
	{
		if(tempDataBufMapPtr->tempDataBuf[reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry].blockingReqTail == reqSlotTag)
			tempDataBufMapPtr->tempDataBuf[reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry].blockingReqTail = REQ_SLOT_TAG_NONE;
	}

	if((targetReqSlotTag != REQ_SLOT_TAG_NONE) && (reqPoolPtr->reqPool[targetReqSlotTag].reqQueueType == REQ_QUEUE_TYPE_BLOCKED_BY_BUF_DEP))
	{
		SelectiveGetFromBlockedByBufDepReqQ(targetReqSlotTag);

		if(reqPoolPtr->reqPool[targetReqSlotTag].reqType == REQ_TYPE_NVME_DMA)
		{
			IssueNvmeDmaReq(targetReqSlotTag);
			PutToNvmeDmaReqQ(targetReqSlotTag);
		}
		else if(reqPoolPtr->reqPool[targetReqSlotTag].reqType  == REQ_TYPE_NAND)
		{
			if(reqPoolPtr->reqPool[targetReqSlotTag].reqOpt.nandAddr == REQ_OPT_NAND_ADDR_VSA)
			{
				dieNo = Vsa2VdieTranslation(reqPoolPtr->reqPool[targetReqSlotTag].nandInfo.virtualSliceAddr);
				chNo =  Vdie2PchTranslation(dieNo);
				wayNo = Vdie2PwayTranslation(dieNo);
			}
			else
				assert(!"[WARNING] Not supported reqOpt-nandAddress [WARNING]");

			if(reqPoolPtr->reqPool[targetReqSlotTag].reqOpt.rowAddrDependencyCheck == REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK)
			{
				rowAddrDepCheckReport = CheckRowAddrDep(targetReqSlotTag, ROW_ADDR_DEPENDENCY_CHECK_OPT_RELEASE);

				if(rowAddrDepCheckReport == ROW_ADDR_DEPENDENCY_REPORT_PASS)
					PutToNandReqQ(targetReqSlotTag, chNo, wayNo);
				else if(rowAddrDepCheckReport == ROW_ADDR_DEPENDENCY_REPORT_BLOCKED)
					PutToBlockedByRowAddrDepReqQ(targetReqSlotTag, chNo, wayNo);
				else
					assert(!"[WARNING] Not supported report [WARNING]");
			}
			else if(reqPoolPtr->reqPool[targetReqSlotTag].reqOpt.rowAddrDependencyCheck == REQ_OPT_ROW_ADDR_DEPENDENCY_NONE)
				PutToNandReqQ(targetReqSlotTag, chNo, wayNo);
			else
				assert(!"[WARNING] Not supported reqOpt [WARNING]");
		}
	}
}


void ReleaseBlockedByRowAddrDepReq(unsigned int chNo, unsigned int wayNo)
{
	unsigned int reqSlotTag, nextReq, rowAddrDepCheckReport;

	reqSlotTag = blockedByRowAddrDepReqQ[chNo][wayNo].headReq;

	while(reqSlotTag != REQ_SLOT_TAG_NONE)
	{
		nextReq = reqPoolPtr->reqPool[reqSlotTag].nextReq;

		if(reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck == REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK)
		{
			rowAddrDepCheckReport = CheckRowAddrDep(reqSlotTag, ROW_ADDR_DEPENDENCY_CHECK_OPT_RELEASE);

			if(rowAddrDepCheckReport == ROW_ADDR_DEPENDENCY_REPORT_PASS)
			{
				SelectiveGetFromBlockedByRowAddrDepReqQ(reqSlotTag, chNo, wayNo);
				PutToNandReqQ(reqSlotTag, chNo, wayNo);
			}
			else if(rowAddrDepCheckReport == ROW_ADDR_DEPENDENCY_REPORT_BLOCKED)
			{
				//pass, go to while loop
			}
			else
				assert(!"[WARNING] Not supported report [WARNING]");
		}
		else
			assert(!"[WARNING] Not supported reqOpt [WARNING]");

		reqSlotTag = nextReq;
	}
}


void IssueNvmeDmaReq(unsigned int reqSlotTag)
{
	//unsigned int devAddr, dmaIndex, numOfNvmeBlock;
    u32 t,p;
    u64 ddr_offset;
	//dmaIndex = reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.startIndex;
	//devAddr = GenerateDataBufAddr(reqSlotTag);
	//numOfNvmeBlock = 0;
	if(reqPoolPtr->reqPool[reqSlotTag].reqCode == REQ_CODE_RxDMA)//  h2c
	{
		if(reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist)
		{
			t = reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.nvmeBlockOffset;
			if(t)
			{
				ddr_offset = 0x1000;
			}
			else
			{
				ddr_offset = 0;
			}
			//first
			H2C_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
			ddr_offset = ddr_offset + (u64)(reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t]);
			t++;
			//middle
	        for(p = 1 ; p < reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock ; p++)//tempNumOfNvmeBlock=2,t=0 p=012 tempNumOfNvmeBlock=1,   p=01
	        {
	        	H2C_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
	        	ddr_offset = ddr_offset + (u64)(reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t]);
	            t++;
	        }
	        //last
	        H2C_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
		}
		else
		{
			t = reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.nvmeBlockOffset;
			if(t)
			{
				ddr_offset = 0x1000;
			}
			else
			{
				ddr_offset = 0;
			}
			for(p = 0 ; p < reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock; p++)//while(numOfNvmeBlock < reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock)
			{
				H2C_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
				ddr_offset = ddr_offset + (u64)(reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t]);
				t++;
			}
		}
	}
	else if(reqPoolPtr->reqPool[reqSlotTag].reqCode == REQ_CODE_TxDMA)//  c2h
	{
		if(reqPoolPtr->reqPool[reqSlotTag].prp_offset_exist)
		{
			t = reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.nvmeBlockOffset;
			if(t)
			{
				ddr_offset = 0x1000;
			}
			else
			{
				ddr_offset = 0;
			}
			//first
			C2H_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
			ddr_offset = ddr_offset + (u64)(reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t]);
			t++;
			//middle
	        for(p = 1 ; p < reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock ; p++)//tempNumOfNvmeBlock=2,t=0 p=012 tempNumOfNvmeBlock=1,   p=01
	        {
	        	C2H_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
	        	ddr_offset = ddr_offset + (u64)(reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t]);
	            t++;
	        }
	        //last
	        C2H_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
		}
		else
		{
			t = reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.nvmeBlockOffset;
			if(t)
			{
				ddr_offset = 0x1000;
			}
			else
			{
				ddr_offset = 0;
			}
			for(p = 0 ; p < reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock; p++)//while(numOfNvmeBlock < reqPoolPtr->reqPool[reqSlotTag].nvmeDmaInfo.numOfNvmeBlock)
			{
				C2H_DMA_PRP2DATA(reqPoolPtr->reqPool[reqSlotTag].prpForEachReq[t], reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry,reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t], ddr_offset);
				ddr_offset = ddr_offset + (u64)(reqPoolPtr->reqPool[reqSlotTag].dataLengthForSlice[t]);
				t++;
			}
		}
	}
	else
		assert(!"[WARNING] Not supported reqCode [WARNING]");
	if((reqPoolPtr->reqPool[reqSlotTag].cqEn) == 1)
	{
		while(nvme_write_io_cq_entry(reqPoolPtr->reqPool[reqSlotTag].cqEntry) == FALSE)
		{
			usleep(100);
		}
	}
}

void CheckDoneNvmeDmaReq()
{
	unsigned int reqSlotTag, prevReq;
	unsigned int rxDone, txDone;

	reqSlotTag = nvmeDmaReqQ.tailReq;
	rxDone = 0;
	txDone = 0;

	while(reqSlotTag != REQ_SLOT_TAG_NONE)
	{
		prevReq = reqPoolPtr->reqPool[reqSlotTag].prevReq;

		if(reqPoolPtr->reqPool[reqSlotTag].reqCode  == REQ_CODE_RxDMA)
		{
				//if((get_io_dma_status() & 0x1) != 0)//write
			rxDone = 1;
			if(rxDone)
				SelectiveGetFromNvmeDmaReqQ(reqSlotTag);
		}
		else
		{
            txDone=1;
			if(txDone)
				SelectiveGetFromNvmeDmaReqQ(reqSlotTag);
		}

		reqSlotTag = prevReq;
	}
}



