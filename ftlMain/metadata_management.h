#ifndef METADATA_MANAGEMENT_H_
#define METADATA_MANAGEMENT_H_

#include "ftl_config.h"
#include "ocssd_manage.h"
void InitBuildMeta(u32 base_addr, u32 chNo);
void InitMetaData();
void IntigrateMetaData();
int BackupMetaData();

#endif /* METADATA_MANAGEMENT_H_ */
