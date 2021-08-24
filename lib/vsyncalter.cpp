#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <vsyncalter.h>
#include <debug.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "mmio.h"
#include <xf86drm.h>

#define TESTING                       0
#define SHIFT                         (0.1)
#define REF_DKL_FREQ                  38.4
#define REF_COMBO_FREQ                19.2
#define ONE_SEC_IN_NS                 (1000 * 1000 * 1000)
#define TV_NSEC(t)                    ((long) ((t * 1000000) % ONE_SEC_IN_NS))
#define TV_SEC(t)                     ((time_t) ((t * 1000000) / ONE_SEC_IN_NS))
#define TIME_IN_USEC(sec, usec)       (unsigned long) (1000000 * sec + usec)
#define BIT(nr)                       (1UL << (nr))
#define BITS_PER_LONG                 64
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define GETBITS_VAL(val, h, l)       ((val & GENMASK(h, l)) >> l)
#define MAX_PHYS                      6
#define PHY_BASE                      0x168000
#define PHY_NUM_BASE(phy_num)         (PHY_BASE + phy_num * 0x1000)
#define DKL_PLL_DIV0(phy_num)         (PHY_NUM_BASE(phy_num) + 0x200)
#define DKL_SSC(phy_num)              (PHY_NUM_BASE(phy_num) + 0x210)
#define DKL_BIAS(phy_num)             (PHY_NUM_BASE(phy_num) + 0x214)
#define DKL_VISA_SERIALIZER(phy_num)  (PHY_NUM_BASE(phy_num) + 0x220)
#define DKL_DCO(phy_num)              (PHY_NUM_BASE(phy_num) + 0x224)
#define ARRAY_SIZE(a)                 (int) (sizeof(a)/sizeof(a[0]))
#define READ_VAL(r, v)                combo_table[i].r.v = READ_OFFSET_DWORD(g_mmio, combo_table[i].r.addr);

typedef struct _reg {
	int addr;
	int orig_val;
	int mod_val;
} reg;

typedef struct _combo_phy_reg {
	reg cfgcr0;
	reg cfgcr1;
	int enabled;
	int done;
} combo_phy_reg;

typedef struct _dkl_phy_reg {
	reg dkl_pll_div0;
	reg dkl_visa_serializer;
	reg dkl_bias;
	reg dkl_ssc;
	reg dkl_dco;
	int enabled;
	int done;
} dkl_phy_reg;

static combo_phy_reg combo_table[] = {
	{{0x164284, 0, 0}, {0x16428C, 0, 0}, 0, 1},
};

static dkl_phy_reg dkl_table[] = {
	{{DKL_PLL_DIV0(0), 0, 0}, {DKL_VISA_SERIALIZER(0), 0, 0}, {DKL_BIAS(0), 0, 0}, {DKL_SSC(0), 0, 0}, {DKL_DCO(0), 0, 0}, 0, 1},
	{{DKL_PLL_DIV0(1), 0, 0}, {DKL_VISA_SERIALIZER(1), 0, 0}, {DKL_BIAS(1), 0, 0}, {DKL_SSC(1), 0, 0}, {DKL_DCO(1), 0, 0}, 0, 1},
	{{DKL_PLL_DIV0(2), 0, 0}, {DKL_VISA_SERIALIZER(2), 0, 0}, {DKL_BIAS(2), 0, 0}, {DKL_SSC(2), 0, 0}, {DKL_DCO(2), 0, 0}, 0, 1},
	{{DKL_PLL_DIV0(3), 0, 0}, {DKL_VISA_SERIALIZER(3), 0, 0}, {DKL_BIAS(3), 0, 0}, {DKL_SSC(3), 0, 0}, {DKL_DCO(3), 0, 0}, 0, 1},
	{{DKL_PLL_DIV0(4), 0, 0}, {DKL_VISA_SERIALIZER(4), 0, 0}, {DKL_BIAS(4), 0, 0}, {DKL_SSC(4), 0, 0}, {DKL_DCO(4), 0, 0}, 0, 1},
	{{DKL_PLL_DIV0(5), 0, 0}, {DKL_VISA_SERIALIZER(5), 0, 0}, {DKL_BIAS(5), 0, 0}, {DKL_SSC(5), 0, 0}, {DKL_DCO(5), 0, 0}, 0, 1},
};

enum {
	DKL,
	COMBO,
	TOTAL_PHYS,
};

class user_info {
	private:
	int phy_type;
	void *phy_reg;
	public:
	user_info(int t, void *r) { phy_type = t; phy_reg = r; }
	int get_type() { return phy_type; }
	void *get_reg() { return phy_reg; }
};

typedef struct _vbl_info {
	long *vsync_array;
	int size;
	int counter;
} vbl_info;


typedef int  (*find_func)();
typedef void (*program_func)(double time_diff);
typedef void (*check_func)();

int find_enabled_dkl_phys();
int find_enabled_combo_phys();
void program_dkl_phys(double time_diff);
void program_combo_phys(double time_diff);
void check_if_dkl_done();
void check_if_combo_done();

typedef struct _phy_funcs {
	char name[20];
	void *table;
	find_func find;
	program_func program;
	check_func check_if_done;
} phy_funcs;

phy_funcs phy[] = {
	{"DKL",   dkl_table,   find_enabled_dkl_phys,   program_dkl_phys,   check_if_dkl_done},
	{"COMBO", combo_table, find_enabled_combo_phys, program_combo_phys, check_if_combo_done},
};

int g_dev_fd = 0;

/*******************************************************************************
 * Description
 *  open_device - This function opens /dev/dri/card0
 * Parameters
 *	NONE
 * Return val
 *  int - >=0 == SUCCESS, <0 == FAILURE
 ******************************************************************************/
int open_device()
{
	return open("/dev/dri/card0", O_RDWR | O_CLOEXEC, 0);
}

/*******************************************************************************
 * Description
 *  close_device - This function closes an open handle
 * Parameters
 *	NONE
 * Return val
 *  void
 ******************************************************************************/
void close_device()
{
	close(g_dev_fd);
}

/*******************************************************************************
 * Description
 *  vsync_lib_init - This function initializes the library. It must be called
 *	ahead of all other functions because it opens device, maps MMIO space and
 *	initializes any key global variables.
 * Parameters
 *	NONE
 * Return val
 *  int - 0 == SUCCESS, 1 == FAILURE
 ******************************************************************************/
int vsync_lib_init()
{
	if(!IS_INIT()) {
		if(map_mmio()) {
			return 1;
		}
		g_init = 1;
	}

	return 0;
}

/*******************************************************************************
 * Description
 *  vsync_lib_uninit - This function uninitializes the library by closing devices
 *  and unmapping memory. It must be called at program exit or else we can have
 *  memory leaks in the program.
 * Parameters
 *	NONE
 * Return val
 *  void
 ******************************************************************************/
void vsync_lib_uninit()
{
	close_mmio_handle();
	g_init = 0;
}

/*******************************************************************************
 * Description
 *  calc_steps_to_sync - This function calculates how many steps we need to take
 *  in order to synchronize the primary and secondary systems given the delta
 *  between primary and secondary and the shift that we need to make in terms of
 *  percentage. Each steps is a single vsync period (typically 16.666 ms).
 * Parameters
 *	double time_diff - The time difference in between the two systems in ms.
 *	double shift - The percentage shift that we need to make in our vsyncs.
 * Return val
 *  int
 ******************************************************************************/
static inline int calc_steps_to_sync(double time_diff, double shift)
{
	return (int) ((time_diff * 100) / shift);
}

/*******************************************************************************
 * Description
 *  program_dkl_mmio - This function programs the DKL Phy MMIO registers needed
 *  to move a vsync period for a system.
 * Parameters
 *	dkl_phy_reg *pr - The data structure that holds all of the PHY registers that
 *	need to be programmed
 *	int mod - This parameter tells the function whether to program the original
 *	values or the modified ones. 0 = Original, 1 = modified
 * Return val
 *  void
 ******************************************************************************/
void program_dkl_mmio(dkl_phy_reg *pr, int mod)
{
#if !TESTING
	WRITE_OFFSET_DWORD(g_mmio, pr->dkl_pll_div0.addr,
			mod ? pr->dkl_pll_div0.mod_val : pr->dkl_pll_div0.orig_val);
	WRITE_OFFSET_DWORD(g_mmio, pr->dkl_visa_serializer.addr,
			mod ? pr->dkl_visa_serializer.mod_val : pr->dkl_visa_serializer.orig_val);
	WRITE_OFFSET_DWORD(g_mmio, pr->dkl_bias.addr,
			mod ? pr->dkl_bias.mod_val : pr->dkl_bias.orig_val);
	WRITE_OFFSET_DWORD(g_mmio, pr->dkl_ssc.addr,
			mod ? pr->dkl_ssc.mod_val : pr->dkl_ssc.orig_val);
	WRITE_OFFSET_DWORD(g_mmio, pr->dkl_dco.addr,
			mod ? pr->dkl_dco.mod_val  : pr->dkl_dco.orig_val);
#endif
}

/*******************************************************************************
 * Description
 *  program_combo_mmio - This function programs the Combo Phy MMIO registers
 *  needed to move avsync period for a system.
 * Parameters
 *	combo_phy_reg *pr - The data structure that holds all of the PHY registers
 *	that need to be programmed
 *	int mod - This parameter tells the function whether to program the original
 *	values or the modified ones. 0 = Original, 1 = modified
 * Return val
 *  void
 ******************************************************************************/
void program_combo_mmio(combo_phy_reg *pr, int mod)
{
#if !TESTING
	WRITE_OFFSET_DWORD(g_mmio, pr->cfgcr0.addr,
			mod ? pr->cfgcr0.mod_val : pr->cfgcr0.orig_val);
#endif
}

#if !TESTING
/*******************************************************************************
 * Description
 *	timer_handler - The timer callback function which gets executed whenever a
 *	timer expires. We program MMIO registers of the PHY in this function becase
 *	we have waited for a certain time period to get the primary and secondary
 *	systems vsync in sync and now it is time to reprogram the default values
 *	for the secondary system's PHYs.
 * Parameters
 *	int sig - The signal that fired
 *	siginfo_t *si - A pointer to a siginfo_t, which is a structure containing
    further information about the signal
 *	void *uc - This is a pointer to a ucontext_t structure, cast to void *.
    The structure pointed to by this field contains signal context information
	that was saved on the user-space stack by the kernel
 * Return val
 *  void
 ******************************************************************************/
static void timer_handler(int sig, siginfo_t *si, void *uc)
{
	user_info *ui = (user_info *) si->si_value.sival_ptr;
	if(!ui) {
		return;
	}

	PRINT("timer done\n");
	if(ui->get_type() == DKL) {
		dkl_phy_reg *dr = (dkl_phy_reg *) ui->get_reg();
		program_dkl_mmio(dr, 0);
		DBG("DEFAULT VALUES\n dkl_pll_div0 = 0x%X\n dkl_visa_serializer = 0x%X\n "
				"dkl_bias = 0x%X\n dkl_ssc = 0x%X\n dkl_dco = 0x%X\n",
				dr->dkl_pll_div0.orig_val,
				dr->dkl_visa_serializer.orig_val,
				dr->dkl_bias.orig_val,
				dr->dkl_ssc.orig_val,
				dr->dkl_dco.orig_val);
		dr->done = 1;
	} else {
		combo_phy_reg *cr = (combo_phy_reg *) ui->get_reg();
		program_combo_mmio(cr, 0);
		DBG("DEFAULT VALUES\n cfgcr0 = 0x%X\n cfgcr1 = 0x%X\n",
				cr->cfgcr0.orig_val, cr->cfgcr1.orig_val);
		cr->done = 1;
	}
	delete ui;
}

/*******************************************************************************
 * Description
 *	make_timer - This function creates a timer.
 * Parameters
 *	long expire_ms - The time period in ms after which the timer will fire.
 *	void *user_ptr - A pointer to pass to the timer handler
 * Return val
 *	int - 0 == SUCCESS, -1 = FAILURE
 ******************************************************************************/
static int make_timer(long expire_ms, void *user_ptr)
{
	struct sigevent         te;
	struct itimerspec       its;
	struct sigaction        sa;
	int                     sig_no = SIGRTMIN;
	timer_t                 timer_id;

	/* Set up signal handler. */
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timer_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig_no, &sa, NULL) == -1) {
		ERR("Failed to setup signal handling for timer.\n");
		return -1;
	}

	/* Set and enable alarm */
	te.sigev_notify = SIGEV_SIGNAL;
	te.sigev_signo = sig_no;
	te.sigev_value.sival_ptr = user_ptr;
	timer_create(CLOCK_REALTIME, &te, &timer_id);

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = TV_NSEC(expire_ms);
	its.it_value.tv_sec = TV_SEC(expire_ms);
	its.it_value.tv_nsec = TV_NSEC(expire_ms);
	timer_settime(timer_id, 0, &its, NULL);

	return 0;
}
#endif


/*******************************************************************************
 * Description
 *	find_enabled_dkl_phys - This function finds out which of the DKL phys are on
 *	It does this by querying the DKL_PLL_DIV0 register for all possible
 *	combinations
 * Parameters
 *	NONE
 * Return val
 *	int - The number of DKL phys that are enabled
 ******************************************************************************/
int find_enabled_dkl_phys()
{
	int enabled = 0;
#if TESTING
	enabled++;
	dkl_table[0].enabled = 1;
#else
	unsigned int val;
	for(int i = 0; i < ARRAY_SIZE(dkl_table); i++) {
		val = READ_OFFSET_DWORD(g_mmio, dkl_table[i].dkl_pll_div0.addr);
		if(val != 0 && val != 0xFFFFFFFF) {
			dkl_table[i].enabled = 1;
			enabled++;
			DBG("DKL phy #%d is on\n", i);
		}
		dkl_table[i].done = 1;
	}
#endif
	DBG("Total DKL phys on: %d\n", enabled);
	return enabled;
}

/*******************************************************************************
 * Description
 *	find_enabled_combo_phys - This function finds out which of the Combo phys are
 *	on.
 * Parameters
 *	NONE
 * Return val
 *	int - The number of combo phys that are enabled
 ******************************************************************************/
int find_enabled_combo_phys()
{
	int enabled = 0;
#if TESTING
	enabled++;
	combo_table[0].enabled = 1;
#else
	unsigned int val;
	for(int i = 0; i < ARRAY_SIZE(combo_table); i++) {
		val = READ_OFFSET_DWORD(g_mmio, combo_table[i].cfgcr0.addr);
		if(val != 0 && val != 0xFFFFFFFF) {
			combo_table[i].enabled = 1;
			enabled++;
			DBG("Combo phy #%d is on\n", i);
		}
		combo_table[i].done = 1;
	}
#endif
	DBG("Total Combo phys on: %d\n", enabled);
	return enabled;
}

/*******************************************************************************
 * Description
 *	program_dkl_phys - This function programs DKL phys on the system
 * Parameters
 *	double time_diff - This is the time difference in between the primary and the
 *	secondary systems in ms. If master is ahead of the slave , then the time
 *	difference is a positive number otherwise negative.
 * Return val
 *	void
 ******************************************************************************/
void program_dkl_phys(double time_diff)
{
	double shift = SHIFT;

	for(int i = 0; i < ARRAY_SIZE(dkl_table); i++) {
		/* Skip any that aren't enabled */
		if(!dkl_table[i].enabled) {
			continue;
		}
#if TESTING
		dkl_table[i].dkl_pll_div0.orig_val = 0x50284274;
		dkl_table[i].dkl_visa_serializer.orig_val = 0x54321000;
		dkl_table[i].dkl_bias.orig_val = 0XC1000000;
		dkl_table[i].dkl_ssc.orig_val = 0x400020ff;
		dkl_table[i].dkl_dco.orig_val = 0xe4004080;
#else
		dkl_table[i].dkl_pll_div0.orig_val         = dkl_table[i].dkl_pll_div0.mod_val        = READ_OFFSET_DWORD(g_mmio, dkl_table[i].dkl_pll_div0.addr);
		dkl_table[i].dkl_visa_serializer.orig_val  = dkl_table[i].dkl_visa_serializer.mod_val = READ_OFFSET_DWORD(g_mmio, dkl_table[i].dkl_visa_serializer.addr);
		dkl_table[i].dkl_bias.orig_val             = dkl_table[i].dkl_bias.mod_val            = READ_OFFSET_DWORD(g_mmio, dkl_table[i].dkl_bias.addr);
		dkl_table[i].dkl_ssc.orig_val              = dkl_table[i].dkl_ssc.mod_val             = READ_OFFSET_DWORD(g_mmio, dkl_table[i].dkl_ssc.addr);
		dkl_table[i].dkl_dco.orig_val              = dkl_table[i].dkl_dco.mod_val             = READ_OFFSET_DWORD(g_mmio, dkl_table[i].dkl_dco.addr);
		/*
		 * For whichever PHY we find, let's set the done flag to 0 so that we can later
		 * have a timer for it to reset the default values back in their registers
		 */
		dkl_table[i].done = 0;

		if(time_diff < 0) {
			shift *= -1;
		}
		int steps = calc_steps_to_sync(time_diff, shift);
		PRINT("steps are %d\n", steps);
		user_info *ui = new user_info(DKL, &dkl_table[i]);
		make_timer((long) steps, ui);
#endif
		DBG("OLD VALUES\n dkl_pll_div0 = 0x%X\n dkl_visa_serializer = 0x%X\n "
				"dkl_bias = 0x%X\n dkl_ssc = 0x%X\n dkl_dco = 0x%X\n",
				dkl_table[i].dkl_pll_div0.orig_val,
				dkl_table[i].dkl_visa_serializer.orig_val,
				dkl_table[i].dkl_bias.orig_val,
				dkl_table[i].dkl_ssc.orig_val,
				dkl_table[i].dkl_dco.orig_val);

		/*
		 * PLL frequency in MHz (base) = 38.4 * DKL_PLL_DIV0[i_fbprediv_3_0] *
		 *		( DKL_PLL_DIV0[i_fbdiv_intgr_7_0]  + DKL_BIAS[i_fbdivfrac_21_0] / 2^22 )
		 */
		int i_fbprediv_3_0    = GETBITS_VAL(dkl_table[i].dkl_pll_div0.orig_val, 11, 8);
		int i_fbdiv_intgr_7_0 = GETBITS_VAL(dkl_table[i].dkl_pll_div0.orig_val, 7, 0);
		int i_fbdivfrac_21_0  = GETBITS_VAL(dkl_table[i].dkl_bias.orig_val, 29, 8);
		double pll_freq = (double) (REF_DKL_FREQ * i_fbprediv_3_0 * (i_fbdiv_intgr_7_0 + i_fbdivfrac_21_0 / pow(2, 22)));
		double new_pll_freq = pll_freq + (shift * pll_freq / 100);
		double new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_DKL_FREQ * i_fbprediv_3_0)) - i_fbdiv_intgr_7_0) * pow(2,22);

		if(new_i_fbdivfrac_21_0 < 0) {
			i_fbdiv_intgr_7_0 -= 1;
			dkl_table[i].dkl_pll_div0.mod_val &= ~GENMASK(7, 0);
			dkl_table[i].dkl_pll_div0.mod_val |= i_fbdiv_intgr_7_0;
			new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_DKL_FREQ * i_fbprediv_3_0)) - (i_fbdiv_intgr_7_0)) * pow(2,22);
		}

		DBG("old pll_freq = %f, new_pll_freq = %f\n", pll_freq, new_pll_freq);
		DBG("new fbdivfrac = %d = 0x%X\n", (int) new_i_fbdivfrac_21_0, (int) new_i_fbdivfrac_21_0);

		dkl_table[i].dkl_bias.mod_val &= ~GENMASK(29, 8);
		dkl_table[i].dkl_bias.mod_val |= (long)new_i_fbdivfrac_21_0 << 8;
		dkl_table[i].dkl_visa_serializer.mod_val &= ~GENMASK(2, 0);
		dkl_table[i].dkl_visa_serializer.mod_val |= 0x200;
		dkl_table[i].dkl_ssc.mod_val &= ~GENMASK(31, 29);
		dkl_table[i].dkl_ssc.mod_val |= 0x2 << 29;
		dkl_table[i].dkl_ssc.mod_val &= ~BIT(13);
		dkl_table[i].dkl_ssc.mod_val |= 0x2 << 12;
		dkl_table[i].dkl_dco.mod_val &= ~BIT(2);
		dkl_table[i].dkl_dco.mod_val |= 0x2 << 1;

		DBG("NEW VALUES\n dkl_pll_div0 = 0x%X\n dkl_visa_serializer = 0x%X\n "
				"dkl_bias = 0x%X\n dkl_ssc = 0x%X\n dkl_dco = 0x%X\n",
				dkl_table[i].dkl_pll_div0.mod_val,
				dkl_table[i].dkl_visa_serializer.mod_val,
				dkl_table[i].dkl_bias.mod_val,
				dkl_table[i].dkl_ssc.mod_val,
				dkl_table[i].dkl_dco.mod_val);
		program_dkl_mmio(&dkl_table[i], 1);
	}
}

/*******************************************************************************
 * Description
 *	program_combo_phys- This function programs Combo phys on the system
 * Parameters
 *	double time_diff - This is the time difference in between the primary and the
 *	secondary systems in ms. If master is ahead of the slave , then the time
 *	difference is a positive number otherwise negative.
 * Return val
 *	void
 ******************************************************************************/
void program_combo_phys(double time_diff)
{
	double shift = SHIFT;

	/* Cycle through all the Combo phys */
	for(int i = 0; i < ARRAY_SIZE(combo_table); i++) {
		/* Skip any that aren't enabled */
		if(!combo_table[i].enabled) {
			continue;
		}
#if TESTING
		combo_table[i].cfgcr0.orig_val = 0x01c001a5;
		combo_table[i].cfgcr1.orig_val = 0x013331cf;
#else
		READ_VAL(cfgcr0, orig_val);
		READ_VAL(cfgcr1, orig_val);

		if(time_diff < 0) {
			shift *= -1;
		}

		/*
		 * For whichever PHY we find, let's set the done flag to 0 so that we can later
		 * have a timer for it to reset the default values back in their registers
		 */
		combo_table[i].done = 0;
		int steps = calc_steps_to_sync(time_diff, shift);
		PRINT("steps are %d\n", steps);
		user_info *ui = new user_info(COMBO, &combo_table[i]);
		make_timer((long) steps, ui);
#endif
		DBG("OLD VALUES\n cfgcr0 = 0x%X\n cfgcr1 = 0x%X\n",
				combo_table[i].cfgcr0.orig_val, combo_table[i].cfgcr1.orig_val);

		/*
		 * Symbol clock frequency in MHz (base) = DCO Divider * Reference frequency in MHz /  (5 * Pdiv * Qdiv * Kdiv)
		 * DCO Divider comes from DPLL_CFGCR0 DCO Integer + (DPLL_CFGCR0 DCO Fraction / 2^15)
		 * Pdiv from DPLL_CFGCR1 Pdiv
		 * Qdiv from DPLL_CFGCR1 Qdiv Mode ? DPLL_CFGCR1 Qdiv Ratio : 1
		 * Kdiv from DPLL_CFGCR1 Kdiv
		 */
		int i_fbdiv_intgr_9_0 = GETBITS_VAL(combo_table[i].cfgcr0.orig_val, 9, 0);
		int i_fbdivfrac_14_0 = GETBITS_VAL(combo_table[i].cfgcr0.orig_val, 24, 10);
		int pdiv = GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 5, 2) + 1;
		int kdiv = GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 8, 6);
		/* TODO: In case we run into some other weird dividers, then we may need to revisit this */
		int qdiv = (kdiv == 2) ? GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 17, 10) : 1;
		double dco_divider = ((double) i_fbdiv_intgr_9_0 + ((double) i_fbdivfrac_14_0  / pow(2, 15)));
		double dco_clock = REF_COMBO_FREQ * dco_divider;
		double pll_freq = dco_clock / (5 * pdiv * qdiv * kdiv);
		double new_pll_freq = pll_freq + (shift * pll_freq / 100);
		double new_dco_clock = dco_clock + (shift * dco_clock / 100);
		double fb_div_frac = (new_dco_clock - (i_fbdiv_intgr_9_0 * REF_COMBO_FREQ)) / REF_COMBO_FREQ;
		if(fb_div_frac > 1) {
			i_fbdiv_intgr_9_0 += 1;
		} else if (fb_div_frac < 0) {
			i_fbdiv_intgr_9_0 -= 1;
		}
		combo_table[i].cfgcr0.mod_val &= ~GENMASK(9, 0);
		combo_table[i].cfgcr0.mod_val |= i_fbdiv_intgr_9_0;
		double new_i_fbdivfrac_14_0  = (((new_pll_freq * (5 * pdiv * qdiv * kdiv)) / REF_COMBO_FREQ) - i_fbdiv_intgr_9_0) * pow(2, 15);

		DBG("old pll_freq = %f, new_pll_freq = %f\n", pll_freq, new_pll_freq);
		DBG("old fbdivfrac = %d = 0x%X\n", (int) i_fbdivfrac_14_0, (int) i_fbdivfrac_14_0);
		DBG("new fbdivfrac = %d = 0x%X\n", (int) new_i_fbdivfrac_14_0, (int) new_i_fbdivfrac_14_0);

		combo_table[i].cfgcr0.mod_val &= ~GENMASK(24, 10);
		combo_table[i].cfgcr0.mod_val |= (long) new_i_fbdivfrac_14_0 << 10;

		DBG("NEW VALUES\n cfgcr0 = 0x%X\n", combo_table[i].cfgcr0.mod_val);
		program_combo_mmio(&combo_table[i], 1);
	}
}

/*******************************************************************************
 * Description
 *	check_if_dkl_done - This function checks to see if the DKL programming is
 *	finished. There is a timer for which time the new values will remain in
 *	effect. After that timer expires, the original values will be restored.
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void check_if_dkl_done()
{
	for(int i = 0; i < ARRAY_SIZE(dkl_table); i++) {
		while(!dkl_table[i].done) {
			usleep(1000);
		}
	}
}

/*******************************************************************************
 * Description
 *	check_if_dkl_done - This function checks to see if the Combo programming is
 *	finished. There is a timer for which time the new values will remain in
 *	effect. After that timer expires, the original values will be restored.
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void check_if_combo_done()
{
	/* Wait to write back the original value */
	for(int i = 0; i < ARRAY_SIZE(combo_table); i++) {
		while(!combo_table[i].done) {
			usleep(1000);
		}
	}
}

/*******************************************************************************
 * Description
 *	synchronize_phys - This function finds enabled phys, synchronizes them and
 *	waits for the sync to finish
 * Parameters
 *	int type - Whether it is a DKL or a Combo phy
 *	double time_diff - This is the time difference in between the primary and the
 *	secondary systems in ms. If master is ahead of the slave , then the time
 *	difference is a positive number otherwise negative.
 * Return val
 *	void
 ******************************************************************************/
void synchronize_phys(int type, double time_diff)
{
	if(!phy[type].find()) {
		DBG("No %s PHYs found\n", phy[type].name);
		return;
	}

	/* Cycle through all the phys */
	phy[type].program(time_diff);

	/* Wait to write back the original values */
	phy[type].check_if_done();
}

/*******************************************************************************
 * Description
 *  synchronize_vsync - This function synchronizes the primary and secondary
 *  systems vsync. It is run on the secondary system. The way that it works is
 *  that it finds out the default PHY register values, calculates a shift
 *  based on the time difference provided by the caller and then reprograms the
 *  PHY registers so that the secondary system can either slow down or speed up
 *  its vsnyc durations.
 * Parameters
 * double time_diff - This is the time difference in between the primary and the
 * secondary systems in ms. If master is ahead of the slave , then the time
 * difference is a positive number otherwise negative.
 * Return val
 *  void
 ******************************************************************************/
void synchronize_vsync(double time_diff)
{
	if(!IS_INIT()) {
		ERR("Uninitialized lib, please call lib init first\n");
		return;
	}

	for(int i = DKL; i < TOTAL_PHYS; i++) {
		synchronize_phys(i, time_diff);
	}
}

/*******************************************************************************
 * Description
 *	vblank_handler - The function which will be called whenever a VBLANK occurs
 * Parameters
 *	int fd - The device file descriptor
 *	unsigned int frame - Frame number
 *	unsigned int sec - second when the vblank occured
 *	unsigned int usec - micro second when the vblank occured
 *	void *data - a private data structure pointing to the vbl_info
 * Return val
 *  void
 ******************************************************************************/
static void vblank_handler(int fd, unsigned int frame, unsigned int sec,
			   unsigned int usec, void *data)
{
	drmVBlank vbl;
	vbl_info *info = (vbl_info *)data;
	memset(&vbl, 0, sizeof(drmVBlank));
	if(info->counter < info->size) {
		info->vsync_array[info->counter++] = TIME_IN_USEC(sec, usec);
	}

	vbl.request.type = (drmVBlankSeqType) (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)data;

	drmWaitVBlank(g_dev_fd, &vbl);
}

/*******************************************************************************
 * Description
 *  get_vsync - This function gets a list of vsyncs for the number of times
 *  indicated by the caller and provide their timestamps in the array provided
 * Parameters
 *	long *vsync_array - The array in which vsync timestamps need to be given
 *	int size - The size of this array. This is also the number of times that we
 *	need to get the next few vsync timestamps.
 * Return val
 *  int - 0 == SUCCESS, -1 = ERROR
 ******************************************************************************/
int get_vsync(long *vsync_array, int size)
{
	drmVBlank vbl;
	int ret;
	drmEventContext evctx;
	vbl_info handler_info;

	g_dev_fd = open_device();
	if(g_dev_fd < 0) {
		ERR("Couldn't open /dev/dri/card0. Is i915 installed?\n");
		return -1;
	}

	memset(&vbl, 0, sizeof(drmVBlank));

	handler_info.vsync_array = vsync_array;
	handler_info.size = size;
	handler_info.counter = 0;

	/* Queue an event for frame + 1 */
	vbl.request.type = (drmVBlankSeqType) (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)&handler_info;
	ret = drmWaitVBlank(g_dev_fd, &vbl);
	if (ret) {
		ERR("drmWaitVBlank (relative, event) failed ret: %i\n", ret);
		close_device();
		return -1;
	}

	/* Set up our event handler */
	memset(&evctx, 0, sizeof evctx);
	evctx.version = DRM_EVENT_CONTEXT_VERSION;
	evctx.vblank_handler = vblank_handler;
	evctx.page_flip_handler = NULL;

	/* Poll for events */
	for(int i = 0; i < size; i++) {
		struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(g_dev_fd, &fds);
		ret = select(g_dev_fd + 1, &fds, NULL, NULL, &timeout);

		if (ret <= 0) {
			ERR("select timed out or error (ret %d)\n", ret);
			continue;
		} else if (FD_ISSET(0, &fds)) {
			break;
		}

		ret = drmHandleEvent(g_dev_fd, &evctx);
		if (ret) {
			ERR("drmHandleEvent failed: %i\n", ret);
			close_device();
			return -1;
		}
	}

	close_device();
	return 0;
}
