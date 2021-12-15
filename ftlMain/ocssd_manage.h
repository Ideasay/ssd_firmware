#ifndef OCSSD_MANAGE_H_
#define OCSSD_MANAGE_H_

#include "ftl_config.h"
#include "../nvme/nvme_structs.h"

//OCSSD Cmd
typedef union _OC_CMD_T
{
    u8 opcode;
    struct 
    {
        u8 dataTransfer :2;
        u8 function     :5;
        u8 generic      :1;  
    };  
}OC_CMD;

/*****************************************************
 *              admin cmd begin
 * **************************************************/
// the following cmd should replace some cmds in NVME_ADMIN_OPCODE_E
/*typedef enum _OC_ADMIN_OPCODE_E
{
    OC_GEOMETRY             = 0xE2,
    OC_GET_LOG_PAGE         = 0x02,
    OC_SET_FEATURE          = 0x09,
    OC_GET_FEATURE          = 0x0A,
}OC_ADMIN_OPCODE;*/
// add above to NVME_ADMIN_OPCODE_E(nvme_struct.h)

//reserved for status field

//reserved for status code

/**********device geometry data structure************/
typedef union _GEOMETRY_STRUCTURE_T
{
    u8 bytes[4096];
    struct 
    {
        u8 MJR;//byte 0: 1->1.2v;2->2.0v;
        u8 MNR;//byte 1: Minor Version Number
        u8 reserved1[6];

        union//byte 15:8
        {
            u64 LBAF;
            struct 
            {
                u8  GBL;//group bit length
                u8  PUBL;//pu bit length
                u8  CBL;//chunk bit length
                u8  LBBL;//logic block bit length
                u32 reserved10;
            };   
        };

        union//byte 19:16
        {
            u32 MCCAP;
            struct 
            {
                u32 vectorChunkCopyEnable       :1;
                u32 multiResetChunkEnable       :1;
                u32 reserved2                   :30;
            };   
        };

        u8 reserved3[12];//byte 31:20
        u8  WIT;//byte 32
        u8  reserved4[31];//byte 63:33

        /***************************
         *   Geometry Related
         ***************************/
        u16 NUM_GRP;//byte 65:64
        u16 NUM_PU;//byte 67:66
        u32 NUM_CHK;//byte 71:68;
        u32 CLBA;
        u8  reserved5[55];

        /***************************
         * write data requirements
         * ************************/
        u32 WS_MIN;//byte 131:128 NLB minimal per cmd within a chunk
        u32 WS_OPT;//byte 135:132
        u32 MW_CUNITS;//byte 139:136;
        //0->all logicBlock before wp could be read
        //1->the host must buffer all logicBlock before wp 
        u32 MAXOC;//byte 143:140
        u32 MAXOCPU;//byte 147:144
        u8 reserved6[44];

        /*****************************
         * performace related metrics
         * ***************************/
        u32 TRDT;
        u32 TRDM;
        u32 TWRT;
        u32 TWRM;
        u32 TCRST;
        u32 TCRSM;
        u8  reserved7[40];

        /****************************
         * reserved8
         * *************************/
        u8 reserved8[2816];

        /****************************
         * vendor specific
         * **************************/
        u8  vendorSpecific[1024];
    }; 
}GEOMETRY_STRUCTURE, *P_GEOMETRY_STRUCTURE;

/**************get log page*******************/

//log id has been added to NVME_LOG_PAGE_ID_E(nvme_struct.h)
//CAh chunk infomation

//reserved for chunk info header. I think it should be organized by list but not struct
typedef struct _CHUNK_DESCRIPTOR_LIST_T
{
    unsigned int headDescpt;
    unsigned int tailDescpt;
    unsigned int desCnt;
}CHUNK_DESCRIPTOR_LIST, *P_CHUNK_DESCRIPTOR_LIST;

typedef struct _CHUNK_DESCRIPTOR_ENTRY_T
{
    CHUNK_DESCRIPTOR chunkDescpt;
    unsigned int preChunkDescpt;
    unsigned int nextChunkDescpt;
    unsigned int groupId;
    unsigned int puId;
    unsigned int chunkId;
}CHUNK_DESCRIPTOR_ENTRY;

//chunk description table
typedef union _CHUNK_DESCRIPTOR_T
{
    u8 bytes[32];
    struct 
    {
        union//byte 0
        {
            u8 CS;//chunk state
            struct 
            {
                u8 chunkFree    :1;
                u8 chunkClosed  :1;
                u8 chunkOpen    :1;
                u8 chunkOffline :1;
                u8 reserved1    :4;
            };    
        };

        union//byte 1
        {
            u8 CT;//chunk type
            struct 
            {
                u8 wrSeq        :1;
                u8 wrRandom     :1;
                u8 reserved2    :2;
                u8 chunkDeviate :1;//if 1,CNLB specific the nlb of this chunk
                u8 reserved2    :3;
            };    
        };

        u8 WLI;//byte2 Wear-Level Index
        //0----------------------->255
        //chunk life begin-------->chunk life end

        u8 reserved3[5];
        u64 SLBA;//start LBA of this chunk
        u64 CNLB;//chunk num of logic block
        u64 WP;//writer pointer;  
    };    
}CHUNK_DESCRIPTOR ,*P_CHUNK_DESCRIPTOR;

/**************feature specific infomation*******************/
//feature id has been add to NVME_FEATURE_ID_E(nvme_struct.h)
//ERROR RECOVERY(05h)  MEDIA FEEDBACK(cah)

typedef union _OC_MEDIA_FEEDBACK_DW11_T //option
{
	u32	dw;

	struct
	{
		u32 HECC    :1;
        u32 VHECC   :1;
        u32 reserved:32;
	};
} OC_MEDIA_FEEDBACK_DW11;
/*****************************************************
 *              admin cmd end
 * **************************************************/

/*****************************************************
 *              io cmd begin
 * **************************************************/

//oc io cmd has been added into  NVME_NVM_OPCODE_E(nvme_structs.h)

//vector DW struct
typedef union _OC_VECTOR_DW10_t
{
    u32 dw;
    u32 LBAL_L;
}OC_VECTOR_DW10;

typedef union _OC_VECTOR_DW11_t
{
    u32 dw;
    u32 LBA_H;
}OC_VECTOR_DW11;

typedef union _OC_VECTOR_DW12_t
{
    u32 dw;
    struct 
    {
        u32 NLB         :6;
        u32 reserved    :26;
    };    
}OC_VECTOR_DW12;

typedef union _OC_VECTOR_DW0_t
{
    u32 dw;
    u32 CS_L;//completion status
}OC_VECTOR_DW10;

typedef union _OC_VECTOR_DW1_t
{
    u32 dw;
    u32 CS_H;//completion status
}OC_VECTOR_DW11;

//vector chunk reset cmd and response



/*****************************************************
 *              io cmd end
 * **************************************************/

#endif
