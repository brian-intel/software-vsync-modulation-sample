/*
 * Copyright © 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _C10_H
#define _C10_H

#include "common.h"

#define _DDI_CLK_VALFREQ_A                          0x64030
#define _DDI_CLK_VALFREQ_B                          0x64130
#define _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_A           0x64040
#define _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_B           0x64140
#define _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_USBC1       0x16F240
#define _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_USBC2       0x16F440
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))

enum port {
	PORT_NONE = -1,

	PORT_A = 0,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_E,
	PORT_F,
	PORT_G,
	PORT_H,
	PORT_I,

	/* tgl+ */
	PORT_TC1 = PORT_D,
	PORT_TC2,
	PORT_TC3,
	PORT_TC4,
	PORT_TC5,
	PORT_TC6,

	/* XE_LPD repositions D/E offsets and bitfields */
	PORT_D_XELPD = PORT_TC5,
	PORT_E_XELPD,

	I915_MAX_PORTS
};


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int  u32;

#define U32_MAX		((u32)~0U)
/**
 * is_power_of_2() - check if a value is a power of two
 * @n: the value to check
 *
 * Determine whether some value is a power of two, where zero is
 * *not* considered a power of two.
 * Return: true if @n is a power of 2, otherwise false.
 */
static inline __attribute__((const))
bool is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}
#define IS_POWER_OF_2 is_power_of_2
#define __is_constexpr(x) \
		(sizeof(int) == sizeof(*(int *)(8 ? ((void *)((long)(x) * 0l)) : (int *)8)))

#define MB_WRITE_COMMITTED      true
#define MB_WRITE_UNCOMMITTED    false

/*
 * Like _PICK_EVEN(), but supports 2 ranges of evenly spaced address offsets.
 * @__c_index corresponds to the index in which the second range starts to be
 * used. Using math interval notation, the first range is used for indexes [ 0,
 * @__c_index), while the second range is used for [ @__c_index, ... ). Example:
 *
 * #define _FOO_A           0xf000
 * #define _FOO_B           0xf004
 * #define _FOO_C           0xf008
 * #define _SUPER_FOO_A         0xa000
 * #define _SUPER_FOO_B         0xa100
 * #define FOO(x)           _MMIO(_PICK_EVEN_2RANGES(x, 3,      \
 *                        _FOO_A, _FOO_B,           \
 *                        _SUPER_FOO_A, _SUPER_FOO_B))
 *
 * This expands to:
 *  0: 0xf000,
 *  1: 0xf004,
 *  2: 0xf008,
 *  3: 0xa000,
 *  4: 0xa100,
 *  5: 0xa200,
 *  ...
 */
#define _PICK_EVEN_2RANGES(__index, __c_index, __a, __b, __c, __d)      \
	(((__index) < (__c_index) ? _PICK_EVEN(__index, __a, __b) :     \
				   _PICK_EVEN((__index) - (__c_index), __c, __d)))


#define XELPDP_PORT_M2P_MSGBUS_CTL(port, lane)      (_PICK_EVEN_2RANGES(port, PORT_TC1, \
			_XELPDP_PORT_M2P_MSGBUS_CTL_LN0_A, \
			_XELPDP_PORT_M2P_MSGBUS_CTL_LN0_B, \
			_XELPDP_PORT_M2P_MSGBUS_CTL_LN0_USBC1, \
			_XELPDP_PORT_M2P_MSGBUS_CTL_LN0_USBC2) + (lane) * 4)
#define PORT_CLOCK_CTL(phy_num)       (_XELPDP_PORT_CLOCK_CTL_A + phy_num * 0x100)
#define PORT_M2P_MSGBUS_CTL(phy_num)  (_XELPDP_PORT_M2P_MSGBUS_CTL_LN0_A + phy_num * 0x100)
#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define _XELPDP_PORT_MSGBUS_TIMER_LN0_A         0x640d8
#define _XELPDP_PORT_MSGBUS_TIMER_LN0_B         0x641d8
#define _XELPDP_PORT_MSGBUS_TIMER_LN0_USBC1     0x16f258
#define _XELPDP_PORT_MSGBUS_TIMER_LN0_USBC2     0x16f458
#define _XELPDP_PORT_MSGBUS_TIMER(port, lane)   (_PICK_EVEN_2RANGES(port, PORT_TC1, \
										 _XELPDP_PORT_MSGBUS_TIMER_LN0_A, \
										 _XELPDP_PORT_MSGBUS_TIMER_LN0_B, \
										 _XELPDP_PORT_MSGBUS_TIMER_LN0_USBC1, \
										 _XELPDP_PORT_MSGBUS_TIMER_LN0_USBC2) + (lane) * 4)
#define XELPDP_PORT_MSGBUS_TIMER(port, lane)                \
	 _XELPDP_PORT_MSGBUS_TIMER(port, lane)
#define XELPDP_PORT_MSGBUS_TIMER_VAL_MASK     REG_GENMASK(23, 0)
#define XELPDP_PORT_MSGBUS_TIMER_VAL          REG_FIELD_PREP(XELPDP_PORT_MSGBUS_TIMER_VAL_MASK, 0xa000)


/**
 * REG_FIELD_PREP() - Prepare a u32 bitfield value
 * @__mask: shifted mask defining the field's length and position
 * @__val: value to put in the field
 *
 * Local copy of FIELD_PREP() to generate an integer constant expression, force
 * u32 and for consistency with REG_FIELD_GET(), REG_BIT() and REG_GENMASK().
 *
 * @return: @__val masked and shifted into the field defined by @__mask.
 */
#define REG_FIELD_PREP(__mask, __val)                       \
	((u32)(((typeof(__mask))(__val) << __bf_shf(__mask)) & (__mask)))


/**
 * REG_FIELD_PREP8() - Prepare a u8 bitfield value
 * @__mask: shifted mask defining the field's length and position
 * @__val: value to put in the field
 *
 * Local copy of FIELD_PREP() to generate an integer constant expression, force
 * u8 and for consistency with REG_FIELD_GET8(), REG_BIT8() and REG_GENMASK8().
 *
 * @return: @__val masked and shifted into the field defined by @__mask.
 */
#define REG_FIELD_PREP8(__mask, __val)                                          \
	((u8)((((typeof(__mask))(__val) << __bf_shf(__mask)) & (__mask)))


/**
 * FIELD_GET() - extract a bitfield element
 * @_mask: shifted mask defining the field's length and position
 * @_reg:  value of entire bitfield
 *
 * FIELD_GET() extracts the field specified by @_mask from the
 * bitfield passed in as @_reg by masking and shifting it down.
 */
#define FIELD_GET(_mask, _reg)                      \
	({                              \
		(typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask)); \
	})


/**
 * REG_FIELD_GET() - Extract a u32 bitfield value
 * @__mask: shifted mask defining the field's length and position
 * @__val: value to extract the bitfield value from
 *
 * Local wrapper for FIELD_GET() to force u32 and for consistency with
 * REG_FIELD_PREP(), REG_BIT() and REG_GENMASK().
 *
 * @return: Masked and shifted value of the field defined by @__mask in @__val.
 */
#define REG_FIELD_GET(__mask, __val)    ((u32)FIELD_GET(__mask, __val))
#define REG_FIELD_GET8(__mask, __val)   ((u8)FIELD_GET(__mask, __val))

#define XELPDP_PORT_M2P_COMMAND_TYPE_MASK          GENMASK(30, 27)
#define XELPDP_PORT_M2P_ADDRESS_MASK               GENMASK(11, 0)
#define XELPDP_PORT_M2P_COMMAND_READ               REG_FIELD_PREP(XELPDP_PORT_M2P_COMMAND_TYPE_MASK, 0x3)
#define XELPDP_PORT_M2P_ADDRESS(val)               REG_FIELD_PREP(XELPDP_PORT_M2P_ADDRESS_MASK, val)
#define INTEL_CX0_LANE0                            BIT(0)
#define INTEL_CX0_LANE1                            BIT(1)
#define INTEL_CX0_BOTH_LANES                       (INTEL_CX0_LANE1 | INTEL_CX0_LANE0)
#define XELPDP_SSC_ENABLE_PLLB                     BIT(0)
#define XELPDP_SSC_ENABLE_PLLA                     BIT(1)
#define XELPDP_LANE1_PHY_CLOCK_SELECT              BIT(8)
#define XELPDP_FORWARD_CLOCK_UNGATE                BIT(10)
#define XELPDP_PORT_M2P_TRANSACTION_RESET          BIT(15)
#define XELPDP_PORT_P2M_ERROR_SET                  BIT(15)
#define XELPDP_PORT_REVERSAL                       BIT(16)
#define XELPDP_PORT_BUF_SOC_PHY_READY              BIT(24)
#define XELPDP_PORT_P2M_RESPONSE_READY             BIT(31)
#define XELPDP_PORT_M2P_TRANSACTION_PENDING        BIT(31)
#define XELPDP_PORT_BUF_SOC_READY_TIMEOUT_US       100
#define XELPDP_PORT_RESET_START_TIMEOUT_US         5
#define XELPDP_REFCLK_ENABLE_TIMEOUT_US            1
#define CX0_P2_STATE_RESET                         0x2
#define _XELPDP_PORT_BUF_CTL1_LN0_A                0x64004
#define _XELPDP_PORT_BUF_CTL1_LN0_B                0x64104
#define _XELPDP_PORT_BUF_CTL1_LN0_USBC1            0x16F200
#define _XELPDP_PORT_BUF_CTL1_LN0_USBC2            0x16F400
#define XELPDP_PORT_BUF_CTL1(port)                 _PICK_EVEN_2RANGES(port, PORT_TC1, \
												   _XELPDP_PORT_BUF_CTL1_LN0_A, \
												   _XELPDP_PORT_BUF_CTL1_LN0_B, \
												   _XELPDP_PORT_BUF_CTL1_LN0_USBC1, \
												   _XELPDP_PORT_BUF_CTL1_LN0_USBC2)
#define XELPDP_PORT_BUF_CTL2(port)                 (_PICK_EVEN_2RANGES(port, PORT_TC1, \
												   _XELPDP_PORT_BUF_CTL1_LN0_A, \
												   _XELPDP_PORT_BUF_CTL1_LN0_B, \
												   _XELPDP_PORT_BUF_CTL1_LN0_USBC1, \
												   _XELPDP_PORT_BUF_CTL1_LN0_USBC2) + 4)
#define XELPDP_PORT_BUF_CTL3(port)                 (_PICK_EVEN_2RANGES(port, PORT_TC1, \
												   _XELPDP_PORT_BUF_CTL1_LN0_A, \
												   _XELPDP_PORT_BUF_CTL1_LN0_B, \
												   _XELPDP_PORT_BUF_CTL1_LN0_USBC1, \
												   _XELPDP_PORT_BUF_CTL1_LN0_USBC2) + 8)
#define XELPDP_PORT_P2M_MSGBUS_STATUS(port, lane)  (_PICK_EVEN_2RANGES(port, PORT_TC1, \
												   _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_A, \
												   _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_B, \
												   _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_USBC1, \
												   _XELPDP_PORT_M2P_MSGBUS_CTL_LN0_USBC2) + (lane) * 4 + 8)
#define _XELPDP_PORT_CLOCK_CTL_A                   0x640E0
#define _XELPDP_PORT_CLOCK_CTL_B                   0x641E0
#define _XELPDP_PORT_CLOCK_CTL_USBC1               0x16F260
#define _XELPDP_PORT_CLOCK_CTL_USBC2               0x16F460
#define XELPDP_PORT_CLOCK_CTL(port)                _PICK_EVEN_2RANGES(port, PORT_TC1, \
												   _XELPDP_PORT_CLOCK_CTL_A, \
												   _XELPDP_PORT_CLOCK_CTL_B, \
												   _XELPDP_PORT_CLOCK_CTL_USBC1, \
												   _XELPDP_PORT_CLOCK_CTL_USBC2)
#define XELPDP_MSGBUS_TIMEOUT_SLOW                 1
#define XELPDP_DDI_CLOCK_SELECT_MAXPCLK            0x8
#define XELPDP_DDI_CLOCK_SELECT_DIV18CLK           0x9
#define XELPDP_DDI_CLOCK_SELECT(val)               REG_FIELD_PREP(XELPDP_DDI_CLOCK_SELECT_MASK, val)
#define XELPDP_PORT_M2P_COMMAND_WRITE_UNCOMMITTED  REG_FIELD_PREP(XELPDP_PORT_M2P_COMMAND_TYPE_MASK, 0x1)
#define XELPDP_PORT_M2P_COMMAND_WRITE_COMMITTED    REG_FIELD_PREP(XELPDP_PORT_M2P_COMMAND_TYPE_MASK, 0x2)
#define XELPDP_LANE_POWERDOWN_UPDATE(lane)         _PICK(lane, BIT(25), BIT(24))
#define XELPDP_LANE_PHY_CURRENT_STATUS(lane)       _PICK(lane, BIT(29), BIT(28))
#define XELPDP_LANE_PIPE_RESET(lane)               _PICK(lane, BIT(31), BIT(30))
#define XELPDP_LANE_PCLK_REFCLK_REQUEST(lane)      BIT(29 - ((lane) * 4))
#define XELPDP_LANE_PCLK_REFCLK_ACK(lane)          BIT(28 - ((lane) * 4))
#define XELPDP_LANE_POWERDOWN_NEW_STATE_MASK       GENMASK(3, 0)
#define XELPDP_POWER_STATE_ACTIVE_MASK             GENMASK(3, 0)
#define XELPDP_POWER_STATE_READY_MASK              GENMASK(7, 4)
#define XELPDP_PLL_LANE_STAGGERING_DELAY_MASK      GENMASK(15, 8)
#define XELPDP_DDI_CLOCK_SELECT_MASK               GENMASK(15, 12)
#define _XELPDP_LANE1_POWERDOWN_NEW_STATE_MASK     GENMASK(19, 16)
#define XELPDP_PORT_M2P_DATA_MASK                  GENMASK(23, 16)
#define XELPDP_PORT_P2M_DATA_MASK                  GENMASK(23, 16)
#define _XELPDP_LANE0_POWERDOWN_NEW_STATE_MASK     GENMASK(23, 20)
#define XELPDP_PORT_P2M_COMMAND_TYPE_MASK          GENMASK(30, 27)
#define _XELPDP_LANE0_POWERDOWN_NEW_STATE(val)     REG_FIELD_PREP(_XELPDP_LANE0_POWERDOWN_NEW_STATE_MASK, val)
#define _XELPDP_LANE1_POWERDOWN_NEW_STATE(val)     REG_FIELD_PREP(_XELPDP_LANE1_POWERDOWN_NEW_STATE_MASK, val)
#define XELPDP_POWER_STATE_ACTIVE(val)             REG_FIELD_PREP(XELPDP_POWER_STATE_ACTIVE_MASK, val)
#define XELPDP_PLL_LANE_STAGGERING_DELAY(val)      REG_FIELD_PREP(XELPDP_PLL_LANE_STAGGERING_DELAY_MASK, val)
#define XELPDP_LANE_POWERDOWN_NEW_STATE(lane, val) _PICK(lane, \
												   _XELPDP_LANE0_POWERDOWN_NEW_STATE(val), \
												   _XELPDP_LANE1_POWERDOWN_NEW_STATE(val))
#define XELPDP_POWER_STATE_READY(val)              REG_FIELD_PREP(XELPDP_POWER_STATE_READY_MASK, val)
#define CX0_P0_STATE_ACTIVE                        0x0
#define CX0_P2_STATE_READY                         0x2
#define CX0_P2PG_STATE_DISABLE                     0x9
#define CX0_P4PG_STATE_DISABLE                     0xC
#define CX0_P2_STATE_RESET                         0x2
#define XELPDP_PORT_RESET_END_TIMEOUT              15
#define XELPDP_PORT_P2M_COMMAND_READ_ACK           0x4
#define XELPDP_PORT_P2M_COMMAND_WRITE_ACK          0x5
#define XELPDP_PORT_M2P_DATA(val)                  REG_FIELD_PREP(XELPDP_PORT_M2P_DATA_MASK, val)


/* C10 Vendor Registers */
#define PHY_C10_VDR_PLL(idx)                       (0xC00 + (idx))
#define C10_PLL0_FRACEN                            REG_BIT8(4)
#define C10_PLL3_MULTIPLIERH_MASK                  REG_GENMASK8(3, 0)
#define C10_PLL15_TXCLKDIV_MASK                    REG_GENMASK8(2, 0)
#define C10_PLL15_HDMIDIV_MASK                     REG_GENMASK8(5, 3)
#define PHY_C10_VDR_CMN(idx)                       (0xC20 + (idx))
#define C10_CMN0_REF_RANGE                         REG_FIELD_PREP(REG_GENMASK(4, 0), 1)
#define C10_CMN0_REF_CLK_MPLLB_DIV                 REG_FIELD_PREP(REG_GENMASK(7, 5), 1)
#define C10_CMN3_TXVBOOST_MASK                     REG_GENMASK8(7, 5)
#define C10_CMN3_TXVBOOST(val)                     REG_FIELD_PREP8(C10_CMN3_TXVBOOST_MASK, val)
#define PHY_C10_VDR_TX(idx)                        (0xC30 + (idx))
#define C10_TX0_TX_MPLLB_SEL                       REG_BIT(4)
#define C10_TX1_TERMCTL_MASK                       REG_GENMASK8(7, 5)
#define C10_TX1_TERMCTL(val)                       REG_FIELD_PREP8(C10_TX1_TERMCTL_MASK, val)
#define PHY_C10_VDR_CONTROL(idx)                   (0xC70 + (idx) - 1)
#define C10_VDR_CTRL_MSGBUS_ACCESS                 REG_BIT8(2)
#define C10_VDR_CTRL_MASTER_LANE                   REG_BIT8(1)
#define C10_VDR_CTRL_UPDATE_CFG                    REG_BIT8(0)
#define PHY_C10_VDR_CUSTOM_WIDTH                   0xD02
#define C10_VDR_CUSTOM_WIDTH_MASK                  REG_GENMASK(1, 0)
#define C10_VDR_CUSTOM_WIDTH_8_10                  REG_FIELD_PREP(C10_VDR_CUSTOM_WIDTH_MASK, 0)
#define PHY_C10_VDR_OVRD                           0xD71
#define PHY_C10_VDR_OVRD_TX1                       REG_BIT8(0)
#define PHY_C10_VDR_OVRD_TX2                       REG_BIT8(2)
#define PHY_C10_VDR_PRE_OVRD_TX1                   0xD80
#define C10_PHY_OVRD_LEVEL_MASK                    REG_GENMASK8(5, 0)
#define C10_PHY_OVRD_LEVEL(val)                    REG_FIELD_PREP8(C10_PHY_OVRD_LEVEL_MASK, val)
#define PHY_CX0_VDROVRD_CTL(lane, tx, control)              \
					(PHY_C10_VDR_PRE_OVRD_TX1 + \
					 ((lane) ^ (tx)) * 0x10 + (control))
#define TRANS_DDI_MODE_SELECT_MASK                 (7 << 24)
#define TRANS_DDI_MODE_SELECT_HDMI                 (0 << 24)
#define TRANS_DDI_MODE_SELECT_DVI                  (1 << 24)
#define TRANS_DDI_MODE_SELECT_DP_SST               (2 << 24)
#define TRANS_DDI_MODE_SELECT_DP_MST               (3 << 24)


/* these are outputs from the chip - integrated only
   external chips are via DVO or SDVO output */
enum intel_output_type {
	INTEL_OUTPUT_UNUSED = 0,
	INTEL_OUTPUT_ANALOG = 1,
	INTEL_OUTPUT_DVO = 2,
	INTEL_OUTPUT_SDVO = 3,
	INTEL_OUTPUT_LVDS = 4,
	INTEL_OUTPUT_TVOUT = 5,
	INTEL_OUTPUT_HDMI = 6,
	INTEL_OUTPUT_DP = 7,
	INTEL_OUTPUT_EDP = 8,
	INTEL_OUTPUT_DSI = 9,
	INTEL_OUTPUT_DDI = 10,
	INTEL_OUTPUT_DP_MST = 11,
};

/*
 * Add the pseudo keyword 'fallthrough' so case statement blocks
 * must end with any of these keywords:
 *   break;
 *   fallthrough;
 *   continue;
 *   goto <label>;
 *   return [expression];
 *
 *  gcc: https://gcc.gnu.org/onlinedocs/gcc/Statement-Attributes.html#Statement-Attributes
 */
#if __has_attribute(__fallthrough__)
# define fallthrough                    __attribute__((__fallthrough__))
#else
# define fallthrough                    do {} while (0)  /* fallthrough */
#endif

#define for_each_if(condition) if (!(condition)) {} else

#define for_each_cx0_lane_in_mask(__lane_mask, __lane) \
	for ((__lane) = 0; (__lane) < 2; (__lane)++) \
		for_each_if((__lane_mask) & BIT(__lane))

#define do_div(n,base)  ({ \
	int __rem = n % base;  \
	n /= base;             \
	__rem;                 \
})

#define DIV_ROUND_CLOSEST_ULL(ll, d) ({ \
	unsigned long long _tmp = (ll)+(d)/2; do_div(_tmp, d); _tmp; \
})

#define DIV_ROUND_CLOSEST(x, divisor)({		\
	typeof(x) __x = x;				\
	typeof(divisor) __d = divisor;			\
	(((typeof(x))-1) > 0 ||				\
	 ((typeof(divisor))-1) > 0 || (__x) > 0) ?	\
		(((__x) + ((__d) / 2)) / (__d)) :	\
		(((__x) - ((__d) / 2)) / (__d));	\
})

#define C10_PLL_REG_COUNT 20
#define C10_PLL_REG_QUOT_LOW  11
#define C10_PLL_REG_QUOT_HIGH 12

#define C10_PLL_REG_DEN_LOW  9
#define C10_PLL_REG_DEN_HIGH 10

#define C10_PLL_REG_REM_LOW  13
#define C10_PLL_REG_REM_HIGH 14

#define C10_PLL_REG_MULTIPLIER  3
#define C10_PLL_REG_TXCLKDIV    15

typedef struct _c10_phy_reg {
	int done;
	u8 pll_state[C10_PLL_REG_COUNT];
	u8 pll_state_old[C10_PLL_REG_COUNT];
} c10_phy_reg;

class c10 : public phys {
public:
	c10(ddi_sel *ds);
	~c10() {};

	void program_mmio(c10_phy_reg *pr, int mod);
	static void reset_phy_regs(int sig, siginfo_t *si, void *uc);
	void reset_phy_regs(c10_phy_reg *dr);
	void program_phy(double time_diff, double shift);
	void wait_until_done();

private:
	c10_phy_reg c10_reg;


private:
	// Private Helper Methods
	int __intel_wait_for_register_fw(u32 reg, u32 mask, u32 value, u32 *out_value);
	int __intel_wait_for_register(u32 reg, u32 mask, u32 value, u32 *out_value);
	int intel_wait_for_register(u32 reg, u32 mask, u32 value, unsigned int timeout_ms);
	int __intel_de_wait_for_register(u32 reg, u32 mask, u32 value, u32 *out_value);
	int intel_de_wait_for_register(u32 reg, u32 mask, u32 value, unsigned int timeout);
	u32 intel_uncore_rmw(u32 reg, u32 clear, u32 set);
	u32 intel_de_rmw(u32 reg, u32 clear, u32 set);
	int intel_de_wait_for_clear(int reg, u32 mask, unsigned int timeout);
	u32 intel_de_read(u32 reg);
	void intel_de_write(u32 reg, u32 val);
	void intel_clear_response_ready_flag(u32 port, int lane);
	void intel_cx0_bus_reset(int port, int lane);
	int intel_cx0_wait_for_ack(u32 port, int command, int lane, u32 *val);
	int __intel_cx0_read_once(u32 port, int lane, u16 addr);
	u8 __intel_cx0_read(u32 port, int lane, u16 addr);
	int __intel_cx0_write_once(u32 port, int lane, u16 addr, u8 data, bool committed);
	void __intel_cx0_write(u32 port, int lane, u16 addr, u8 data, bool committed);
	void __intel_cx0_rmw(u32 port, int lane, u16 addr, u8 clear, u8 set, bool committed);
	void intel_cx0_rmw(u32 port, u8 lane_mask, u16 addr, u8 clear, u8 set, bool committed);
	void intel_cx0_write(u32 port, u8 lane_mask, u16 addr, u8 data, bool committed);


};

extern c10_phy_reg c10_table[];


#endif