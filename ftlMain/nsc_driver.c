#include "nsc_driver.h"


void select_way(uint32_t base_addr, uint32_t way)
{
	Xil_Out32(base_addr+rAddress, way);
	Xil_Out32(base_addr+rCommand, 0x00000020);	
}
void select_col(uint32_t base_addr, uint32_t col)
{
	Xil_Out32(base_addr+rAddress, col);
	Xil_Out32(base_addr+rCommand, 0x00000022);	
}
void select_row(uint32_t base_addr, uint32_t row)
{
	Xil_Out32(base_addr+rAddress, row);
	Xil_Out32(base_addr+rCommand, 0x00000024);	
}
void set_feature(uint32_t base_addr, uint32_t feature)
{
	Xil_Out32(base_addr+rAddress, feature);
	Xil_Out32(base_addr+rCommand, 0x00000028);	
}
void set_length(uint32_t base_addr, uint32_t length)
{
	Xil_Out32(base_addr+rLength, length);
}

void set_DelayTap(uint32_t base_addr, uint32_t DelayTap)
{
	Xil_Out32(base_addr+rDelayTap, DelayTap);
}
void reset_ffh(uint32_t base_addr, uint32_t way)
{
	select_way(base_addr,way);
	Xil_Out32(base_addr+rCommand, 0x00000001);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void setfeature_efh(uint32_t base_addr, uint32_t way, uint32_t feature)
{
	select_way(base_addr,way);
	set_feature(base_addr,feature);
	Xil_Out32(base_addr+rCommand, 0x00000002);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void getfeature_eeh(uint32_t base_addr, uint32_t way)
{
	select_way(base_addr,way);
	Xil_Out32(base_addr+rCommand, 0x00000005);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void readparameterpage(uint32_t base_addr, uint32_t way, uint32_t DMAWAddress)
{
	select_way(base_addr,way);
	set_length(base_addr,256);
	Xil_Out32(base_addr+rDMAWAddress, DMAWAddress);
	Xil_Out32(base_addr+rCommand, 0x00010004);
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void progpage_80h_10h(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMARAddress)
{
	select_way(base_addr,way);
	select_col(base_addr,col);
	select_row(base_addr,row);
	set_length(base_addr,length);
	Xil_Out32(base_addr+rDMARAddress, DMARAddress);
	Xil_Out32(base_addr+rCommand, 0x00000003);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void progpage_80h_15h_cache(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMARAddress)
{
	select_way(base_addr,way);
	select_col(base_addr,col);
	select_row(base_addr,row);
	set_length(base_addr,length);
	Xil_Out32(base_addr+rDMARAddress, DMARAddress);
	Xil_Out32(base_addr+rCommand, 0x00010003);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void progpage_80h_11h_multplane(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMARAddress)
{
	select_way(base_addr,way);
	select_col(base_addr,col);
	select_row(base_addr,row);
	set_length(base_addr,length);
	Xil_Out32(base_addr+rDMARAddress, DMARAddress);
	Xil_Out32(base_addr+rCommand, 0x00020003);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void readpage_00h_30h(uint32_t base_addr, uint32_t way, uint32_t col, uint32_t row, uint32_t length, uint32_t DMAWAddress)
{
	select_way(base_addr,way);
	select_col(base_addr,col);
	select_row(base_addr,row);
	set_length(base_addr,length);
	Xil_Out32(base_addr+rDMAWAddress, DMAWAddress);
	Xil_Out32(base_addr+rCommand, 0x00000004);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void eraseblock_60h_d0h(uint32_t base_addr, uint32_t way, uint32_t row)
{
	select_way(base_addr,way);
	select_row(base_addr,row);
	Xil_Out32(base_addr+rCommand, 0x00000006);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}
void eraseblock_60h_d1h_multiplane(uint32_t base_addr, uint32_t way, uint32_t row)
{
	select_way(base_addr,way);
	select_row(base_addr,row);
	Xil_Out32(base_addr+rCommand, 0x00020006);	
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));
}

uint8_t readstatus_70h(uint32_t base_addr, uint32_t way)
{
	select_way(base_addr,way);
	Xil_Out32(base_addr+rCommand, 0x00000007);	
	//usleep(1);
//	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
//	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));

	u8 status = ((Xil_In32(base_addr+rNFCStatus) & 0x0000ff00) >> 8);
	return status;
}
uint8_t readstatus_78h(uint32_t base_addr, uint32_t way, uint32_t row)
{
	select_way(base_addr,way);
	select_row(base_addr,row);
	Xil_Out32(base_addr+rCommand, 0x00010007);	
	usleep(1);
//	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000000));
//	while(((Xil_In32(base_addr+rNFCStatus) & 0x00000001) == 0x00000001));

	u8 status = ((Xil_In32(base_addr+rNFCStatus) & 0x0000ff00) >> 8);
	return status;
}
