#ifndef METADATA_MANAGEMENT_H_
#define METADATA_MANAGEMENT_H_

#include "ftl_config.h"
#include "ocssd_manage.h"
#include "memory_map.h"
#include "../nvme/debug.h"
void InitBuildMeta(u32 base_addr,u32 ddr_addr, u32 chNo);
void InitMetaData();
void IntigrateMetaData();
int  BackupMetaData();
void MaintainMetaData(u32 lba, u32 opCode);
int CheckMetaData(u32 lba, u32 opCode, nvme_cq_entry_t *cq_entry);
int ForceUpdateMetadata(u64* lba_list, CHUNK_DESCRIPTOR* chunk_descriptor, u32 nlb);
void EraseAll();
#endif /* METADATA_MANAGEMENT_H_ */
