//this file will include process_io_cmds function
#ifndef __NVME_IO_CMDS_H__
#define __NVME_IO_CMDS_H__

//admin use
/*#include "nvme_io_que_db.h"
#include "nvme_regs.h"
#include "nvme_transmitter.h"
#include "xil_printf.h"


u32 g_buf[1024];
#define BRAM_BUF_BASEADDR		(g_buf)*/
#include "nvme_transmitter.h"
#include "reg_rw.h"
#include "xil_printf.h"
#include "nvme_structs.h"
#include "hw_params.h"
#include "nvme_regs.h"
#include "debug.h"



//this header file is used for transform io_cmd to slices.
//#include "../request_transform.h"

//input should be (nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry) here
//refer to cosmos handle_nvme_io_cmd(NVME_COMMAND *nvmeCmd)
int process_io_cmd(nvme_sq_entry_t* sq_entry, nvme_cq_entry_t* cq_entry);

//refer to void handle_nvme_io_read
void handle_nvme_io_read(nvme_sq_entry_t *sq_entry, nvme_cq_entry_t *cq_entry);

//refer to void handle_nvme_io_write
void handle_nvme_io_write(nvme_sq_entry_t *sq_entry, nvme_cq_entry_t *cq_entry);

void handle_nvme_io_reset(nvme_sq_entry_t *sq_entry, nvme_cq_entry_t *cq_entry);

void ReqTransNvmeToSlice(unsigned int startLba, unsigned int nlb, unsigned int cmdCode, u64 prp1ForReq, u64 prp2ForReq, int prpNum, int start_offset, int data_length, nvme_cq_entry_t *cq_entry);




#endif 	//__NVME_IO_CMDS_H__
