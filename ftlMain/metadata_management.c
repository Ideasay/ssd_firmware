#include "metadata_management.h"
void InitBuildMeta(u32 base_addr,u32 ddr_addr, u32 chNo)
{
	int i;
	P_CHUNK_DESCRIPTOR p_chunk_desc = (P_CHUNK_DESCRIPTOR)ddr_addr;
	INIT_PRINT("Build metadata here!\n\r");
	INIT_PRINT("1. Erase metadata block here!\n\r");
	eraseblock_60h_d0h(base_addr, 1, 0x00080);
	INIT_PRINT("2. build metadata for each chunk here!\n\r");
    for(i=0 ; i< CHUNK_NUM_PER_PU;i++)
    {
    	//chunk state
    	p_chunk_desc->CS = 0;
    	p_chunk_desc->chunkFree = 1;
    	//chunk type
    	p_chunk_desc->CT = 0;
    	p_chunk_desc->wrSeq = 1;
        //wear-level index
    	p_chunk_desc->WLI = 0;
    	//reserved
    	p_chunk_desc->reserved3[0] = 0;
    	p_chunk_desc->reserved3[1] = 0;
    	p_chunk_desc->reserved3[2] = 0;
    	p_chunk_desc->reserved3[3] = 0;
    	p_chunk_desc->reserved3[4] = 0;
    	//starting LBA
    	p_chunk_desc->SLBA = chNo*CHUNK_NUM_PER_PU*128 + i*128;
    	//number of blocks in chunk
    	p_chunk_desc->CNLB = 0;
    	//Write Pointer
    	p_chunk_desc->WP = 0;

    	p_chunk_desc = p_chunk_desc + 32;
    }
    Xil_Out32(ddr_addr + 0x1FFC, 0x1FFC);
}

void InitMetaData()
{

	u32 way=1;
	int word;
	int i;
	INIT_PRINT("Init Metadata here \n\r");
	u32 * wptr;
	wptr = (u32*)PREDEFINED_DATA_ADDR;
	for(i=0 ; i<2048; i++)
	{
		*(wptr) = 0xc0ffee;
		wptr = wptr + i + 1;
	}
	INIT_PRINT("predefined data done! \n\r");
	if(NSC_0_CONNECTED)
	{
		readpage_00h_30h(NSC_0_BASEADDR,  way , 0x00000000, 0x00080, BYTES_PER_DATA_REGION_OF_PAGE, CH0_META_DATA_ADDR);
		word=Xil_In32(CH0_META_DATA_ADDR + 0x1FFC);
		if(word == 0x1FFC)
		{
			INIT_PRINT("CH0 meta data exists\n\r");
		}
		else
		{
			INIT_PRINT("CH0 meta data doesn't exist and we build it here\n\r");
			InitBuildMeta(NSC_0_BASEADDR,CH0_META_DATA_ADDR,0);
		}
	}
	if(NSC_1_CONNECTED)
	{
		readpage_00h_30h(NSC_1_BASEADDR,  way , 0x00000000, 0x00080, BYTES_PER_DATA_REGION_OF_PAGE, CH1_META_DATA_ADDR);
		word=Xil_In32(CH1_META_DATA_ADDR + 0x1FFC);
		if(word == 0x1FFC)
		{
			INIT_PRINT("CH1 meta data exists\n\r");
		}
		else
		{
			INIT_PRINT("CH1 meta data doesn't exist and we build it here\n\r");
			InitBuildMeta(NSC_1_BASEADDR,CH1_META_DATA_ADDR,1);
		}
	}
}
void IntigrateMetaData()
{
	META_PRINT("Integrate Metadata here \n\r");
	u32 pointer = TOTAL_META_DATA_ADDR;
	memcpy((void*)pointer,(void*)CH0_META_DATA_ADDR,CHUNK_NUM_PER_PU*sizeof(CHUNK_DESCRIPTOR));
	pointer = pointer + CHUNK_NUM_PER_PU*sizeof(CHUNK_DESCRIPTOR);
	memcpy((void*)pointer,(void*)CH1_META_DATA_ADDR,CHUNK_NUM_PER_PU*sizeof(CHUNK_DESCRIPTOR));
}

int BackupMetaData()
{
	META_PRINT("Backup Metadata here \n\r");
	if(NSC_0_CONNECTED)
	{
		META_PRINT("Backup CH0 Metadata here \n\r");
		progpage_80h_10h(NSC_0_BASEADDR, 1, 0, 0x00080, BYTES_PER_DATA_REGION_OF_PAGE, CH0_META_DATA_ADDR);

		//readpage_00h_30h(NSC_0_BASEADDR,  way , 0x00000000, 0x00080, BYTES_PER_DATA_REGION_OF_PAGE, 0x90000000);
	}
	if(NSC_1_CONNECTED)
	{
		META_PRINT("Backup CH1 Metadata here \n\r");
		progpage_80h_10h(NSC_1_BASEADDR, 1, 0, 0x00080, BYTES_PER_DATA_REGION_OF_PAGE, CH1_META_DATA_ADDR);
	}
	return 1;
}
void MaintainMetaData(u32 lba, u32 opCode)
{
	P_CHUNK_DESCRIPTOR p_chunk_desc     ;
	OC_PHYSICAL_ADDRESS physical_address;
	u32	logical_block_addr	            ;
	u32	chunk_addr		                ;
	//u32	pu_addr	     	            ;
	u32	group_addr	                    ;
    u32 metaDataBaseAddr                ;
	physical_address=(OC_PHYSICAL_ADDRESS)lba;
	logical_block_addr = physical_address.logical_block_addr;
	chunk_addr = physical_address.chunk_addr;
	//u32	pu_addr	     	;
	group_addr = physical_address.group_addr;
	META_PRINT("logical_block_addr is 0x%x \n\r",logical_block_addr);
	META_PRINT("chunk_addr is 0x%x \n\r",chunk_addr);
	META_PRINT("group_addr is 0x%x \n\r",group_addr);
	if(group_addr == 1)
	{
		metaDataBaseAddr = CH1_META_DATA_ADDR;
	}
	else
	{
		metaDataBaseAddr = CH0_META_DATA_ADDR;
	}
	metaDataBaseAddr = metaDataBaseAddr + chunk_addr*32;
	META_PRINT("metaDataBaseAddr is 0x%x \n\r",metaDataBaseAddr);
	p_chunk_desc = (P_CHUNK_DESCRIPTOR)metaDataBaseAddr;
    if(opCode == IO_NVM_READ)
    {
    	return;
    }
    else if(opCode == IO_NVM_WRITE)
    {
    	//Write Pointer
    	p_chunk_desc->WP = p_chunk_desc->WP + 1;
    	if(p_chunk_desc->CS == 1)
    	{
        	p_chunk_desc->CS = 4;//open
    	}
    	else if((p_chunk_desc->CS == 4)&&(p_chunk_desc->WP == 128))
    	{
    		p_chunk_desc->CS = 2;//close
    	}
    }
    else if(opCode == IO_NVM_MANAGEMENT)
    {
        //wear-level index
    	p_chunk_desc->WLI = p_chunk_desc->WLI + 1;
    	if(p_chunk_desc->CS == 1)
    	{
        	p_chunk_desc->CS = 1;
    	}
    	else if((p_chunk_desc->CS == 2)&&(p_chunk_desc->WLI==255))
    	{
    		p_chunk_desc->CS = 8;
    	}
    	else if((p_chunk_desc->CS == 2))
    	{
    		p_chunk_desc->CS = 0;
    		p_chunk_desc->WP = 0;
    	}
    }
    else
    {
    	ASSERT(0);
    }
    return;
}
int CheckMetaData(u32 lba, u32 opCode, nvme_cq_entry_t *cq_entry)
{
	P_CHUNK_DESCRIPTOR p_chunk_desc     ;
	OC_PHYSICAL_ADDRESS physical_address;
	u32	logical_block_addr	            ;
	u32	chunk_addr		                ;
	//u32	pu_addr	     	            ;
	u32	group_addr	                    ;
    u32 metaDataBaseAddr                ;
    u32 state                           ;
    u32 wp                              ;

	physical_address=(OC_PHYSICAL_ADDRESS)lba;
	logical_block_addr = physical_address.logical_block_addr;
	chunk_addr = physical_address.chunk_addr;
	//u32	pu_addr	     	;
	group_addr = physical_address.group_addr;
	META_PRINT("logical_block_addr is 0x%x \n\r",logical_block_addr);
	META_PRINT("chunk_addr is 0x%x \n\r",chunk_addr);
	META_PRINT("group_addr is 0x%x \n\r",group_addr);
	if(group_addr == 1)
	{
		metaDataBaseAddr = CH1_META_DATA_ADDR;
	}
	else
	{
		metaDataBaseAddr = CH0_META_DATA_ADDR;
	}
	metaDataBaseAddr = metaDataBaseAddr + chunk_addr*32;
	META_PRINT("metaDataBaseAddr is 0x%x \n\r",metaDataBaseAddr);
	p_chunk_desc = (P_CHUNK_DESCRIPTOR)metaDataBaseAddr;
	state = p_chunk_desc->CS;
	wp = p_chunk_desc->WP;

	//if(state == 1) //free
	//else if(state == 2)//closed
	//else if(state == 4)//open
	//else if(state == 8)//offline

	if(opCode == IO_NVM_READ)
	{
		if(((state == 4)||(state == 2))&&(logical_block_addr < wp - MW_CUNITS_DATA))
		{
			META_PRINT("read oc check return 0\n\r");
			cq_entry->status = 0;
			return 0;
		}
		else
		{
			META_PRINT("read oc check return 1 (predefined data)\n\r");
			cq_entry->status = 0;
			return 1;
		}
	}
	else if(opCode == IO_NVM_WRITE)
	{
           if(((logical_block_addr > wp)||(logical_block_addr < wp))&&(state == 4))
           {
        	   cq_entry->status = 0xF2;
        	   return 2;                          //forbidden write
           }
           else if((state == 2)||(state == 8))
           {
        	   cq_entry->status = 0x80;
        	   return 2;                          //forbidden write
           }
           else if(physical_address.reserved0!=0 )
           {
        	   cq_entry->status = 0x80;
        	   return 2;                          //forbidden write
           }
	}
	else if(opCode == IO_NVM_MANAGEMENT)
	{
		if((MULTI_RESET_EN == 0)&&(state == 1))
		{
     	   cq_entry->status = 0xC1;
     	   return 2;                          //invalid reset
		}
		else if((state == 8))//(MULTI_RESET_EN == 1)&&
		{
     	   cq_entry->status = 0xC0;
     	   return 2;                          //offline no longer reset
		}

		else if(state == 4)
		{
			cq_entry->status = 0xC1;
			return 2;
		}
        else if(physical_address.reserved0!=0 )
        {
     	   cq_entry->status = 0xC1;
     	   return 2;                          //invalid reset
        }
	}
	return 10;
}
