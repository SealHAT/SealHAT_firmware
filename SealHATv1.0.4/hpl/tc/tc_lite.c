
/**
 * \file
 *
 * \brief TC related functionality implementation.
 *
 * Copyright (C) 2017 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include "tc_lite.h"

/**
 * \brief Initialize TC interface
 */
int8_t TIMER_MS_init()
{

	hri_tc_wait_for_sync(TC4, TC_SYNCBUSY_SWRST);
	if (hri_tc_get_CTRLA_ENABLE_bit(TC4)) {
		return ERR_DENIED;
	}

	hri_tc_write_CTRLA_reg(TC4,
	                       0 << TC_CTRLA_COPEN0_Pos          /* Capture Pin 0 Enable: disabled */
	                           | 0 << TC_CTRLA_COPEN1_Pos    /* Capture Pin 1 Enable: disabled */
	                           | 0 << TC_CTRLA_CAPTEN0_Pos   /* Capture Channel 0 Enable: disabled */
	                           | 0 << TC_CTRLA_CAPTEN1_Pos   /* Capture Channel 1 Enable: disabled */
	                           | 0 << TC_CTRLA_ALOCK_Pos     /* Auto Lock: disabled */
	                           | 1 << TC_CTRLA_PRESCSYNC_Pos /* Prescaler and Counter Synchronization: 1 */
	                           | 0 << TC_CTRLA_ONDEMAND_Pos  /* Clock On Demand: disabled */
	                           | 1 << TC_CTRLA_RUNSTDBY_Pos  /* Run in Standby: enabled */
	                           | 4 << TC_CTRLA_PRESCALER_Pos /* Setting: 4 */
	                           | 0x0 << TC_CTRLA_MODE_Pos);  /* Operating Mode: 0x0 */

	hri_tc_write_CTRLB_reg(TC4,
	                       0 << TC_CTRLBSET_CMD_Pos           /* Command: 0 */
	                           | 0 << TC_CTRLBSET_ONESHOT_Pos /* One-Shot: disabled */
	                           | 0 << TC_CTRLBCLR_LUPD_Pos    /* Setting: disabled */
	                           | 0 << TC_CTRLBSET_DIR_Pos);   /* Counter Direction: disabled */

	// hri_tc_write_WAVE_reg(TC4,0); /* Waveform Generation Mode: 0 */

	// hri_tc_write_DRVCTRL_reg(TC4,0 << TC_DRVCTRL_INVEN1_Pos /* Output Waveform 1 Invert Enable: disabled */
	//		 | 0 << TC_DRVCTRL_INVEN0_Pos); /* Output Waveform 0 Invert Enable: disabled */

	// hri_tc_write_DBGCTRL_reg(TC4,0); /* Run in debug: 0 */

	// hri_tccount16_write_CC_reg(TC4, 0 ,0x0); /* Compare/Capture Value: 0x0 */

	// hri_tccount16_write_CC_reg(TC4, 1 ,0x0); /* Compare/Capture Value: 0x0 */

	// hri_tccount16_write_COUNT_reg(TC4,0x0); /* Counter Value: 0x0 */

	hri_tc_write_EVCTRL_reg(
	    TC4,
	    0 << TC_EVCTRL_MCEO0_Pos       /* Match or Capture Channel 0 Event Output Enable: disabled */
	        | 0 << TC_EVCTRL_MCEO1_Pos /* Match or Capture Channel 1 Event Output Enable: disabled */
	        | 0 << TC_EVCTRL_OVFEO_Pos /* Overflow/Underflow Event Output Enable: disabled */
	        | 1 << TC_EVCTRL_TCEI_Pos  /* TC Event Input: enabled */
	        | 0 << TC_EVCTRL_TCINV_Pos /* TC Inverted Event Input: disabled */
	        | 1);                      /* Event Action: 1 */

	// hri_tc_write_INTEN_reg(TC4,0 << TC_INTENSET_MC0_Pos /* Match or Capture Channel 0 Interrupt Enable: disabled */
	//		 | 0 << TC_INTENSET_MC1_Pos /* Match or Capture Channel 1 Interrupt Enable: disabled */
	//		 | 0 << TC_INTENSET_ERR_Pos /* Error Interrupt Enable: disabled */
	//		 | 0 << TC_INTENSET_OVF_Pos); /* Overflow Interrupt enable: disabled */

	hri_tc_write_CTRLA_ENABLE_bit(TC4, 1 << TC_CTRLA_ENABLE_Pos); /* Enable: enabled */

	return 0;
}
