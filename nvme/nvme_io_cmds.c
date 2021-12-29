#include "nvme_io_cmds.h"
#include "nvme_structs.h"

//#define debug 0
//changed in 1030
void handle_nvme_io_read(nvme_sq_entry_t *sq_entry, nvme_cq_entry_t *cq_entry)
{
	nvme_sq_read_dw12_t* sq_entry_dw12;
	u32 nlb;
    u64 prp[2];
    u32 offset_mask;
    u32 start_offset;
    u32 prp_num;
    u32 data_length;

    prp[0] = sq_entry->prp1;
    prp[1] = sq_entry->prp2;
	sq_entry_dw12 = (nvme_sq_read_dw12_t*)(&sq_entry->dw[12]);

	//used for tansform2slice
	u32 startLba[2];
	startLba[0] = sq_entry->dw[10];
	startLba[1] = sq_entry->dw[11];
	nlb = sq_entry_dw12->nlb;
	data_length = (nlb+1)*LBA_SIZE;
	offset_mask = (1<<MEM_PAGE_WIDTH)-1;
	start_offset = (u32)(prp[0]) & offset_mask;
	prp_num = (start_offset + data_length + offset_mask) >> MEM_PAGE_WIDTH;

	ReqTransNvmeToSlice(startLba[0], nlb, IO_NVM_READ,prp[0],prp[1], prp_num,start_offset,data_length,cq_entry);

}

void handle_nvme_io_write(nvme_sq_entry_t *sq_entry, nvme_cq_entry_t *cq_entry)
{
	nvme_sq_read_dw12_t* sq_entry_dw12;

	u32 nlb;
    u64 prp[2];
    u32 offset_mask = 0 ;
    u32 start_offset =0 ;
    u32 prp_num =0 ;
	u32 data_length;

    prp[0] = sq_entry->prp1;
    prp[1] = sq_entry->prp2;

	sq_entry_dw12 = (nvme_sq_read_dw12_t*)(&sq_entry->dw[12]);

	//used for tansform2slice
	u32 startLba[2];
	startLba[0] = sq_entry->dw[10];
	startLba[1] = sq_entry->dw[11];

	nlb = sq_entry_dw12->nlb;
	data_length = (nlb+1)*LBA_SIZE;
	offset_mask = (1<<MEM_PAGE_WIDTH)-1;
	start_offset = (u32)(prp[0]) & offset_mask;
	prp_num = (start_offset + data_length + offset_mask) >> MEM_PAGE_WIDTH;
	
	//this function contains prp decode function
	ReqTransNvmeToSlice(startLba[0], nlb, IO_NVM_WRITE,prp[0],prp[1], prp_num,start_offset,data_length,cq_entry);

}
void handle_nvme_io_reset(nvme_sq_entry_t *sq_entry, nvme_cq_entry_t *cq_entry)
{
	u32 startLba[2];
	//u32 startLba1;
	OC_PHYSICAL_ADDRESS physical_address;
	//u32	logical_block_addr	;
	u32	chunk_addr		    ;
	//u32	pu_addr	     	;
	u32	group_addr	        ;
    u32 baseAddress         ;
    u32 rowAddress          ;
    u32 reset_valid         ;
	startLba[0] = sq_entry->dw[10];
	startLba[1] = sq_entry->dw[11];

	physical_address = (OC_PHYSICAL_ADDRESS)startLba[0];
	//logical_block_addr = physical_address.logical_block_addr;
	chunk_addr = physical_address.chunk_addr;
	//u32	pu_addr	     	;
	group_addr = physical_address.group_addr;
	if(group_addr == 1)
	{
		baseAddress = NSC_1_BASEADDR;
	}
	else
	{
		baseAddress = NSC_0_BASEADDR;
	}
	reset_valid = CheckMetaData(startLba[0], IO_NVM_MANAGEMENT, cq_entry);
	if(reset_valid == 2)
		goto invalid_reset;
	rowAddress = chunk_addr<<7;
	//while((readstatus_70h(way) & (ARDY | RDY)) == 0x00);
	eraseblock_60h_d0h(baseAddress, 1, rowAddress);
	MaintainMetaData(startLba[0], IO_NVM_MANAGEMENT);
	return;
invalid_reset:
    RESET_PRINT("invalid reset!\n\r");
    return;
}
int process_io_cmd(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{

	int need_cqe = 1;
	u16 cid = sq_entry->cid;
	nvme_sq_dataset_management_dw11_t dw11;
	memset(cq_entry, 0x0, sizeof(nvme_cq_entry_t));
	cq_entry->cid = cid;
	cq_entry->status = 0;
	dw11 = (nvme_sq_dataset_management_dw11_t)(sq_entry->cdw11);
	switch(sq_entry->opc)
	{
		case IO_NVM_FLUSH:
		{
			//to be completed
			break;
		}
		case IO_NVM_WRITE://OC_NVM_PPA_OPCODE_WRITE
		{
			IO_PRINT("IO Write Command\r\n");
			handle_nvme_io_write(sq_entry, cq_entry);

			break;
			}
		case IO_NVM_READ://OC_NVM_PPA_OPCODE_READ
		{
			IO_PRINT("IO Read Command\r\n");
			handle_nvme_io_read(sq_entry, cq_entry);
			break;
		}
		case IO_NVM_MANAGEMENT://OC_NVM_PPA_OPCODE_DATASET_MANAGEMENT
		{
			if(dw11.ad)
			{
				IO_PRINT(" OC Reset Chunk\r\n");
				handle_nvme_io_reset( sq_entry, cq_entry);
			}

			break;
		}
		default:
		{
			IO_PRINT("Not Support IO Command OPC: %X\r\n", sq_entry->opc);
			ASSERT(0);
			break;
		}
	}
	return need_cqe;

}

void ReqTransNvmeToSlice(u32 startLba, unsigned int nlb, unsigned int cmdCode, u64 prp1ForReq, u64 prp2ForReq, int prpNum, int start_offset, int data_length, nvme_cq_entry_t *cq_entry)
{
	unsigned int  requestedNvmeBlock, tempNumOfNvmeBlock, transCounter, loop, nvmeBlockOffset;
	u32 tempLsa;
	//new added for transforming req
	u64 prpCollectedForSlice[prpNum];
	int dataLengthForSlice[prpNum];
	//int tempLastSliceOfNvme;
	int left_length;
	OC_PHYSICAL_ADDRESS physical_address;
	u32 i;
	u32 prp_array_transfer_count;
	u32 tmp_prp_num;
	u32 tmp_prp_num_for_cycle;

	u32	logical_block_addr	;
	u32	chunk_addr		    ;
	//u32	pu_addr	     	;
	u32	group_addr	        ;
    u32 baseAddress         ;
    u32 rowAddress          ;
    u32 temp_offset         ;
    u32 predefined_data_valid=0;
	//init the first prp
	int prpCnt = 0;
	u32 offset_mask = (1<<MEM_PAGE_WIDTH)-1;

	requestedNvmeBlock = nlb + 1;

	transCounter = 0;

	tempLsa = startLba / NVME_BLOCKS_PER_SLICE;
	loop = ((startLba % NVME_BLOCKS_PER_SLICE) + requestedNvmeBlock) / NVME_BLOCKS_PER_SLICE;
	SLICE_PRINT("ReqTransNvmeToSlice\n\r");
	SLICE_PRINT("ReqTransNvmeToSlice here!\n\r");
	SLICE_PRINT("startLba is 0x%x\n\r",startLba);
	SLICE_PRINT("start_offset is 0x%x\n\r",start_offset);
	SLICE_PRINT("prpNum is 0x%x\n\r",prpNum);
	SLICE_PRINT("requestedNvmeBlock is 0x%x\n\r",requestedNvmeBlock);
	SLICE_PRINT("loop is 0x%x\n\r",loop);

	if(prpNum == 1)
	{
		prpCnt = 1;
		prpCollectedForSlice[0] = prp1ForReq;
		dataLengthForSlice[0] = LBA_SIZE - start_offset;
		SLICE_PRINT("prpNum == 1\n\r");
		SLICE_PRINT("prpCollectedForSlice[0](L) is 0x%x!\n\r",prpCollectedForSlice[0]);
		SLICE_PRINT("prpCollectedForSlice[0](H) is 0x%x!\n\r",prpCollectedForSlice[0]>>32);
		SLICE_PRINT("dataLengthForSlice[0] is 0x%x!\n\r",dataLengthForSlice[0]);
	}
	else if(prpNum == 2)
	{
		prpCnt = 2;
		prpCollectedForSlice[0] = prp1ForReq;
		prpCollectedForSlice[1] = prp2ForReq & (0-(1<<12));
		dataLengthForSlice[0] = LBA_SIZE - start_offset;
		dataLengthForSlice[1] = data_length - dataLengthForSlice[0];
		SLICE_PRINT("prpNum == 2\n\r");
		SLICE_PRINT("prpCollectedForSlice[0](L) is 0x%x!\n\r",prpCollectedForSlice[0]);
		SLICE_PRINT("prpCollectedForSlice[0](H) is 0x%x!\n\r",prpCollectedForSlice[0]>>32);
		SLICE_PRINT("prpCollectedForSlice[1](L) is 0x%x!\n\r",prpCollectedForSlice[1]);
		SLICE_PRINT("prpCollectedForSlice[1](H) is 0x%x!\n\r",prpCollectedForSlice[1]>>32);
		SLICE_PRINT("dataLengthForSlice[0] is 0x%x!\n\r",dataLengthForSlice[0]);
		SLICE_PRINT("dataLengthForSlice[1] is 0x%x!\n\r",dataLengthForSlice[1]);
	}
	else
	{
		//add prp[0]
		prpCollectedForSlice[0] = prp1ForReq;
		SLICE_PRINT("prpNum >=3\n\r");

		prpCnt = 1;
		dataLengthForSlice[0] = LBA_SIZE - start_offset;
		left_length = data_length - dataLengthForSlice[0];
		SLICE_PRINT("prpCollectedForSlice[0](L) is 0x%x!\n\r",prpCollectedForSlice[0]);
		SLICE_PRINT("prpCollectedForSlice[0](H) is 0x%x!\n\r",prpCollectedForSlice[0]>>32);
		SLICE_PRINT("dataLengthForSlice[0] is 0x%x!\n\r",dataLengthForSlice[0]);
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
			    SLICE_PRINT("prpCollectedForSlice[%d](L) is 0x%llx!\n\r",prpCnt,prpCollectedForSlice[prpCnt]);
			    SLICE_PRINT("prpCollectedForSlice[%d](H) is 0x%llx!\n\r",prpCnt,prpCollectedForSlice[prpCnt]>>32);
				if(left_length>LBA_SIZE)
				{
					dataLengthForSlice[prpCnt] = LBA_SIZE;
				}
			    else
			    {
			    	dataLengthForSlice[prpCnt] = left_length;
			    }
				SLICE_PRINT("dataLengthForSlice[%d] is 0x%x!\n\r",prpCnt,dataLengthForSlice[prpCnt]);
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

	SLICE_PRINT("nvmeBlockOffset is 0x%x\n\r",nvmeBlockOffset);
	SLICE_PRINT("numOfNvmeBlock is 0x%x\n\r",tempNumOfNvmeBlock);

	physical_address=(OC_PHYSICAL_ADDRESS)startLba;
	if(start_offset)
	{
		SLICE_PRINT("start offset exist!!!!\n\r");
		if(cmdCode == IO_NVM_READ)
		{
			logical_block_addr = physical_address.logical_block_addr;
			chunk_addr = physical_address.chunk_addr;
			//u32	pu_addr	     	;
			group_addr = physical_address.group_addr;
			if(group_addr == 1)
			{
				baseAddress = NSC_1_BASEADDR;
			}
			else
			{
				baseAddress = NSC_0_BASEADDR;
			}
			rowAddress = (chunk_addr<<7) + logical_block_addr;
			SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
			SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);
			predefined_data_valid = CheckMetaData(startLba, cmdCode, cq_entry);
			if(predefined_data_valid == 0)
			{
				readpage_00h_30h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress, BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_READ_BUF_BASEADDR);
				//MaintainMetaData(startLba, cmdCode);
			}
			//trigger DMA here!
			temp_offset = dataLengthForSlice[prp_array_transfer_count];
			if(predefined_data_valid == 0)
			{
				write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_READ_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			}
			else if(predefined_data_valid == 1)
			{
				write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PREDEFINED_DATA_ADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			}
			while((get_io_dma_status() & 0x4) == 0);
			prp_array_transfer_count++;
			if(predefined_data_valid == 0)
			{
				write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_READ_BUF_BASEADDR + temp_offset,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			}
			else if(predefined_data_valid == 1)
			{
				write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PREDEFINED_DATA_ADDR,dataLengthForSlice[prp_array_transfer_count]);
			}
			while((get_io_dma_status() & 0x4) == 0);
			prp_array_transfer_count++;
			SLICE_PRINT("!!! read data from DDR data buffer!!! \r\n");
		}

		if(cmdCode == IO_NVM_WRITE)
		{
			//add by zheng here
			//trigger DMA here!
			predefined_data_valid = CheckMetaData(startLba, cmdCode, cq_entry);
			if(predefined_data_valid==2)
				goto forbidden_write;
			temp_offset = dataLengthForSlice[prp_array_transfer_count];
			write_ioD_h2c_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_WRITE_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			while((get_io_dma_status() & 0x1) == 0);
			prp_array_transfer_count++;
			write_ioD_h2c_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_WRITE_BUF_BASEADDR + temp_offset,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			while((get_io_dma_status() & 0x1) == 0);
			prp_array_transfer_count++;
			SLICE_PRINT("!!! write data to DDR data buffer!!! \r\n");
			logical_block_addr = physical_address.logical_block_addr;
			chunk_addr = physical_address.chunk_addr;
			//u32	pu_addr	     	;
			group_addr = physical_address.group_addr;
			if(group_addr == 1)
			{
				baseAddress = NSC_1_BASEADDR;
			}
			else
			{
				baseAddress = NSC_0_BASEADDR;
			}
			rowAddress = (chunk_addr<<7) + logical_block_addr;
			SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
			SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);
			//progpage_80h_10h(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMARAddress)
			progpage_80h_10h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress,BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_WRITE_BUF_BASEADDR);
			MaintainMetaData(startLba, cmdCode);

		}
	}
	else//no start offset
	{
		SLICE_PRINT("no start offset!!!!\n\r");

		//add by zheng begin
		if(cmdCode == IO_NVM_READ)
		{
			logical_block_addr = physical_address.logical_block_addr;
			chunk_addr = physical_address.chunk_addr;
			//u32	pu_addr	     	;
			group_addr = physical_address.group_addr;
			if(group_addr == 1)
			{
				baseAddress = NSC_1_BASEADDR;
			}
			else
			{
				baseAddress = NSC_0_BASEADDR;
			}
			rowAddress = (chunk_addr<<7) + logical_block_addr;
			SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
			SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);

			predefined_data_valid = CheckMetaData(startLba, cmdCode, cq_entry);
			if(predefined_data_valid == 0)
			{
				readpage_00h_30h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress, BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_READ_BUF_BASEADDR);
				//MaintainMetaData(startLba, cmdCode);
				write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_READ_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			}
			else if(predefined_data_valid == 1)
			{
				write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PREDEFINED_DATA_ADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			}
			//trigger DMA here! once DMA is enough because there is no offset

			while((get_io_dma_status() & 0x4) == 0);
			prp_array_transfer_count++;
			SLICE_PRINT("!!! read data from DDR data buffer!!! \r\n");
		}

		if(cmdCode == IO_NVM_WRITE)
		{
			//trigger DMA here! once DMA is enough because there is no offset
			predefined_data_valid = CheckMetaData(startLba, cmdCode, cq_entry);
			if(predefined_data_valid==2)
				goto forbidden_write;
			write_ioD_h2c_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_WRITE_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
			while((get_io_dma_status() & 0x1) == 0);
			prp_array_transfer_count++;
			SLICE_PRINT("!!! write data to DDR data buffer!!! \r\n");

			logical_block_addr = physical_address.logical_block_addr;
			chunk_addr = physical_address.chunk_addr;
			//u32	pu_addr	     	;
			group_addr = physical_address.group_addr;
			if(group_addr == 1)
			{
				baseAddress = NSC_1_BASEADDR;
			}
			else
			{
				baseAddress = NSC_0_BASEADDR;
			}
			rowAddress = (chunk_addr<<7) + logical_block_addr;
			SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
			SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);
			//progpage_80h_10h(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMARAddress)
			progpage_80h_10h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress,BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_WRITE_BUF_BASEADDR);
			MaintainMetaData(startLba, cmdCode);
		}
		//add by zheng end

	}

	tempLsa++;
	transCounter++;

	//transform continue
	while(transCounter < loop)
	{
		physical_address=(OC_PHYSICAL_ADDRESS)tempLsa;
		if(start_offset)
		{
			SLICE_PRINT("start offset exist!!!!\n\r");
			if(cmdCode == IO_NVM_READ)
			{
				logical_block_addr = physical_address.logical_block_addr;
				chunk_addr = physical_address.chunk_addr;
				//u32	pu_addr	     	;
				group_addr = physical_address.group_addr;
				if(group_addr == 1)
				{
					baseAddress = NSC_1_BASEADDR;
				}
				else
				{
					baseAddress = NSC_0_BASEADDR;
				}
				rowAddress = (chunk_addr<<7) + logical_block_addr;
				SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
				SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);
				predefined_data_valid = CheckMetaData(tempLsa, cmdCode, cq_entry);
				if(predefined_data_valid == 0)
				{
					readpage_00h_30h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress, BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_READ_BUF_BASEADDR);
					//MaintainMetaData(startLba, cmdCode);
				}
				//trigger DMA here!
				temp_offset = dataLengthForSlice[prp_array_transfer_count];
				if(predefined_data_valid == 0)
				{
					write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_READ_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				}
				else if(predefined_data_valid == 1)
				{
					write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PREDEFINED_DATA_ADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				}
				while((get_io_dma_status() & 0x4) == 0);
				prp_array_transfer_count++;
				if(predefined_data_valid == 0)
				{
					write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_READ_BUF_BASEADDR + temp_offset,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				}
				else if(predefined_data_valid == 1)
				{
					write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PREDEFINED_DATA_ADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				}
				while((get_io_dma_status() & 0x4) == 0);
				prp_array_transfer_count++;
				SLICE_PRINT("!!! read data from DDR data buffer!!! \r\n");
			}

			if(cmdCode == IO_NVM_WRITE)
			{
				//add by zheng here
				//trigger DMA here!
				predefined_data_valid = CheckMetaData(tempLsa, cmdCode, cq_entry);
				if(predefined_data_valid==2)
					goto forbidden_write;
				temp_offset = dataLengthForSlice[prp_array_transfer_count];
				write_ioD_h2c_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_WRITE_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				while((get_io_dma_status() & 0x1) == 0);
				prp_array_transfer_count++;
				write_ioD_h2c_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_WRITE_BUF_BASEADDR + temp_offset,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				while((get_io_dma_status() & 0x1) == 0);
				prp_array_transfer_count++;
				SLICE_PRINT("!!! write data to DDR data buffer!!! \r\n");
				logical_block_addr = physical_address.logical_block_addr;
				chunk_addr = physical_address.chunk_addr;
				//u32	pu_addr	     	;
				group_addr = physical_address.group_addr;
				if(group_addr == 1)
				{
					baseAddress = NSC_1_BASEADDR;
				}
				else
				{
					baseAddress = NSC_0_BASEADDR;
				}
				rowAddress = (chunk_addr<<7) + logical_block_addr;
				SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
				SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);
				//progpage_80h_10h(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMARAddress)
				progpage_80h_10h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress,BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_WRITE_BUF_BASEADDR);
				MaintainMetaData(startLba, cmdCode);

			}
		}
		else//no start offset
		{
			SLICE_PRINT("no start offset!!!!\n\r");

			//add by zheng begin
			if(cmdCode == IO_NVM_READ)
			{
				logical_block_addr = physical_address.logical_block_addr;
				chunk_addr = physical_address.chunk_addr;
				//u32	pu_addr	     	;
				group_addr = physical_address.group_addr;
				if(group_addr == 1)
				{
					baseAddress = NSC_1_BASEADDR;
				}
				else
				{
					baseAddress = NSC_0_BASEADDR;
				}
				rowAddress = (chunk_addr<<7) + logical_block_addr;
				SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
				SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);
				predefined_data_valid = CheckMetaData(tempLsa, cmdCode, cq_entry);
				if(predefined_data_valid == 0)
				{
					readpage_00h_30h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress, BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_READ_BUF_BASEADDR);
				//MaintainMetaData(startLba, cmdCode);
				//trigger DMA here! once DMA is enough because there is no offset
					write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_READ_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				}
				else if(predefined_data_valid == 1)
				{
					write_ioD_c2h_dsc(prpCollectedForSlice[prp_array_transfer_count],PREDEFINED_DATA_ADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				}
				while((get_io_dma_status() & 0x4) == 0);
				prp_array_transfer_count++;
				SLICE_PRINT("!!! read data from DDR data buffer!!! \r\n");
			}

			if(cmdCode == IO_NVM_WRITE)
			{
				//trigger DMA here! once DMA is enough because there is no offset
				predefined_data_valid = CheckMetaData(tempLsa, cmdCode, cq_entry);
				if(predefined_data_valid==2)
					goto forbidden_write;
				write_ioD_h2c_dsc(prpCollectedForSlice[prp_array_transfer_count],PL_IO_WRITE_BUF_BASEADDR,dataLengthForSlice[prp_array_transfer_count]);// unit is byte!
				while((get_io_dma_status() & 0x1) == 0);
				prp_array_transfer_count++;
				SLICE_PRINT("!!! write data to DDR data buffer!!! \r\n");

				logical_block_addr = physical_address.logical_block_addr;
				chunk_addr = physical_address.chunk_addr;
				//u32	pu_addr	     	;
				group_addr = physical_address.group_addr;
				if(group_addr == 1)
				{
					baseAddress = NSC_1_BASEADDR;
				}
				else
				{
					baseAddress = NSC_0_BASEADDR;
				}
				rowAddress = (chunk_addr<<7) + logical_block_addr;
				SLICE_PRINT("baseAddress is 0x%x!\n\r",baseAddress);
				SLICE_PRINT("rowAddress is 0x%x!\n\r",rowAddress);
				//progpage_80h_10h(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMARAddress)
				progpage_80h_10h(baseAddress, 1/*way*/ , 0/*col*/, rowAddress,BYTES_PER_DATA_REGION_OF_PAGE, PL_IO_WRITE_BUF_BASEADDR);
				MaintainMetaData(startLba, cmdCode);
			}
			//add by zheng end

		}

		tempLsa++;
		transCounter++;

	}


	//last transform
	nvmeBlockOffset = 0;
	tempNumOfNvmeBlock = (startLba + requestedNvmeBlock) % NVME_BLOCKS_PER_SLICE;
	if((tempNumOfNvmeBlock == 0) || (loop == 0))//1
	{
		SLICE_PRINT("ReqTransNvmeToSlice end!\n\r");
		return ;
	}

forbidden_write:
    SLICE_PRINT("forbidden_write!\n\r");
    //assert(0);
    return;

	//Delete operation tail here!
}
