/**
* NVMeCHA: NVMe Controller featuring Hardware Acceleration
* Copyright (C) 2021 State Key Laboratory of ASIC and System, Fudan University
* Contributed by Yunhui Qiu
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nvme_admin_cmds.h"

static u64 nc=1;

// SQ/CQ reset due to queue delete
u32 g_sq_reset = 0;
u32 g_cq_reset = 0;
u32 g_iocq_irq_enable = 0;

void create_io_sq(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	nvme_sq_create_io_sq_dw10_t* sq_entry_dw10 = (nvme_sq_create_io_sq_dw10_t*)(&sq_entry->dw[10]);
	nvme_sq_create_io_sq_dw11_t* sq_entry_dw11 = (nvme_sq_create_io_sq_dw11_t*)(&sq_entry->dw[11]);

	u16 cid = sq_entry->cid;
	u16 sq_id = sq_entry_dw10->qid;
	u16 qsize = sq_entry_dw10->qsize; // 0 based
	u16 cq_id = sq_entry_dw11->cqid;
	u16	pc = sq_entry_dw11->pc;
	u8 psdt = sq_entry->psdt;
	u64 qbase_addr = sq_entry->prp1;

	AD_PRINT("Admin Command(Create IO SQ), cid: %d, nsid: %d, sq_id: %d, cq_id: %d, qsize: %d\n\r", cid, sq_entry->nsid, sq_id, cq_id, qsize);

	u32 sq_index;
	sq_attribute_t sq_attribute;
	sq_attribute.qid = sq_id;
	sq_attribute.qsize = qsize;
	sq_attribute.cqid = cq_id;
	sq_attribute.qbase_addr = qbase_addr;
	//memset(cq_entry, 0, NVME_CQ_ENTRY_SIZE);//1021
	cq_entry->cid = cid;
	//cq_entry->sqid = sq_id; //1021
	if((psdt != 0) || (pc != 1)){ // only support physically contiguous region
		cq_entry->sct = 0;
		cq_entry->sc = 2;
	} else if(cq_exist(cq_id) == FALSE){ // paired CQ not created yet
		cq_entry->sct = 1;
		cq_entry->sc = 0;
	} else if((qsize == 0) || (qsize > NVME_QUE_MAX_SIZE)){
		cq_entry->sct = 1;
		cq_entry->sc = 2;
	} else if(set_sq_attribute(cq_id, &sq_attribute, &sq_index)  == FALSE){ // SQ created yet
		cq_entry->sct = 1;
		cq_entry->sc = 1;
	} else {
		cq_entry->sct = 0;
		cq_entry->sc = 0;
		g_sq_reset = g_sq_reset & (~(1<<sq_index));
		//set_sq_reset(g_sq_reset);
		set_sq_base_addr(sq_index, qbase_addr);
		set_sq_size_id(sq_index, qsize + 1, sq_id);
		AD_PRINT("Succeed! qbase_addr: 0x%lx\n\r", qbase_addr);
	}
}


void create_io_cq(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{

	nvme_sq_create_io_cq_dw10_t* sq_entry_dw10 = (nvme_sq_create_io_cq_dw10_t*)(&sq_entry->dw[10]);
	nvme_sq_create_io_cq_dw11_t* sq_entry_dw11 = (nvme_sq_create_io_cq_dw11_t*)(&sq_entry->dw[11]);

	u16 cid = sq_entry->cid;
	u16 cq_id = sq_entry_dw10->qid;
	u16 qsize = sq_entry_dw10->qsize;
	u16 irq_vector = sq_entry_dw11->iv;
	u16 irq_en = sq_entry_dw11->ien;
	u16	pc = sq_entry_dw11->pc;
	u8 psdt = sq_entry->psdt;
	u64 qbase_addr = sq_entry->prp1;

	AD_PRINT("Admin Command(Create IO CQ), cid: %d, nsid: %d, cq_id: %d, qsize: %d\n\r", cid, sq_entry->nsid, cq_id, qsize);

	u32 cq_index;
	cq_attribute_t cq_attribute;
	cq_attribute.qid = cq_id;
	cq_attribute.qsize = qsize;
	cq_attribute.iv = irq_vector;
	cq_attribute.ien = irq_en;
	cq_attribute.qbase_addr = qbase_addr;
	//memset(cq_entry, 0, NVME_CQ_ENTRY_SIZE);//1021
	cq_entry->cid = cid;
	//cq_entry->sqid = cq_id;//1021
	if((psdt != 0) || (pc != 1)){ // only support physically contiguous region
		cq_entry->sct = 0;
		cq_entry->sc = 2;
	} else if((qsize == 0) || (qsize > NVME_QUE_MAX_SIZE)){
		cq_entry->sct = 1;
		cq_entry->sc = 2;
	} else if((irq_en > 0) && (NVME_MSIX_EN > 0) && (irq_vector > NVME_MSIX_TABLE_SIZE)){
		cq_entry->sct = 1;
		cq_entry->sc = 8;
	} else if(set_cq_attribute(cq_id, &cq_attribute, &cq_index) == FALSE){ // CQ created yet
		cq_entry->sct = 1;
		cq_entry->sc = 1;
	} else {
		cq_entry->sct = 0;
		cq_entry->sc = 0;
		g_cq_reset = g_cq_reset & (~(1<<cq_index));
		//set_cq_reset(g_cq_reset);

		set_cq_base_addr(cq_index, qbase_addr);
		set_cq_size_id(cq_index, qsize + 1, cq_id);
		AD_PRINT("g_iocq_irq_enable:0x%x\n\r", g_iocq_irq_enable);
		if(irq_en){
			xdma_usr_irq_vector_set(cq_index+1, irq_vector);
			g_iocq_irq_enable = g_iocq_irq_enable | (1<<cq_index);
			set_iocq_irq_enable(g_iocq_irq_enable);
		}
		else{
			g_iocq_irq_enable = g_iocq_irq_enable & (~(1<<cq_index));
		}
		AD_PRINT("Succeed! g_iocq_irq_enable: 0x%x, irq_en: 0x%x, irq_vector: 0x%x\n\r", g_iocq_irq_enable, irq_en, irq_vector);
		AD_PRINT("Succeed! qbase_addr: 0x%lX, irq_en: 0x%x, irq_vector: 0x%x\n\r", qbase_addr, irq_en, irq_vector);
	}
}


void delete_io_sq(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	nvme_sq_delete_io_que_dw10_t* sq_entry_dw10 = (nvme_sq_delete_io_que_dw10_t*)(&sq_entry->dw[10]);
	u16 cid = sq_entry->cid;
	u16 sq_id = sq_entry_dw10->qid;
	AD_PRINT("Admin Command(Delete IO SQ), cid: %d, nsid: %d, qid: %d\n\r", cid, sq_entry->nsid, sq_id);
	u32 sq_index;
//	memset(cq_entry, 0, NVME_CQ_ENTRY_SIZE);
	cq_entry->cid = cid;
	if(del_sq_attribute(sq_id, &sq_index) == FALSE){ // SQ not created yet
		cq_entry->sct = 1;
		cq_entry->sc = 1;
	} else {
		cq_entry->sct = 0;
		cq_entry->sc = 0;
		g_sq_reset = g_sq_reset | (1<<sq_index);
		//set_sq_reset(g_sq_reset);
		set_sq_base_addr(sq_index, 0);
		set_sq_size_id(sq_index, 0, 0);
		AD_PRINT("Succeed!\n\r");
	}
}


void delete_io_cq(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	nvme_sq_delete_io_que_dw10_t* sq_entry_dw10 = (nvme_sq_delete_io_que_dw10_t*)(&sq_entry->dw[10]);
	u16 cid = sq_entry->cid;
	u16 cq_id = sq_entry_dw10->qid;
	AD_PRINT("Admin Command(Delete IO CQ), cid: %d, nsid: %d, qid: %d\n\r", cid, sq_entry->nsid, cq_id);

	u32 cq_index;
//	memset(cq_entry, 0, NVME_CQ_ENTRY_SIZE);
	cq_entry->cid = cid;
	if(del_cq_attribute(cq_id, &cq_index) == FALSE){ // CQ not created yet
		cq_entry->sct = 1;
		cq_entry->sc = 1;
	} else if(sq_exist_at_index(cq_index) == TRUE){ // paired SQ not deleted yet
		cq_entry->sct = 1;
		cq_entry->sc = 12;
	} else {
		cq_entry->sct = 0;
		cq_entry->sc = 0;
		g_cq_reset = g_cq_reset | (1<<cq_index);
		//set_cq_reset(g_cq_reset);
		set_cq_base_addr(cq_index, 0);
		set_cq_size_id(cq_index, 0, 0);
		g_iocq_irq_enable = g_iocq_irq_enable & (~(1<<cq_index));
		set_iocq_irq_enable(g_iocq_irq_enable);
//		xdma_usr_irq_vector_set(cq_index, 0);
		AD_PRINT("Succeed!\n\r");
	}
}




void get_log_page(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	nvme_sq_get_log_page_dw10_t sq_entry_dw10;
//	nvme_sq_get_log_page_dw11_t sq_entry_dw11;
	sq_entry_dw10.dw = sq_entry->dw[10];
//	sq_entry_dw11.dw = sq_entry->dw[11];
	u16 cid = sq_entry->cid;
	u8 psdt = sq_entry->psdt;
	u64 host_base_addr = sq_entry->prp1;
//	u16 num_dwords_l = sq_entry_dw10.numdl;
//	u16 num_dwords_h = sq_entry_dw11.numdu;
//	u32 num_bytes = (((u32)num_dwords_h << 16) | num_dwords_l) + 1;
//	u32 log_page_offset_l = sq_entry->dw[12] & 0xfffffffc;
//	u32 log_page_offset_h = sq_entry->dw[13];
//	u64 offset = ((u64)log_page_offset_h << 32) | log_page_offset_l;

	AD_PRINT("Admin Command(Get Log Page), cid: %d, nsid: %d, lid = %d\n\r", cid, sq_entry->nsid, sq_entry_dw10.lid);

	u32* buf = (u32*)BRAM_BUF_BASEADDR;
	//u64 addr_total;
	u32 byte_len = 512;
	u32 data_valid = 0;
	u32 ocssd_valid = 0;
	cq_entry->cid = cid;
	memset(buf, 0x0, byte_len);
	if(psdt != 0){ // only support physically contiguous region
		cq_entry->sct = 0;
		cq_entry->sc = 2;
	} else{
		switch (sq_entry_dw10.lid)
		{
			case NVME_LOG_PAGE_ID_ERROR_INFORMATION:
			{
//				memset(buf, 0x0, byte_len);
				data_valid = 1;
				break;
			}
			case NVME_LOG_PAGE_ID_SMART_HEALTH_INFORMATION:
			{
//				memset(buf, 0x0, byte_len);
				data_valid = 1;
				break;
			}
			case NVME_LOG_PAGE_ID_FIRMWARE_SLOT_INFORMATION:
			{
//				memset(buf, 0x0, byte_len);
				data_valid = 1;
				break;
			}
			case OC_LOG_PAGE_ID_CHUNK_INFO:
			{
				ocssd_valid = 1;
				IntigrateMetaData();
				AD_PRINT("SQ DPTR(L) is 0x%x!\n\r",sq_entry->dptr[0]);
				AD_PRINT("SQ DPTR(H) is 0x%x!\n\r",sq_entry->dptr[1]);
				//addr_total=((u64)(sq_entry->dptr[0]))+((u64)(sq_entry->dptr[1])<<32);
				u64 prp1 = sq_entry->prp1;
				u64 prp2 = sq_entry->prp2;
				ocssd_sq_get_log_page_dw10_t ocssd_sq_get_log_page_dw10;
				ocssd_sq_get_log_page_dw10.dw = sq_entry->dw[10];
				u32 numd = ocssd_sq_get_log_page_dw10.numd;
				u32 prp_num = ((numd+1)<<2)>>MEM_PAGE_WIDTH;
				u32 offset_mask = (1<<MEM_PAGE_WIDTH)-1;
				u32 offset = ((numd+1)<<2) & offset_mask;
				u32 start_offset = (u32)(prp1) & offset_mask;
				u32 data_length = (numd+1)<<2;
				u32 meta_data_addr = TOTAL_META_DATA_ADDR;
				if(offset!=0)
				{
					prp_num = prp_num +1;
				}

				if(prp_num == 1)
				{
					write_c2h_dsc(prp1, TOTAL_META_DATA_ADDR,2*CHUNK_NUM_PER_PU*32);
					while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
				}
				else if(prp_num == 2)
				{
					u32 fist_lenth = PRP_PAGE_SIZE - start_offset;
					write_c2h_dsc(prp1, TOTAL_META_DATA_ADDR,fist_lenth);
					while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
					write_c2h_dsc(prp2, TOTAL_META_DATA_ADDR + fist_lenth,(2*CHUNK_NUM_PER_PU*32)-fist_lenth);
					while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
				}
				else //prp num>=3
				{
					u64 prpCollectedForSlice[50];
					u32 dataLengthForSlice[50];
			        prpCollectedForSlice[0] = prp1;
			        AD_PRINT("(chunk-log)prp_num >=3\n\r");

			        u32 prpCnt = 1;
			        dataLengthForSlice[0] = PRP_PAGE_SIZE - start_offset;
			        u32 left_length = data_length - dataLengthForSlice[0];
			        AD_PRINT("(chunk-log)prpCollectedForSlice[0](L) is 0x%x!\n\r",prpCollectedForSlice[0]);
			        AD_PRINT("(chunk-log)prpCollectedForSlice[0](H) is 0x%x!\n\r",prpCollectedForSlice[0]>>32);
			        AD_PRINT("(chunk-log)dataLengthForSlice[0] is 0x%x!\n\r",dataLengthForSlice[0]);
					write_c2h_dsc(prpCollectedForSlice[0], meta_data_addr, dataLengthForSlice[0]);
					while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
					meta_data_addr = meta_data_addr + dataLengthForSlice[0];
			        int left_prp_num = prp_num-1;
			        //next prp should be restored in prpCollectedForSlice array also.
			        u64 next_prplist_addr = prp2;
			        next_prplist_addr = next_prplist_addr & (0-(1<<2));
			        //prp_list_start_offset = (u32)(prp[1]) & offset_mask;
			        int prp_list_start_offset = (u32)(next_prplist_addr) & ((1<<MEM_PAGE_WIDTH)-1);
			        int prp_offset = prp_list_start_offset >>3;
			        int prp_max_num = 1<<(MEM_PAGE_WIDTH-3);
			        u32 tmp_prp_num;
			        u32 tmp_prp_num_for_cycle;
			        u32 i = 0;
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
			            write_ioP_h2c_dsc(next_prplist_addr,PL_AD_PRP_BUF_BASEADDR,tmp_prp_num*8);
			            while((get_io_dma_status() & 0x2) == 0);

			            u32 rd_data[2];
			            u64 addr_total;

			            for( i=0;i<tmp_prp_num_for_cycle;i++)
			            {
			                rd_data[0]=Xil_In32(PL_AD_PRP_BUF_BASEADDR+i*8);
			                rd_data[1]=Xil_In32(PL_AD_PRP_BUF_BASEADDR+i*8+4);
			                addr_total=((u64)(rd_data[0]))+((u64)(rd_data[1])<<32);
			                addr_total= addr_total & (0-(1<<12)) ;
			                prpCollectedForSlice[prpCnt] = addr_total;
			                AD_PRINT("(chunk-log)prpCollectedForSlice[%d](L) is 0x%x!\n\r",prpCnt,prpCollectedForSlice[prpCnt]);
			                AD_PRINT("(chunk-log)prpCollectedForSlice[%d](H) is 0x%x!\n\r",prpCnt,prpCollectedForSlice[prpCnt]>>32);
			                if(left_length>PRP_PAGE_SIZE)
			                {
			                    dataLengthForSlice[prpCnt] = PRP_PAGE_SIZE;
			                }
			                else
			                {
			                    dataLengthForSlice[prpCnt] = left_length;
			                }
			                AD_PRINT("(chunk-log)dataLengthForSlice[%d] is 0x%x!\n\r",prpCnt,dataLengthForSlice[prpCnt]);
							write_c2h_dsc(prpCollectedForSlice[prpCnt], meta_data_addr, dataLengthForSlice[prpCnt]);
							while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
							meta_data_addr = meta_data_addr + dataLengthForSlice[prpCnt];
			                left_length = left_length - dataLengthForSlice[prpCnt++];//tmp_length
			            }
			            if(tmp_prp_num_for_cycle == tmp_prp_num-1)
			            {
			                rd_data[0]=Xil_In32(PL_AD_PRP_BUF_BASEADDR+i*8);
			                rd_data[1]=Xil_In32(PL_AD_PRP_BUF_BASEADDR+i*8+4);
			                next_prplist_addr = ((u64)(rd_data[0]))+((u64)(rd_data[1])<<32);
			                next_prplist_addr = next_prplist_addr & (0-(1<<2)) ;
			                prp_offset = (next_prplist_addr & offset_mask)>>3;
			            }
			        }//for (;left_prp_num>0;)

				}//else prp num>3




				AD_PRINT("get chunk dsc log done!\n\r");
				break;
			}
			case OC_LOG_CHUNK_NOTIFICATION_LOG:
			{
				ocssd_valid = 1;
				CHUNK_NOTIFICATION_ENTRY* chunk_notification_entry = (void*)CHUNK_LOG_NOTIFICATION_ENTRY;
				memset((void*)CHUNK_LOG_NOTIFICATION_ENTRY,0,sizeof(CHUNK_NOTIFICATION_ENTRY));
				chunk_notification_entry->nc = nc;
				chunk_notification_entry->nsid = 1;
				chunk_notification_entry->cover_chunk=1;
				write_c2h_dsc(host_base_addr, CHUNK_LOG_NOTIFICATION_ENTRY,sizeof(CHUNK_NOTIFICATION_ENTRY));
				while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
				AD_PRINT("chunk notification done!\n\r");
				nc++;
				break;
			}
			default:
			{
				data_valid = 0;
				AD_PRINT("Unsupported LID!\n\r");
			}
		}
		if(ocssd_valid)
		{
			cq_entry->sct = 0;
			cq_entry->sc = 0;
			AD_PRINT("OCSSD Get Log Page Succeed!\n\r");
			return;
		}
		if(data_valid == 0){
			cq_entry->sct = 0;
			cq_entry->sc = 2;  // invalid field: unsupported LID value
		} else if(nvme_write_cq_data(host_base_addr, buf, byte_len) == FALSE){
			cq_entry->sct = 0;
			cq_entry->sc = 4; // DATA transfer error
		} else {
			cq_entry->sct = 0;
			cq_entry->sc = 0;
			AD_PRINT("Succeed!\n\r");
		}
	}
}

void identify_namespace_data(u32* buf)
{
	memset(buf, 0, sizeof(nvme_identify_namespace_data_t));
	nvme_identify_namespace_data_t* namespace_data = (nvme_identify_namespace_data_t*)buf;

	namespace_data->nsze = 0x10000ULL;   //512blocks*128pages
	namespace_data->ncap = 0x10000ULL;
	namespace_data->nuse = 0x10000ULL;

	// NSFEAT
	namespace_data->thin_provisioning = 0;
	namespace_data->namespace_atomic = 0;
	namespace_data->deallocated = 0;
	namespace_data->unique_id = 0;
	namespace_data->io_optimization = 0;

	namespace_data->nlbaf = 0;

	//FLBAS
	namespace_data->lba_format_index = 0;
	namespace_data->contiguous_metadata_buffer = 0;

	// MC
	namespace_data->metadata_as_part_of_extened_data_lba = 0;
	namespace_data->meatadata_pointer = 0;

	// DPC
	namespace_data->protection_information_type_1 = 0;
	namespace_data->protection_information_type_2 = 0;
	namespace_data->protection_information_type_3 = 0;
	namespace_data->first_eight_bytes_of_metadata = 0;
	namespace_data->last_eight_bytes_of_metadata = 0;

	// DPS
	namespace_data->protection_information = 0;
	namespace_data->first_eight_bytes_enable = 0;

	// NMIC
	namespace_data->namespace_sharing = 0;

	// RESCAP
	namespace_data->persist_through_power_loss = 0;
	namespace_data->write_exclusive_reservation = 0;
	namespace_data->exclusive_access_reservation = 0;
	namespace_data->write_exclusive_registrants_only = 0;
	namespace_data->exclusive_access_registrants_only = 0;
	namespace_data->write_exclusive_all_registrants = 0;
	namespace_data->exclusive_access_all_registrants = 0;
	namespace_data->ignore_existing_key_revision_1_3 = 0;

	// FPI
	namespace_data->remained_percentage_of_format_nvm = 0;
	namespace_data->format_progress_indicator = 0;

	// DLFEAT
	namespace_data->deallocated_value = 0;
	namespace_data->deallocate_bit_in_write_zeros = 0;
	namespace_data->crc_for_guard_field = 0;

	namespace_data->nawun = 0;
	namespace_data->nawupf = 0;
	namespace_data->nacwu = 0;
	namespace_data->nabsn = 0;
	namespace_data->nabo = 0;
	namespace_data->nabspf = 0;
	namespace_data->noiob = 0;

	memset(namespace_data->nvmcap, 0x0, 16);
	memset(namespace_data->nguid, 0x0, 16);
	memset(namespace_data->eui64, 0x0, 8);

	namespace_data->lba_format[0].ms = 0;
	namespace_data->lba_format[0].lbads = 0xc;	// d:8192 bytes c:4096bytes
	namespace_data->lba_format[0].rp = 0;
	namespace_data->vendor_specific[0] =1;
}


void identify_controller_data(u32* buf)
{
	memset(buf, 0, sizeof(nvme_identify_controller_data_t));
	nvme_identify_controller_data_t* controller_data = (nvme_identify_controller_data_t*)buf;

	controller_data->vid = PCI_VENDOR_ID;
	controller_data->ssvid = PCI_SUBSYSTEM_VENDOR_ID;

	memcpy(controller_data->sn, SERIAL_NUMBER, sizeof(SERIAL_NUMBER));
	memcpy(controller_data->mn, MODEL_NUMBER, sizeof(MODEL_NUMBER));
	memcpy(controller_data->fr, FIRMWARE_REVISION, sizeof(FIRMWARE_REVISION));

	controller_data->rab = 0;

	const u32 ieee = 0xAEBEAF;
	const u32 ieee_byte_size = 3;
	memcpy(controller_data->ieee, &ieee, ieee_byte_size);

	// CMIC
	controller_data->multi_subsystem_port = 0;
	controller_data->multi_controller = 0;
	controller_data->sr_iov = 0;
	controller_data->asymmetric_ns_access_rpt = 0;

	controller_data->mdts = 10;
	controller_data->cntlid = CONTROLLER_ID;
	controller_data->ver = NVME_VERSION;
	controller_data->rtd3r = 0;
	controller_data->rtd3e = 0;

	// OAES
	controller_data->namespace_attribute_notices = 0; // OAES:8
	controller_data->firmware_activation_notices = 0; // OAES:9
	controller_data->asymmetric_namespace_access = 0; // OAES:11
	controller_data->predictable_latency_event = 0;   // OAES:12
	controller_data->lba_status_info = 0;             // OAES:13
	controller_data->endurance_group_event = 0;       // OAES:14

	// CTRATT
	controller_data->host_id_128bit = 0; // CTRATT:0
	controller_data->non_operational_power_state_permissive_mode = 0; // CTRATT:1
	controller_data->nvm_sets = 0; // CTRATT:2
	controller_data->read_recovery_levels = 0; // CTRATT:3
	controller_data->endurance_groups = 0; // CTRATT:4
	controller_data->predictable_latency_mode = 0; // CTRATT:5
	controller_data->traffic_based_keep_alive_support = 0; // CTRATT:6
	controller_data->namespace_granularity = 0; // CTRATT:7
	controller_data->sq_associations = 0; // CTRATT:8
	controller_data->uuid_list = 0; // CTRATT:9

	controller_data->rrls = 0;
	controller_data->cntrltype = 1;
	memset(controller_data->fguid, 0x0, 16);
	memset(controller_data->management_interface, 0x0, 16);
	controller_data->crdt1 = 0;
	controller_data->crdt2 = 0;
	controller_data->crdt3 = 0;

	// OACS
	controller_data->security_send_and_receive = 0;
	controller_data->format_nvm = 0;
	controller_data->firmware_commit_and_download = 0;
	controller_data->namespace_management = 0;
	controller_data->device_self_test = 0;
	controller_data->directives = 0;
	controller_data->nvme_mi_send_receive = 0;
	controller_data->virtualization_management = 0;
	controller_data->doorbell_buffer_config = 0;
	controller_data->get_lba_status = 0;

	controller_data->acl = 3;
	controller_data->aerl = 3;

	// FRMW
	controller_data->first_firmware_slot_read_only = 1;
	controller_data->number_of_firmware_slots = 1;
	controller_data->firmware_activation_without_reset = 0;

	// LPA
	controller_data->smart_health_info_log_page = 0;
	controller_data->command_supported_and_effects_log_page = 0;
	controller_data->extended_data_for_log_page = 0;
	controller_data->telemetry_initiated_log_page = 0;
	controller_data->persistent_event_log = 0;

	controller_data->elpe = 63;
	controller_data->npss = 0;

	// AVSCC
	controller_data->default_format_for_admin_vendor = 1;

	// APSTA
	controller_data->auto_power_state_transition = 0;

	controller_data->wctemp = 0x157;
	controller_data->cctemp = 0x160;
	controller_data->mtfa = 50;
	controller_data->hmpre = 0;
	controller_data->hmmin = 0;

	controller_data->tnvmcap[0] = 0x0;
	controller_data->tnvmcap[1] = 0x100;
	controller_data->tnvmcap[2] = 0x0;
	controller_data->tnvmcap[3] = 0x0;
	memset(controller_data->unvmcap, 0x0, 16);

	// RPMPS
	controller_data->number_of_rpmb_units = 0;
	controller_data->authentication_method = 0;
	controller_data->total_size = 0;
	controller_data->access_size = 0;

	controller_data->edstt = 0;

	// DSTO
	controller_data->only_one_device_self_test = 0;

	controller_data->fwug = 0;
	controller_data->kas = 0;

	// HCTMA
	controller_data->host_controlled_thermal_management = 0;

	controller_data->mntmt = 0;
	controller_data->mxtmt = 0;

	// SANICAP
	controller_data->crypto_erase_sanitize = 0;
	controller_data->block_erase_sanitize = 0;
	controller_data->overwrite_sanitize = 0;
	controller_data->no_deallocate_inhibited = 0;
	controller_data->no_deallocate_modifies_media = 0;

	controller_data->hmminds = 0;
	controller_data->hmmaxd = 0;
	controller_data->nsetidmax = 0;
	controller_data->endgidmax = 0;
	controller_data->anatt = 0;

	// ANACAP
	controller_data->ana_optimized = 0;
	controller_data->ana_non_optimized = 0;
	controller_data->ana_inaccessible = 0;
	controller_data->ana_persistent_loss = 0;
	controller_data->ana_change = 0;
	controller_data->anacap_reserved0 = 0;
	controller_data->anagrpid_not_change = 0;
	controller_data->anagrpid_non_zero = 0;

	controller_data->anagrpmax = 0;
	controller_data->nanagrpid = 0;
	controller_data->pels = 0;

	controller_data->maximum_sq_entry_size = 6;
	controller_data->required_sq_entry_size = 6;
	controller_data->maximum_cq_entry_size = 4;
	controller_data->required_cq_entry_size = 4;
	controller_data->maxcmd = 0;
	controller_data->nn = 1;

	// ONCS
	controller_data->compare = 0;
	controller_data->write_uncorrectable = 0;
	controller_data->dataset_management = 0;
	controller_data->write_zeroes = 0;
	controller_data->save_field_for_feature = 0;
	controller_data->reservation = 0;
	controller_data->timestamp = 0;
	controller_data->verify = 0;

	// FUSES
	controller_data->compare_and_write_fused = 0;

	//FNA
	controller_data->format_apply_to_all_namespaces = 0;
	controller_data->secure_erase_apply_to_all_namespaces = 0;
	controller_data->cryptographic_erase = 0;

	// VWC
	controller_data->volatile_write_cache = 0;
	controller_data->flush_behavior = 0;

	controller_data->awun = 255;
	controller_data->awupf = 7;

	// NVSCC
	controller_data->default_format_for_nvm_vendor = 1;

	controller_data->acwu = 0;

	// NWPC
	controller_data->no_write_protect = 0;
	controller_data->write_protect_until_power_cycle = 0;
	controller_data->permanent_write_protect = 0;

	//SGLS
	controller_data->support_type = 0;
	controller_data->keyed_sgl_data_block = 0;
	controller_data->bit_bucket = 0;
	controller_data->contiguous_physical_buffer_for_metadata = 0;
	controller_data->larger_data_than_to_be_transferred = 0;
	controller_data->metadata_pointer_contain_sgl_segment = 0;
	controller_data->address_field_specify_offset = 0;
	controller_data->transport_sgl_data_block = 0;

	controller_data->mnan = 0;

//	memcpy(controller_data->subnqn, SUBNQN, sizeof(SUBNQN));
//	memset(controller_data->subnqn, 0x0, 256);
//	memset(controller_data->nvme_over_fabrics, 0x0, 256);
	controller_data->psd[0].mp = 800;
	controller_data->psd[0].mxps = 0;
	controller_data->psd[0].nops = 0;
	controller_data->psd[0].enlat = 0;
	controller_data->psd[0].exlat = 0;
	controller_data->psd[0].rrt = 0;
	controller_data->psd[0].rrl = 0;
	controller_data->psd[0].rwt = 0;
	controller_data->psd[0].rwl = 0;
	controller_data->psd[0].idlp = 0;
	controller_data->psd[0].ips = 0;
	controller_data->psd[0].actp = 0;
	controller_data->psd[0].apw = 0;
	controller_data->psd[0].aps = 0;
//	controller_data->psd[1].mp = 450;
//	controller_data->psd[1].mxps = 0;
//	controller_data->psd[1].nops = 0;
//	controller_data->psd[1].enlat = 5;
//	controller_data->psd[1].exlat = 5;
//	controller_data->psd[1].rrt = 1;
//	controller_data->psd[1].rrl = 1;
//	controller_data->psd[1].rwt = 1;
//	controller_data->psd[1].rwl = 1;
//	controller_data->psd[1].idlp = 0;
//	controller_data->psd[1].ips = 0;
//	controller_data->psd[1].actp = 0;
//	controller_data->psd[1].apw = 0;
//	controller_data->psd[1].aps = 0;
//	controller_data->psd[2].mp = 300;
//	controller_data->psd[2].mxps = 0;
//	controller_data->psd[2].nops = 0;
//	controller_data->psd[2].enlat = 5;
//	controller_data->psd[2].exlat = 5;
//	controller_data->psd[2].rrt = 2;
//	controller_data->psd[2].rrl = 2;
//	controller_data->psd[2].rwt = 2;
//	controller_data->psd[2].rwl = 2;
//	controller_data->psd[2].idlp = 0;
//	controller_data->psd[2].ips = 0;
//	controller_data->psd[2].actp = 0;
//	controller_data->psd[2].apw = 0;
//	controller_data->psd[2].aps = 0;
//	controller_data->psd[3].mp = 7;
//	controller_data->psd[3].mxps = 0;
//	controller_data->psd[3].nops = 1;
//	controller_data->psd[3].enlat = 1000;
//	controller_data->psd[3].exlat = 5000;
//	controller_data->psd[3].rrt = 3;
//	controller_data->psd[3].rrl = 3;
//	controller_data->psd[3].rwt = 3;
//	controller_data->psd[3].rwl = 3;
//	controller_data->psd[3].idlp = 0;
//	controller_data->psd[3].ips = 0;
//	controller_data->psd[3].actp = 0;
//	controller_data->psd[3].apw = 0;
//	controller_data->psd[3].aps = 0;
//	controller_data->psd[4].mp = 1;
//	controller_data->psd[4].mxps = 0;
//	controller_data->psd[4].nops = 1;
//	controller_data->psd[4].enlat = 5000;
//	controller_data->psd[4].exlat = 50000;
//	controller_data->psd[4].rrt = 4;
//	controller_data->psd[4].rrl = 4;
//	controller_data->psd[4].rwt = 4;
//	controller_data->psd[4].rwl = 4;
//	controller_data->psd[4].idlp = 0;
//	controller_data->psd[4].ips = 0;
//	controller_data->psd[4].actp = 0;
//	controller_data->psd[4].apw = 0;
//	controller_data->psd[4].aps = 0;

}



void identify(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	nvme_sq_identify_dw10_t sq_entry_dw10;
	sq_entry_dw10.dw = sq_entry->dw[10];
	u32 nsid = sq_entry->nsid;
	u16 cid = sq_entry->cid;
	u8 psdt = sq_entry->psdt;
	u64 host_base_addr = sq_entry->prp1;

	AD_PRINT("Admin Command(Identify), cid: %d, nsid: %d, cns: 0x%X\n\r", cid, nsid, sq_entry_dw10.cns);

	u32* buf = (u32*)BRAM_BUF_BASEADDR;
	u32 byte_len = 4096;
	u32 data_valid = 0;
	cq_entry->cid = cid;
	if(psdt != 0){ // only support physically contiguous region
		cq_entry->sct = 0;
		cq_entry->sc = 2;
	} else{
		switch (sq_entry_dw10.cns)
		{
			case NVME_SQ_IDENTIFY_CNS_NAMESPACE_DATA:
			{
				if(nsid == 1){
					identify_namespace_data(buf);
					data_valid = 1;
				} else{
					data_valid = 0;
					cq_entry->sct = 0;
					cq_entry->sc = 0xB; // invalid namespace or format
				}
				break;
			}
			case NVME_SQ_IDENTIFY_CNS_CONTOLLER_DATA:
			{
				identify_controller_data(buf);
				data_valid = 1;
				break;
			}
			case NVME_SQ_IDENTIFY_CNS_ACTIVE_NAMESPACE_LIST:
			{
				if((nsid != 0xffffffff) && (nsid != 0xfffffffe)){
					memset(buf, 0x0, byte_len);
					nvme_namespace_list_t* namespace_list = (nvme_namespace_list_t*)buf;
					namespace_list->identifier[0] = (nsid == 0)? 1 : 0;
					data_valid = 1;
				} else {
					data_valid = 0;
					cq_entry->sct = 0;
					cq_entry->sc = 0xB; // invalid namespace or format
				}
				break;
			}
			case NVME_SQ_IDENTIFY_CNS_NAMESPACE_IDENTIFICATION:
			{
				memset(buf, 0x0, byte_len);
				data_valid = 1;
				break;
			}
			default:
			{
				AD_PRINT("Unsupported Identify CNS Value!\n\r");
				data_valid = 0;
				cq_entry->sct = 0;
				cq_entry->sc = 2;  // invalid field: unsupported CNS value
			}
		}
		if(data_valid && (nvme_write_cq_data(host_base_addr, buf, byte_len) == FALSE)){
			cq_entry->sct = 0;
			cq_entry->sc = 4; // DATA transfer error
			AD_PRINT("FALSE!\n\r");
			AD_PRINT("data_valid is %x!\n",&data_valid);
		} else if(data_valid) {
			cq_entry->sct = 0;
			cq_entry->sc = 0;
			AD_PRINT("Succeed!\n\r");
		}
	}
}


void nvme_set_feature(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	nvme_sq_set_feature_dw10_t sq_entry_dw10;
	sq_entry_dw10.dw = sq_entry->dw[10];
	u16 cid = sq_entry->cid;

	AD_PRINT("Admin Command(Set Feature), cid: %d, nsid: 0%d, fid: 0x%X\n\r", cid, sq_entry->nsid, sq_entry_dw10.fid);

	cq_entry->cid = cid;
	switch (sq_entry_dw10.fid)
	{
		case NVME_FEATURE_ID_ARBITRATION:
		{
			AD_PRINT("NVME_FEATURE_ID_ARBITRATION\n\r");

			break;
		}
		case NVME_FEATURE_ID_POWER_MANAGEMENT:
		{
			AD_PRINT("NVME_FEATURE_ID_POWER_MANAGEMENT\n\r");

			break;
		}
		case NVME_FEATURE_ID_TEMPERATURE_THRESHOLD:
		{
			AD_PRINT("NVME_FEATURE_ID_TEMPERATURE_THRESHOLD\n\r");

			break;
		}
		case NVME_FEATURE_ID_ERROR_RECOVERY:
		{
			AD_PRINT("NVME_FEATURE_ID_ERROR_RECOVERY\n\r");

			break;
		}
		case NVME_FEATURE_ID_NUMBER_OF_QUEUE:
		{
			AD_PRINT("NVME_FEATURE_ID_NUMBER_OF_QUEUE\n\r");
			nvme_sq_set_features_number_of_queues_dw11_t sq_entry_dw11;
			sq_entry_dw11.dw = sq_entry->dw[11];
			u16 sq_allocated = MIN(sq_entry_dw11.nsqr, NVME_IO_SQ_NUM - 1);
			u16 cq_allocated = MIN(sq_entry_dw11.ncqr, NVME_IO_CQ_NUM - 1);
			sq_entry_dw11.nsqr = sq_allocated;
			sq_entry_dw11.ncqr = cq_allocated;
//			sq_entry_dw11.nsqr = NVME_IO_SQ_NUM - 1;
//			sq_entry_dw11.ncqr = NVME_IO_CQ_NUM - 1;

			cq_entry->command_specific = sq_entry_dw11.dw;
			AD_PRINT("Admin Command(NVME_FEATURE_ID_NUMBER_OF_QUEUE), sq_entry_dw11: 0x%X\n\r", sq_entry_dw11.dw);

			break;
		}
		case NVME_FEATURE_ID_INTERRUPT_COALESCING:
		{
			AD_PRINT("NVME_FEATURE_ID_INTERRUPT_COALESCING\n\r");

			break;
		}
		case NVME_FEATURE_ID_INTERRUPT_VECTOR_CONFIG:
		{
			AD_PRINT("NVME_FEATURE_ID_INTERRUPT_VECTOR_CONFIG\n\r");

			break;
		}
		case NVME_FEATURE_ID_WRITE_ATOMICITY_NORMAL:
		{
			AD_PRINT("NVME_FEATURE_ID_WRITE_ATOMICITY_NORMAL\n\r");

			break;
		}
		case NVME_FEATURE_ID_ASYNCHRONOUS_EVENT_CONFIG:
		{
			AD_PRINT("NVME_FEATURE_ID_ASYNCHRONOUS_EVENT_CONFIG\n\r");
			AD_PRINT("Admin Command(NVME_FEATURE_ID_ASYNCHRONOUS_EVENT_CONFIG), sq_entry_dw11: 0x%X\n\r", sq_entry->dw[11]);

			break;
		}
		case NVME_FEATURE_ID_AUTO_POWER_STATE_TRANSITION:
		{
//			nvme_sq_set_features_auto_power_state_transition_dw11_t sq_entry_dw11;;
//			sq_entry_dw11.dw = sq_entry->dw[11];
//			if(sq_entry_dw11.apste == 1)
//				//xil_printf("Auto power state transition enable\n\r");
//			else
//				//xil_printf("Auto power state transition disable\n\r");

			break;
		}
		default:
		{
			AD_PRINT("Unsupported Set Feature FID!\n\r");
			cq_entry->sct = 0;
			cq_entry->sc = 2;  // invalid field: unsupported FID value
		}
	}
}


void nvme_get_feature(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	nvme_sq_get_feature_dw10_t sq_entry_dw10;
	sq_entry_dw10.dw = sq_entry->dw[10];
	u16 cid = sq_entry->cid;

	AD_PRINT("Admin Command(Get Feature), cid: %d, nsid: %d, fid: 0x%X\n\r", cid, sq_entry->nsid, sq_entry_dw10.fid);
	cq_entry->cid = cid;
	switch (sq_entry_dw10.fid)
	{
		case NVME_FEATURE_ID_ARBITRATION:
		{
			AD_PRINT("NVME_FEATURE_ID_ARBITRATION\n\r");
			nvme_sq_set_features_arbitration_dw11_t sq_entry_dw11;
			sq_entry_dw11.ab = 7;
			sq_entry_dw11.reserved0 = 0;
			sq_entry_dw11.lpw = 0xff;
			sq_entry_dw11.mpw = 0xff;
			sq_entry_dw11.hpw = 0xff;
			cq_entry->command_specific = sq_entry_dw11.dw;
			break;
		}
		case NVME_FEATURE_ID_POWER_MANAGEMENT:
		{
			AD_PRINT("NVME_FEATURE_ID_POWER_MANAGEMENT\n\r");
			nvme_sq_set_features_power_management_dw11_t sq_entry_dw11;
			sq_entry_dw11.dw = 0;
			sq_entry_dw11.ps = 0;
			sq_entry_dw11.wh = 0;
			cq_entry->command_specific = sq_entry_dw11.dw;
			break;
		}
		case NVME_FEATURE_ID_TEMPERATURE_THRESHOLD:
		{
			AD_PRINT("NVME_FEATURE_ID_TEMPERATURE_THRESHOLD\n\r");

			break;
		}
		case NVME_FEATURE_ID_ERROR_RECOVERY:
		{
			AD_PRINT("NVME_FEATURE_ID_ERROR_RECOVERY\n\r");

			break;
		}
		case NVME_FEATURE_ID_NUMBER_OF_QUEUE:
		{
			nvme_sq_set_features_number_of_queues_dw11_t sq_entry_dw11;
			sq_entry_dw11.nsqr = NVME_IO_SQ_NUM - 1;
			sq_entry_dw11.ncqr = NVME_IO_CQ_NUM - 1;

			cq_entry->command_specific = sq_entry_dw11.dw;

			break;
		}
		case NVME_FEATURE_ID_INTERRUPT_COALESCING:
		{
			AD_PRINT("NVME_FEATURE_ID_INTERRUPT_COALESCING\n\r");

			break;
		}
		case NVME_FEATURE_ID_INTERRUPT_VECTOR_CONFIG:
		{
			AD_PRINT("NVME_FEATURE_ID_INTERRUPT_VECTOR_CONFIG\n\r");

			break;
		}
		case NVME_FEATURE_ID_WRITE_ATOMICITY_NORMAL:
		{
			AD_PRINT("NVME_FEATURE_ID_WRITE_ATOMICITY_NORMAL\n\r");

			break;
		}
		case NVME_FEATURE_ID_ASYNCHRONOUS_EVENT_CONFIG:
		{
			AD_PRINT("NVME_FEATURE_ID_ASYNCHRONOUS_EVENT_CONFIG\n\r");

			break;
		}
		case NVME_FEATURE_ID_AUTO_POWER_STATE_TRANSITION:
		{
			AD_PRINT("NVME_FEATURE_ID_AUTO_POWER_STATE_TRANSITION\n\r");
			nvme_sq_set_features_auto_power_state_transition_dw11_t sq_entry_dw11;;
			sq_entry_dw11.dw = 0;
			sq_entry_dw11.apste = 0;
			cq_entry->command_specific = sq_entry_dw11.dw;
			u64 host_base_addr = sq_entry->prp1;
			u64 entries[32];
			memset(entries, 0x0, 256);
			if(nvme_write_cq_data(host_base_addr, (u32 *)entries, 256) == FALSE){
				cq_entry->sct = 0;
				cq_entry->sc = 4; // DATA transfer error
			}
			break;
		}
		default:
		{
			AD_PRINT("Unsupported Get Feature FID!\n\r");
			cq_entry->sct = 0;
			cq_entry->sc = 2;  // invalid field: unsupported FID value
		}
	}

}


void async_event_req(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	AD_PRINT("Admin Command(Asynchronous Event Request)\n\r");
	cq_entry->cid = sq_entry->cid;
}


void cmd_abort(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	AD_PRINT("Admin Command(Abort)\n\r");
	cq_entry->cid = sq_entry->cid;
	cq_entry->command_specific = 0x1;
}

void submit_geometry(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry)
{
	int i;
	u64 addr_total;
	P_GEOMETRY_STRUCTURE p_geometry_data;
	AD_PRINT("Admin Command(Geometry)\n\r");
	cq_entry->cid = sq_entry->cid;
	//memset((void*)GEOMETRY_DATA_ADDR, 0x00, sizeof(GEOMETRY_STRUCTURE));
	//AD_PRINT("size of GEOMETRY_STRUCTURE* is %d\n\r",sizeof(GEOMETRY_STRUCTURE*));
	p_geometry_data = (void*)GEOMETRY_DATA_ADDR;//(P_GEOMETRY_STRUCTURE)
	//Major Version Number (MJR)
	p_geometry_data->MJR = 2;
	//Minor Version Number (MNR)
	p_geometry_data->MNR = 0;
	//reserved1
	p_geometry_data->reserved1[0] = 0;
	p_geometry_data->reserved1[1] = 0;
	p_geometry_data->reserved1[2] = 0;
	p_geometry_data->reserved1[3] = 0;
	p_geometry_data->reserved1[4] = 0;
	p_geometry_data->reserved1[5] = 0;
	//LBA Format(LBAF)
	p_geometry_data->LBAF = 0;            //initial total
	p_geometry_data->GBL  = 1;
	p_geometry_data->PUBL = 0;
	p_geometry_data->CBL  = 8;
	p_geometry_data->LBBL = 7;
	//Media and Controller Capabilities (MCCAP)
	p_geometry_data->MCCAP = 0;
	p_geometry_data->multiResetChunkEnable = MULTI_RESET_EN;
	//reserved3
	for(i = 0 ; i < 12 ; i++)
	{
		p_geometry_data->reserved3[i] = 0;
	}
    //Wear-level Index Delta Threshold (WIT):
	p_geometry_data->WIT = 2;
	//reserved4
	for(i = 0 ; i < 31 ; i++)
	{
		p_geometry_data->reserved4[i] = 0;
	}
	//*******************
	//Geometry Related
	//*******************
	//Number of Groups (NUM_GRP)
	p_geometry_data->NUM_GRP = 2;
	//Number of parallel units per group (NUM_PU)
	p_geometry_data->NUM_PU = 1;
	//Number of chunks per parallel unit (NUM_CHK)
	p_geometry_data->NUM_CHK = 256;
	//Chunk Size (CLBA)
	p_geometry_data->CLBA = 128;

	//reserved5
	for(i = 0 ; i < 55 ; i++)
	{
		p_geometry_data->reserved5[i] = 0;
	}
	//**************************
	//Write Data Requirements
	//**************************
	//Minimum Write Size (WS_MIN)
	p_geometry_data->WS_MIN = 1;
	//Optimal Write Size (WS_OPT)
	p_geometry_data->WS_OPT = WS_OPT_DATA;
	//Cache Minimum Write Size Units (MW_CUNITS)
	p_geometry_data->MW_CUNITS = MW_CUNITS_DATA;
	//Maximum Open Chunks (MAXOC)
	p_geometry_data->MAXOC = 0;
    //Maximum Open Chunks per PU (MAXOCPU)
	p_geometry_data->MAXOCPU = 0;

	//reserved6
	for(i = 0 ; i < 44 ; i++)
	{
		p_geometry_data->reserved6[i] = 0;
	}
	//*****************************
	//Performance Related Metrics
	//*****************************
	//tRD Typical (TRDT)
	p_geometry_data->TRDT = 1000000;
	//tRD Max (TRDM)
	p_geometry_data->TRDM = 23000000;
	//tWR Typical (TWRT)
	p_geometry_data->TWRT = 1000000;
	//tWR Max (TWRM)
	p_geometry_data->TWRM = 23000000;
	//tCRS Typical (TCRST)
	p_geometry_data->TCRST = 1000000;
	//tCRS Max (TCRSM)
	p_geometry_data->TCRSM = 23000000;
	//reserved7
	for(i = 0 ; i < 40 ; i++)
	{
		p_geometry_data->reserved7[i] = 0;
	}
	//reserved8
	for(i = 0 ; i < 2816 ; i++)
	{
		p_geometry_data->reserved8[i] = 0;
	}
	//Vendor Specific
	for(i = 0 ; i < 1024 ; i++)
	{
		p_geometry_data->vendorSpecific[i] = 0;
	}
	AD_PRINT("SQ DPTR(L) is 0x%x!\n\r",sq_entry->dptr[0]);
	AD_PRINT("SQ DPTR(H) is 0x%x!\n\r",sq_entry->dptr[1]);
	addr_total=((u64)(sq_entry->dptr[0]))+((u64)(sq_entry->dptr[1])<<32);
	write_c2h_dsc( addr_total,GEOMETRY_DATA_ADDR,4096);
	while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
	AD_PRINT("write geometry data ture!\n\r");


}
