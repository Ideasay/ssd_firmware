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

#include "nvme_transmitter.h"

u32 g_sq_buf_rptr = 0; // SQ buffer read pointer
u32 g_cq_buf_wptr = 0; // CQ buffer write pointer

u32 g_io_sq_buf_rptr = 0; // SQ buffer read pointer
u32 g_io_cq_buf_wptr = 0; // CQ buffer write pointer

void nvme_transmitter_init()
{
	g_sq_buf_rptr = 0;
	g_cq_buf_wptr = 0;
	g_io_sq_buf_rptr = 0; // SQ buffer read pointer
	g_io_cq_buf_wptr = 0; // CQ buffer write pointer
}

// Host to Card(PS) direction
void write_h2c_dsc(u64 base_addr,u64 dst_addr,u32 len)
{
	set_asq_dsc_dst_addr(dst_addr);
	set_asq_dsc_addr(base_addr);
	set_asq_dsc_len(len);
	set_asq_dsc_ctl(1);
	set_asq_dsc_ctl(0);
}


// Card(PS) to Host direction
void write_c2h_dsc(u64 base_addr,u64 src_addr,u32 len)
{
	set_acq_dsc_src_addr(src_addr);
	AD_PRINT("L_src_addr is %x!\n",src_addr);
	AD_PRINT("H_src_addr is %x!\n",src_addr>>32);
	set_acq_dsc_addr(base_addr);
	AD_PRINT("base_addr is %x!\n",base_addr);
	set_acq_dsc_len(len);
	AD_PRINT("len is %x!\n",len);
	set_acq_dsc_ctl(1);
	set_acq_dsc_ctl(0);
}

void write_ioD_h2c_dsc(u64 base_addr,u64 dst_addr,u32 len)
{
	set_ioh2cD_dsc_dst_addr(dst_addr);
	set_ioh2cD_dsc_addr(base_addr);
	set_ioh2cD_dsc_len(len);
	set_ioh2cD_dsc_ctl(1);
	set_ioh2cD_dsc_ctl(0);
}

void write_ioP_h2c_dsc(u64 base_addr,u64 dst_addr,u32 len)
{
	set_ioh2cP_dsc_dst_addr(dst_addr);
	set_ioh2cP_dsc_addr(base_addr);
	set_ioh2cP_dsc_len(len);
	set_ioh2cP_dsc_ctl(1);
	set_ioh2cP_dsc_ctl(0);
}

void write_ioD_c2h_dsc(u64 base_addr,u64 src_addr,u32 len)
{


	///"L_src_addr is %x!\n",src_addr);
	IO_PRINT("H_src_addr is %x!\n",src_addr>>32);
	IO_PRINT("base_addr is %x!\n",base_addr);
	IO_PRINT("len is %x!\n",len);

	set_ioc2hD_dsc_src_addr(src_addr);
	set_ioc2hD_dsc_addr(base_addr);
	set_ioc2hD_dsc_len(len);
	set_ioc2hD_dsc_ctl(1);
	set_ioc2hD_dsc_ctl(0);
}

// Read SQ entry from PL-side buffer
// return TRUE/FALSE
u32 nvme_read_sq_entry(nvme_sq_entry_t* sq_entry)
{
	u32 sq_buf_wptr = get_asq_buf_wptr();
	u32 i;
	u32 rd_data;
	if(sq_buf_wptr != g_sq_buf_rptr){
		AD_PRINT("sq_buf_wptr: %x\n\r", sq_buf_wptr);
		//mem_read((PL_SQ_ENTRY_BUF_BASEADDR + g_sq_buf_rptr * NVME_SQ_ENTRY_SIZE), (void *)sq_entry, NVME_SQ_ENTRY_SIZE);

		for(i=0; i<16;i++)
		{
			rd_data=Xil_In32(PL_SQ_ENTRY_BUF_BASEADDR+g_sq_buf_rptr* NVME_SQ_ENTRY_SIZE+i*4);
			AD_PRINT("ADMIN Read [%d]: %x\n\r", i,rd_data);
			sq_entry->dw[i]=rd_data;
		}
		if(g_sq_buf_rptr < (PL_SQ_ENTRY_NUM - 1))
			g_sq_buf_rptr += 1;
		else
			g_sq_buf_rptr = 0;
		return TRUE;
	} else{ // have no new SQ entry
		return FALSE;
	}
}

u32 nvme_read_io_sq_entry(nvme_sq_entry_t* sq_entry)
{
	u32 io_sq_buf_wptr = get_io0_sq_buf_wptr();
	u32 i;
	u32 rd_data;
	if(io_sq_buf_wptr != g_io_sq_buf_rptr){
		IO_PRINT("io_sq_buf_wptr: %x\n\r", io_sq_buf_wptr);
		//mem_read((PL_SQ_ENTRY_BUF_BASEADDR + g_sq_buf_rptr * NVME_SQ_ENTRY_SIZE), (void *)sq_entry, NVME_SQ_ENTRY_SIZE);

		for(i=0; i<16;i++)
		{
			rd_data=Xil_In32(PL_SQ_ENTRY_BUF_BASEADDR+g_io_sq_buf_rptr* NVME_SQ_ENTRY_SIZE+i*4+1024);//1024 is the bias addr for io sq bram
			IO_PRINT("IO Read [%d]: %x\n\r", i,rd_data);
			sq_entry->dw[i]=rd_data;
		}
		if(g_io_sq_buf_rptr < (PL_SQ_ENTRY_NUM - 1))
			g_io_sq_buf_rptr += 1;
		else
			g_io_sq_buf_rptr = 0;
		return TRUE;
	} else{ // have no new SQ entry
		return FALSE;
	}
}

// Write CQ entry to PL-side buffer
// return TRUE/FALSE
u32 nvme_write_cq_entry(nvme_cq_entry_t* cq_entry)
{
	u32 cq_buf_rptr = get_acq_buf_rptr();
	u32 i;
    AD_PRINT("cq_buf_rptr  : %x\n\r", cq_buf_rptr);
    AD_PRINT("g_cq_buf_wptr: %x\n\r", g_cq_buf_wptr);
	if((cq_buf_rptr == (g_cq_buf_wptr + 1)) || ((cq_buf_rptr == 0) && (g_cq_buf_wptr == PL_CQ_ENTRY_NUM - 1))){
		return FALSE;
	} else{
		//mem_write((PL_CQ_ENTRY_BUF_BASEADDR + g_cq_buf_wptr * NVME_CQ_ENTRY_SIZE), (void *)cq_entry, NVME_CQ_ENTRY_SIZE);
		for(i=0;i<4;i++)
		{
			AD_PRINT("ADMIN Write [%d]: %x\n\r", i,cq_entry->dw[i]);
			Xil_Out32(PL_CQ_ENTRY_BUF_BASEADDR + g_cq_buf_wptr * NVME_CQ_ENTRY_SIZE+i*4, cq_entry->dw[i]);
		}

		if(g_cq_buf_wptr < (PL_CQ_ENTRY_NUM - 1))
			g_cq_buf_wptr += 1;
		else
			g_cq_buf_wptr = 0;
		return TRUE;
	}
}


u32 nvme_write_io_cq_entry(nvme_cq_entry_t* cq_entry)
{
	u32 cq_buf_rptr = get_io0_cq_buf_rptr();
	u32 i;
	IO_PRINT("cq_buf_rptr: %x\n\r", cq_buf_rptr);
	if((cq_buf_rptr == (g_io_cq_buf_wptr + 1)) || ((cq_buf_rptr == 0) && (g_io_cq_buf_wptr == PL_CQ_ENTRY_NUM - 1))){
		return FALSE;
	} else{
		//mem_write((PL_CQ_ENTRY_BUF_BASEADDR + g_cq_buf_wptr * NVME_CQ_ENTRY_SIZE), (void *)cq_entry, NVME_CQ_ENTRY_SIZE);
		for(i=0;i<4;i++)
		{
			IO_PRINT("IO Write [%d]: %x\n\r", i,cq_entry->dw[i]);
			Xil_Out32(PL_CQ_ENTRY_BUF_BASEADDR + g_io_cq_buf_wptr * NVME_CQ_ENTRY_SIZE+i*4+1024, cq_entry->dw[i]);
		}

		if(g_io_cq_buf_wptr < (PL_CQ_ENTRY_NUM - 1))
			g_io_cq_buf_wptr += 1;
		else
			g_io_cq_buf_wptr = 0;
		return TRUE;
	}
}

// Read data that host provides from SQ data buffer
// return TRUE/FALSE
u32 nvme_read_sq_data(u64 dsc_base_addr, u32* buf, u32 size)
{
	if(size <= PL_SQ_DATA_BUF_SIZE){
		write_h2c_dsc(dsc_base_addr, PL_SQ_DATA_BUF_BASEADDR,size);
		while((get_h2c_dma_status() & 0x1) == 0); // data not ready in buffer
		mem_read(PL_SQ_DATA_BUF_BASEADDR, (void *)buf, size);
		return TRUE;
	} else {
		return FALSE;
	}
}


// Write data that host requests to CQ data buffer
// return TRUE/FALSE
u32 nvme_write_cq_data(u64 dsc_base_addr, u32* buf, u32 size)
{
	if(size <= PL_CQ_DATA_BUF_SIZE){
		mem_write(PL_CQ_DATA_BUF_BASEADDR, (void *)buf, size);
		write_c2h_dsc(dsc_base_addr, PL_CQ_DATA_BUF_BASEADDR,size);
		while(((get_c2h_dma_status()) & 0x1) == 0); // data not transferred to Host
		return TRUE;
		AD_PRINT("write cq data ture!\n\r");
	} else {
		return FALSE;
		AD_PRINT("write cq data false!\n\r");
	}
}


//// Batch mode
//u32 nvme_read_sq_entry_batch(u64 dsc_base_addr, nvme_sq_entry_t** sq_entry, u32 batch_num)
//{
//	write_h2c_dsc(dsc_base_addr, NVME_SQ_ENTRY_SIZE*batch_num);
//	mb_axis_error_clr(); // CLEAR ERROR in Microblaze MSR register
//	for(u32 i = 0; i < batch_num; i++){
//		if(i+1 < batch_num){
//			mb_axis_rx((u32*)(sq_entry[i]), NVME_SQ_ENTRY_SIZE/4);
//		} else {
//			if(mb_axis_rx_chk_last((u32*)(sq_entry[i]), NVME_SQ_ENTRY_SIZE/4) == TRUE){
//				return TRUE;
//			} else{
//				return FALSE;
//			}
//		}
//	}
//	return TRUE;
//}
//
//
//u32 nvme_write_cq_entry_batch(u64 dsc_base_addr, nvme_cq_entry_t** cq_entry, u32 batch_num)
//{
//	write_c2h_dsc(dsc_base_addr, NVME_CQ_ENTRY_SIZE*batch_num);
////	mb_axis_error_clr(); // CLEAR ERROR in Microblaze MSR register
//	for(u32 i = 0; i < batch_num; i++){
//		if(i+1 < batch_num){
//			mb_axis_tx((u32*)(cq_entry[i]), NVME_CQ_ENTRY_SIZE/4);
//		} else {
//			mb_axis_tx_with_last((u32*)(cq_entry[i]), NVME_CQ_ENTRY_SIZE/4);
//		}
//	}
//	return TRUE;
//}
