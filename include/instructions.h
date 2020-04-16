#pragma once



enum
{
	INST_ADDH   = 0x54u, // Add halfword, bit 1 ra, bits 8-15 imm[7:0], bits 16-23 imm[15:8]
	INST_ADNH   = 0x5Cu, // Add negative halfword, bit 1 ra, bits 8-15 imm[7:0], bits 16-23 imm[15:8]
	INST_END    = 0x00u, // End
	INST_FLUSHP = 0x35u, // Flush and notify periphal, bits 11-15 periph[4:0]
	INST_GO     = 0xA0u, // Go, bit 1 ns, bits 8-10 cn[2:0], bits 16-47 imm[31:0]
	INST_KILL   = 0x01u, // Kill
	INST_LD     = 0x04u, // Load, bit 0 x, bit 1 bs
	INST_LDP    = 0x25u, // Load and notify periphal, bit 1 bs, bits 11-15 periph[4:0]
	INST_LP     = 0x20u, // Loop, bit 1 lc, bits 8-15 iter[7:0]
	INST_LPEND  = 0x28u, // Loop end, bit 0 x, bit 1 bs, bit 2 lc, bit 4 nf, bits 8-15 backwards_jump[7:0]
	//INST_LPFE          // Loop forever, pseudo instruction
	INST_MOV    = 0xBCu, // Move, bits 8-10 rd[2:0], bits 16-47 imm[31:0]
	INST_NOP    = 0x18u, // No operation
	INST_RMB    = 0x12u, // Read memory barrier
	INST_SEV    = 0x34u, // Send event, bits 11-15 event_num[4:0]
	INST_ST     = 0x08u, // Store, bit 0 x, bit 1 bs
	INST_STP    = 0x29u, // Store and notify periphal, bit 1 bs, bits 11-15 periph[4:0]
	INST_STZ    = 0x0Cu, // Store zero
	INST_WFE    = 0x36u, // Wait for event, bit 9 i, bits 11-15 event_num[4:0]
	INST_WFP    = 0x30u, // Wait for periphal, bit 0 p, bit 1 bs, bits 11-15 peripheral[4:0]
	INST_WMB    = 0x13u, // Write memory barrier
};


// Instruction bits.
#define INST_BIT_ADD_DAR            (1u<<1) // ra
#define INST_BIT_GO_NON_SEC         (1u<<1) // ns
#define INST_BIT_COND               (1u)    // x
#define INST_BIT_BURST              (1u<<1) // bs
#define INST_BIT_LP_LC1             (1u<<1) // lc (DMALP)
#define INST_BIT_LPEND_LC1          (1u<<2) // lc (DMALPEND)
#define INST_BIT_LPEND_NOT_FOREVER  (1u<<4) // nf
#define INST_BIT_WFE_INVAL          (1u<<9) // i
#define INST_BIT_WFP_PERIPH         (1u)    // p

// Instruction specific shifts/masks.
#define INST_ADD_IMM_SHIFT          (8u)
#define INST_GO_CN_SHIFT            (8u)
#define INST_GO_CN_MASK             (7u)
#define INST_GO_IMM_SHIFT           (16u)
#define INST_LP_ITER_SHIFT          (8u)
#define INST_LPEND_BACK_JMP_SHIFT   (8u)
#define INST_MOV_RD_SHIFT           (8u)
#define INST_MOV_IMM_SHIFT          (16u)

// Universal shifts/masks.
#define INST_PERIPH_SHIFT           (11u)
#define INST_PERIPH_MASK            (0x1Fu)
#define INST_EVENT_SHIFT            (11u)
#define INST_EVENT_MASK             (0x1Fu)

// CCR register.
#define CCR_DEFAULT_VAL             (1u<<CCR_DST_INC_SHIFT | 1u<<CCR_SRC_INC_SHIFT)
#define CCR_SRC_INC_SHIFT           (0u)
#define CCR_SRC_BURST_SIZE_SHIFT    (1u)
#define CCR_SRC_BURST_LEN_SHIFT     (4u)
#define CCR_SRC_PROT_CTRL_SHIFT     (8u)
#define CCR_SRC_CACHE_CTRL_SHIFT    (11u)
#define CCR_DST_INC_SHIFT           (14u)
#define CCR_DST_BURST_SIZE_SHIFT    (15u)
#define CCR_DST_BURST_LEN_SHIFT     (18u)
#define CCR_DST_PROT_CTRL_SHIFT     (22u)
#define CCR_DST_CACHE_CTRL_SHIFT    (25u)
#define CCR_ENDIAN_SWAP_SIZE_SHIFT  (28u)
