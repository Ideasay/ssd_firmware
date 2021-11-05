#include "nvme_io_cmds.h"
#include "../ourNVMe/nvme_structs.h"

//#define debug 0
//changed in 1030
void handle_nvme_io_read(unsigned int cmdSlotTag, nvme_sq_entry_t *sq_entry)
{
	nvme_sq_read_dw12_t* sq_entry_dw12;

	//u32 startLba[2];
	u32 nlb;
    u64 prp[2];

    u32 offset_mask;
    u32 prp_max_num;
    u32 start_offset;
    u32 prp_list_start_offset;
    u32 prp_offset;
    u32 prp_num;
	u64 prp2_trans;
    //u32 start_len;
    //u32 last_len_tmp;
    //u32 last_len;
    u32 data_length;
    u32 left_length;
    u32 first_length;
    u32 tmp_length;
    //u32 prp_mask;
    u32 left_prp_num;
    u32 tmp_prp_num;
    u64 next_prplist_addr;
    u32 tmp_prp_num_for_cycle;
    prp[0] = sq_entry->prp1;
    prp[1] = sq_entry->prp2;
//#if debug
    {
    	//xil_printf("prp[0] is %x!\n\r",prp[0]);
    	//xil_printf("prp[1] is %x!\n\r",prp[1]);
    }
//#endif
	sq_entry_dw12 = (nvme_sq_read_dw12_t*)(&sq_entry->dw[12]);

	//startLba[0] = sq_entry->dw[10];
	//startLba[1] = sq_entry->dw[11];
	nlb = sq_entry_dw12->nlb;
//#if debug
	//xil_printf("nlb is %d\n\r",nlb);
//#endif
	data_length = (nlb+1)*LBA_SIZE;
	offset_mask = (1<<MEM_PAGE_WIDTH)-1;
	prp_max_num = 1<<(MEM_PAGE_WIDTH-3);
	//prp_mask = prp_max_num-1;
	start_offset = (u32)(prp[0]) & offset_mask;
//#if debug
	//xil_printf("start_offset is %x!\n\r",start_offset);
//#endif
	prp_list_start_offset = prp[1] & offset_mask;
	prp_offset = prp_list_start_offset >>3;
	prp_num = (start_offset + data_length + offset_mask) >> MEM_PAGE_WIDTH;
//#if debug
	//xil_printf("prp num is %d\n\r",prp_num);
//#endif
	//start_len = (1<<MEM_PAGE_WIDTH) - start_offset;
	//last_len_tmp = (start_offset + data_length) & offset_mask;
	//if(last_len_tmp==0)
		//last_len = (1<<MEM_PAGE_WIDTH);
	//else
	//	last_len = last_len_tmp;

	if(prp_num==1)
	{
//#if debug
		//xil_printf("only one nvme io read prp transmission is needed!\n\r");
//#endif
        //trans data
		//write dsc
		write_ioD_c2h_dsc(prp[0],PL_IO_READ_BUF_BASEADDR,data_length);
		//check done
		while((get_io_dma_status() & 0x4) == 0);
//#if debug
		//xil_printf("io read(c2h) prp transmission is done!\n\r");
//#endif
	}
	else if(prp_num==2)
	{
//#if debug
		//xil_printf("two nvme io read prp transmissions are needed!\n\r");
//#endif
		first_length = LBA_SIZE - start_offset;
		left_length = data_length - first_length;
		write_ioD_c2h_dsc(prp[0],PL_IO_READ_BUF_BASEADDR,first_length);
		//check done
		while((get_io_dma_status() & 0x4) == 0);
//#if debug
		//xil_printf("first io read(c2h) prp transmission is done!\n\r");
//#endif
		prp2_trans = prp[1] & (0-(1<<12)) ;
		write_ioD_c2h_dsc(prp2_trans,PL_IO_READ_BUF_BASEADDR,left_length);
		//check done
		while((get_io_dma_status() & 0x4) == 0);
//#if debug
		//xil_printf("second io read(c2h) prp transmission is done!\n\r");
//#endif
	}
	else
	{
		//xil_printf("prp list is needed!\n\r");
		left_prp_num = prp_num-1;
//#if debug
		//xil_printf("left_prp_num is %x!\n\r",left_prp_num);
//#endif
		next_prplist_addr = prp[1];
		next_prplist_addr = next_prplist_addr & (0-(1<<2)) ;
		u32 first_time=1;
		for (;left_prp_num>0;)
		{
			if(left_prp_num + prp_offset >prp_max_num)
			{
				tmp_prp_num = prp_max_num - prp_offset;
				left_prp_num = left_prp_num - tmp_prp_num +1;
				tmp_prp_num_for_cycle = tmp_prp_num -1;
//#if debug
				//xil_printf("tmp_prp_num is %x!\n\r",tmp_prp_num);
				//xil_printf("left_prp_num is %x!\n\r",left_prp_num);
				//xil_printf("tmp_prp_num_for_cycle is %x!\n\r",tmp_prp_num_for_cycle);
//#endif
			}
			else
			{
				tmp_prp_num = left_prp_num;
				left_prp_num = 0;
				tmp_prp_num_for_cycle = tmp_prp_num;
//#if debug
				//xil_printf("tmp_prp_num is %x!\n\r",tmp_prp_num);
				//xil_printf("left_prp_num is %x!\n\r",left_prp_num);
				//xil_printf("tmp_prp_num_for_cycle is %x!\n\r",tmp_prp_num_for_cycle);

//#endif
			}
		    write_ioP_h2c_dsc(next_prplist_addr,PL_IO_PRP_BUF_BASEADDR,tmp_prp_num*8);
		    while((get_io_dma_status() & 0x2) == 0);
//#if debug
		    //xil_printf("get the prp list!\n\r");
//#endif
            if(first_time)
            {
//#if debug
        	    //xil_printf("write the prp[0] here!\n\r");
//#endif
        	    first_length = LBA_SIZE - start_offset;
        	    left_length = data_length - first_length;
        	    write_ioD_c2h_dsc(prp[0],PL_IO_READ_BUF_BASEADDR,first_length);
        	    while((get_io_dma_status() & 0x4) == 0);
//#if debug
        	    //xil_printf("prp[0] io read(c2h) prp transmission is done!\n\r");
//#endif
        	    first_time=0;
            }
		    u32 i;
		    u32 rd_data[2];
		    u64 addr_total;
		    for(i=0;i<tmp_prp_num_for_cycle;i++)
		    {
			    if(left_length>LBA_SIZE)
				    tmp_length = LBA_SIZE;
			    else
				    tmp_length = left_length;
			    rd_data[0]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8);
//#if debug
			    //xil_printf("prp list addr_L[%d] is %x!\n\r",i,rd_data[0]);
//#endif
			    rd_data[1]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8+4);
//#if debug
			    //xil_printf("prp list addr_H[%d] is %x!\n\r",i,rd_data[1]);
//#endif
			    addr_total=((u64)(rd_data[0]))+((u64)(rd_data[1])<<32);
				addr_total= addr_total & (0-(1<<12)) ;
			    write_ioD_c2h_dsc(addr_total,PL_IO_READ_BUF_BASEADDR,tmp_length);
			    while((get_io_dma_status() & 0x4) == 0);
			    left_length = left_length - tmp_length;
//#if debug
			    //xil_printf("prp list[i] io read(c2h) prp transmission is done!\n\r",i);
//#endif
		    }
		    if(tmp_prp_num_for_cycle == tmp_prp_num -1)
		    {
			    rd_data[0]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8);
			    rd_data[1]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8+4);
			    next_prplist_addr = ((u64)(rd_data[0]))+((u64)(rd_data[1])<<32);
			    next_prplist_addr = next_prplist_addr & (0-(1<<2)) ;
//#if debug
			    //xil_printf("prp list addr_L[%d](next_prplist_addr) is %x!\n\r",i,next_prplist_addr);
			    //xil_printf("prp list addr_H[%d](next_prplist_addr) is %x!\n\r",i,next_prplist_addr>>32);
//#endif
			    prp_offset = (next_prplist_addr & offset_mask)>>3;
//#if debug
			    //xil_printf("prp_offset is %x!\n\r",prp_offset);
//#endif
		    }
		    //ASSERT(0);

	    }




	}
	//to be completed about the ASSERT/storageCapacity_L
	/*ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
	//ASSERT(nlb < MAX_NUM_OF_NLB);*/
	//prp1_L prp2_L
	//ASSERT((sq_entry->dptr[0] & 0xF) == 0 && (sq_entry->dptr[2] & 0xF) == 0); //error
	//prp1_H prp2_H
	//ASSERT(sq_entry->dptr[1] < 0x10 && sq_entry->dptr[3] < 0x10);
	//ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, NVME_IO_OPCODE_READ);
}

void handle_nvme_io_write(unsigned int cmdSlotTag, nvme_sq_entry_t *sq_entry)
{
	nvme_sq_read_dw12_t* sq_entry_dw12;

	u32 nlb;
    u64 prp[2];
	u64 prp2_trans;
    u32 data_length;
    u32 offset_mask = 0 ;
    u32 prp_max_num = 0 ;
    u32 start_offset =0 ;
    u32 prp_list_start_offset;
    u32 prp_num =0 ;
    u32 left_length;
    u32 first_length;
    u32 tmp_length;
    u64 next_prplist_addr;
	u32 first_time;

    u32 left_prp_num;
    u32 prp_offset;
    u32 tmp_prp_num;
    u32 tmp_prp_num_for_cycle;
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
	prp_max_num = 1<<(MEM_PAGE_WIDTH-3);
	start_offset = (u32)(prp[0]) & offset_mask;
	prp_list_start_offset = (u32)(prp[1]) & offset_mask;

	prp_offset = prp_list_start_offset >>3;
	prp_num = (start_offset + data_length + offset_mask) >> MEM_PAGE_WIDTH;
	
	//this function contains prp decode function
	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_WRITE,prp[0],prp[1], prp_num);
	ReqTransSliceToLowLevel();
	if(prp_num==1)
	{
		//xil_printf("only one nvme io write prp transmission is needed!\n\r");
		//ASSERT(0);
        //trans data
		//write dsc
		

		//dma operation is as follows
		write_ioD_h2c_dsc(prp[0],PL_IO_WRITE_BUF_BASEADDR,data_length);
		//check done
		while((get_io_dma_status() & 0x1) == 0);
//#if debug
		//xil_printf("io write(h2c) prp transmission is done!\n\r");
//#endif
	}
	else if(prp_num==2)
	{
//#if debug
		//xil_printf("two nvme io write prp transmissions are needed!\n\r");
//#endif
		//ASSERT(0);
		first_length = LBA_SIZE - start_offset;
		left_length = data_length - first_length;
		write_ioD_h2c_dsc(prp[0],PL_IO_WRITE_BUF_BASEADDR,first_length);
		//check done
		while((get_io_dma_status() & 0x1) == 0);
//#if debug
		//xil_printf("first io write(h2c) prp transmission is done!\n\r");
//#endif
		prp2_trans = prp[1] & (0-(1<<12)) ;
		write_ioD_h2c_dsc(prp2_trans,PL_IO_WRITE_BUF_BASEADDR,left_length);
		//check done
		while((get_io_dma_status() & 0x1) == 0);
//#if debug
		//xil_printf("second io write(h2c) prp transmission is done!\n\r");
//#endif
	}
	else
	{
//#if debug
		//xil_printf("nvme io write prp list is needed!");
//#endif
		left_prp_num = prp_num-1;
		next_prplist_addr = prp[1];
		next_prplist_addr = next_prplist_addr & (0-(1<<2)) ;
		first_time=1;

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

		    write_ioP_h2c_dsc(next_prplist_addr,PL_IO_PRP_BUF_BASEADDR,tmp_prp_num*8);
		    while((get_io_dma_status() & 0x2) == 0);
		    //xil_printf("get the prp list!\n\r");
            if(first_time)
            {
//#if debug
        	    //xil_printf("write the prp[0] here!\n\r");
//#endif
        	    first_length = LBA_SIZE - start_offset;
        	    left_length = data_length - first_length;
        	    write_ioD_h2c_dsc(prp[0],PL_IO_WRITE_BUF_BASEADDR,first_length);
        	    while((get_io_dma_status() & 0x1) == 0);
//#if debug
        	    //xil_printf("prp[0] io write(h2c) prp transmission is done!\n\r");
//#endif
        	    first_time=0;
            }
		    u32 i;
		    u32 rd_data[2];
		    u64 addr_total;
		    for(i=0;i<tmp_prp_num_for_cycle;i++)
		    {
			    if(left_length>LBA_SIZE)
				    tmp_length = LBA_SIZE;
			    else
				    tmp_length = left_length;
			    rd_data[0]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8);
			    //xil_printf("prp list addr_L[%d] is %x!\n\r",i,rd_data[0]);
			    rd_data[1]=Xil_In32(PL_IO_PRP_BUF_BASEADDR+i*8+4);
			    //xil_printf("prp list addr_H[%d] is %x!\n\r",i,rd_data[1]);
			    addr_total=((u64)(rd_data[0]))+((u64)(rd_data[1])<<32);
				addr_total= addr_total & (0-(1<<12)) ;
			    write_ioD_h2c_dsc(addr_total,PL_IO_WRITE_BUF_BASEADDR,tmp_length);
			    while((get_io_dma_status() & 0x1) == 0);
			    left_length = left_length - tmp_length;
			    //xil_printf("prp list[i] io read(c2h) prp transmission is done!\n\r",i);
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
