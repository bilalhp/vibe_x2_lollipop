#include <cust_vibrator.h>
#include <linux/types.h>

static struct vibrator_hw cust_vibrator_hw = {
	.vib_timer = 25,
  #ifdef CUST_VIBR_LIMIT
	.vib_limit = 9,
  #endif
  
 /*
  *chris vibrator voltage mapping: VIBR for mt6325 datasheet:
  * bit mask(0b)  default voltage(v)
  *    111             3.3
  *    110             3.0
  *    101             2.8
  *    100             2.5
  *    011             1.8
  *    010             1.5
  *    001             1.3
  *    000             1.2
  */

  #ifdef CUST_VIBR_VOL
	.vib_vol = 0x4,//chris  2015.04.23 modify default voltage to 2.5v for vibrating tenstity is too strong 
  #endif
};

struct vibrator_hw *get_cust_vibrator_hw(void)
{
    return &cust_vibrator_hw;
}

