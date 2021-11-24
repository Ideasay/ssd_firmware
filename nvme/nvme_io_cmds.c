#include "nvme_io_cmds.h"
#include "nvme_structs.h"

//#define debug 0
//changed in 1030
void handle_nvme_io_read(unsigned int cmdSlotTag, nvme_sq_entry_t *sq_entry)
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

	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_READ,prp[0],prp[1], prp_num,start_offset,data_length);

	ReqTransSliceToLowLevel();
}

void handle_nvme_io_write(unsigned int cmdSlotTag, nvme_sq_entry_t *sq_entry)
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
	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_WRITE,prp[0],prp[1], prp_num,start_offset,data_length);

	ReqTransSliceToLowLevel();
}

int process_io_cmd(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry,unsigned short cmdSlotTag)
{
	memset(cq_entry, 0x0, sizeof(nvme_cq_entry_t));
	int need_cqe = 1;
	u16 cid = sq_entry->cid;
	cq_entry->cid = cid;
	switch(sq_entry->opc)
	{
		case IO_NVM_FLUSH:
		{
			//to be completed
			break;
		}
		case IO_NVM_WRITE:
		{
			//xil_printf("IO Write Command\r\n");
			handle_nvme_io_write(cmdSlotTag, sq_entry);

			break;
			}
		case IO_NVM_READ:
		{
			//xil_printf("IO Read Command\r\n");
			handle_nvme_io_read(cmdSlotTag, sq_entry);
			break;
		}
		default:
		{
			//xil_printf("Not Support IO Command OPC: %X\r\n", sq_entry->opc);
			ASSERT(0);
			break;
		}
	}
	return need_cqe;

}
