#include "generate.h"
#include "nvme/host_lld.h"
void generateReQ(unsigned int count)
{
	NVME_COMMAND *nvmeCmd;
	NVME_IO_COMMAND * nvmeIOCmd_ptr;
    nvmeCmd= (NVME_COMMAND *)(NVME_REQ_SIM_ADDR + (count-1)*sizeof(NVME_COMMAND));
    nvmeIOCmd_ptr = (NVME_IO_COMMAND*)nvmeCmd->cmdDword;
    char * char_addr_ptr;
    unsigned int temp;
    temp=count/2;

    if(count==1)
    {
    	nvmeIOCmd_ptr->OPC=IO_NVM_WRITE;
    	//nvmeIOCmd_ptr->dword[10]=0x00000000 + (count-1);// * BYTES_PER_DATA_REGION_OF_SLICE
    	nvmeIOCmd_ptr->dword[10]=0x00000000 + 0;// * BYTES_PER_DATA_REGION_OF_SLICE

        unsigned int i;
        char_addr_ptr = (char*)(NVME_DATA_SIM_ADDR + (count-1) * BYTES_PER_DATA_REGION_OF_SLICE);
        for(i=0 ; i<BYTES_PER_DATA_REGION_OF_SLICE ; i++)
        {
        	*(char_addr_ptr+i) = (char) count;
        }
    }
    else if(count==2)
    {
    	nvmeIOCmd_ptr->OPC=IO_NVM_WRITE;
    	//nvmeIOCmd_ptr->dword[10]=0x00000000 + (count-1);// * BYTES_PER_DATA_REGION_OF_SLICE
    	nvmeIOCmd_ptr->dword[10]=0x00000000 + 2;// * BYTES_PER_DATA_REGION_OF_SLICE

        unsigned int i;
        char_addr_ptr = (char*)(NVME_DATA_SIM_ADDR + (count-1) * BYTES_PER_DATA_REGION_OF_SLICE);
        for(i=0 ; i<BYTES_PER_DATA_REGION_OF_SLICE ; i++)
        {
        	*(char_addr_ptr+i) = (char) count;
        }
    }
    else if(count==3)
    {
    	nvmeIOCmd_ptr->OPC=IO_NVM_WRITE;
    	//nvmeIOCmd_ptr->dword[10]=0x00000000 + (count-1);// * BYTES_PER_DATA_REGION_OF_SLICE
    	nvmeIOCmd_ptr->dword[10]=0x00000000 + 4;// * BYTES_PER_DATA_REGION_OF_SLICE

        unsigned int i;
        char_addr_ptr = (char*)(NVME_DATA_SIM_ADDR + (count-1) * BYTES_PER_DATA_REGION_OF_SLICE);
        for(i=0 ; i<BYTES_PER_DATA_REGION_OF_SLICE ; i++)
        {
        	*(char_addr_ptr+i) = (char) count;
        }

    }
    else if(count==4)
    {
    	nvmeIOCmd_ptr->OPC=IO_NVM_READ;
    	nvmeIOCmd_ptr->dword[10]=0x00000000 + 0;// * BYTES_PER_DATA_REGION_OF_SLICE
    }


    nvmeIOCmd_ptr->dword[11]=0x00000000;
    //?????????????????????????
    nvmeIOCmd_ptr->dword[12]=0x00010000;

    //unsigned int addr_incr;
    //addr_incr=0;

}

