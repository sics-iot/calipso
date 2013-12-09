/*
 * net_setup_process.c
 *
 * Contiki OS System Process.
 * Waits for AR9170 device connection and performs
 * the required initialization, including the 
 * transmission of the eject command that detaches
 * the mass storage device, that initially appears.
 *
 * Created: 5/14/2013 4:45:32 PM
 *  Author: Ioannis Glaropoulos
 */ 

#include "contiki.h"
#include "usb_lock.h"
#include "compiler.h"
#include "regd.h"
#include "ath.h"
#include "ar9170.h"
#include "mac80211.h"
#include "ar9170_main.h"
#include "ieee80211_ibss.h"
#include "usb_cmd_wrapper.h"
#include "usb_fw_wrapper.h"
#include "contiki-main.h"
#include "clock.h"
#include "net_setup_process.h"
#include <sys\errno.h>
#include "platform-conf.h"
#include "ar9170_led.h"
#include "tft_process.h"
#include "smalloc.h"
#include "ibss_setup_process.h"
#include "net_scheduler_process.h"
#include "netstack.h"



PROCESS(net_setup_process, "Network Setup Process");

int add_network_interface(struct ar9170* ar) {
	
	U32 bss_info_changed_flag = 0;
		
	PRINTF("NET_SETUP_PROCESS: Enable wireless networking.\n");
	
	/* Parse the given device firmware */
	if (ar9170_parse_firmware(ar)) {
		printf("ERROR: Parsing device firmware returned errors.\n");
		return -EINVAL;
	}
	
	/* Initialize AR9170 device */
	if (!ar9170_init_device(ar)) {
		printf("ERROR: Device could not be initialized.\n");
		return -ENXIO;
	}
	
	/* Register device */
	if (!ar9170_register_device(ar)) {
		printf("ERROR: Device could not be registered.\n");
		return -ENXIO;
	}

	// EXTRA FIXME - put it somewhere better than just here
	ar->common.regulatory.country_code = CTRY_SWITZERLAND;
	ar->common.regulatory.regpair = smalloc(sizeof(struct reg_dmn_pair_mapping ));
	ar->common.regulatory.regpair->reg_2ghz_ctl = 48; // XXX XXX This is an ugly hack and we should fix it better
	
	/* Initially the PS state is OFF, as well as the OFF override flag */
	ar->ps.state = 0;
	ar->ps.off_override = 0;
		
	/* Start device */
	if (!ar9170_op_start(ar)) {
		printf("ERROR: Device could not start!\n");
		return -ENXIO;
	}
		
	/* Add interface */
	if (ar9170_op_add_interface(hw, unique_vif)) {
		printf("ERROR: Interface could not be added!\n");
		return -1;
	}
	
	// Call bss_info_changed for modifying slot time
	bss_info_changed_flag |= BSS_CHANGED_ERP_SLOT;
	ar9170_op_bss_info_changed(hw, unique_vif, 
						&unique_vif->bss_conf, 
						bss_info_changed_flag);
						
	/* Inform the uIP that the MAC address is now registered. 
	 * We do this by re-calling NETSTACK_INIT for the network
	 * module.
	 */
	NETSTACK_NETWORK.init();					
	/* All OK */					
	return 0; 
}


/* Configure device so network setup is possible. 
 * The procedure is as follows: the device will 
 * attempt to scan for the default IBSS. If it is 
 * not possible to join this IBSS, the device 
 * shall proceed with creating the default IBSS
 */
int start_network_setup_operation() 
{	
	U32 config_flag_changed = 0;
	int err;
	
	#if AR9170_MAIN_DEBUG
	printf("DEBUG: __start network operation.\n");
	#endif
	
	/* Update TX QoS */		
	struct ieee80211_tx_queue_params params[4];
	
	CARL9170_FILL_QUEUE(params[3], 7, 31, 1023,	0);
	CARL9170_FILL_QUEUE(params[2], 3, 31, 1023,	0);
	CARL9170_FILL_QUEUE(params[1], 2, 15, 31,	188);
	CARL9170_FILL_QUEUE(params[0], 2, 7,  15,	102);	
	
	/* Configure tx queues */
	 if (ar9170_op_conf_tx(hw, unique_vif, 0, &params[0]) ||
		 ar9170_op_conf_tx(hw, unique_vif, 1, &params[1]) ||
		 ar9170_op_conf_tx(hw, unique_vif, 2, &params[2]) ||
		 ar9170_op_conf_tx(hw, unique_vif, 3, &params[3])) {
		 printf("ERROR: Could not set TX parameters.\n");
		 return -1;
	 }
	 
	/* BSS Change notification: change device to idle. */
	config_flag_changed = BSS_CHANGED_IDLE;
	ar9170_op_bss_info_changed(hw,unique_vif, &unique_vif->bss_conf, config_flag_changed);
	
	/* Flush device if necessary. Drain beacon.*/
	ar9170_op_flush(hw, false);
	
	config_flag_changed = 0;
	config_flag_changed |= IEEE80211_CONF_CHANGE_CHANNEL |
	IEEE80211_CONF_CHANGE_POWER |
	IEEE80211_CONF_CHANGE_LISTEN_INTERVAL;// |	IEEE80211_CONF_CHANGE_PS;
	
	/* Configure network hardware parameters */ 
	err =  __ieee80211_hw_config(hw, config_flag_changed);
	
	if (err) {
		printf("ERROR: IEEE80211 HW configuration returned errors.\n");
		return -1;
	}		
	
	/* Everything is fine */  
	return 0;
}
/*---------------------------------------------------------------------------*/


static int allocate_mac_resources()
{
	struct ar9170* ar;
	/* Allocate resources for AR9170 */
	struct ar9170** ar_pt = ar9170_get_device_pt();
	
	if ((*ar_pt) != NULL) {
		PRINTF("ERROR: AR9170 device already initialized when allocation is called.\n");
		return -EINVAL;
		
	} else {
		ar = ar9170_alloc(ar_pt);
	}
	/* Check the result of memory allocation */
	if (ar == NULL) {
		PRINTF("ERROR: Could not allocate memory for the AR9170 struct.\n");
		return -ENOMEM;
	}
	/* Allocate resource for IEEE80211 HW structure */
	if(__ieee80211_sta_init_config()) {
		PRINTF("ERROR: Could not allocate memory for the IEEE80211 HW struct.\n");
		return -ENOMEM;
	}
	/* Copy the network SSID */
	memcpy(ar->common.curbssid, ibss_info->ibss_bssid, ETH_ALEN);
	
	/* Assign pointers: AR9170 has a pointer to the unique hw structure */
	ar->hw = hw;
	ar->channel = hw->conf.channel;
	if (!ar->hw->wiphy) {
		PRINTF("ERROR: Could not allocate memory for the WIPHY struct.\n");
		return -ENOMEM;
	}
	// AR9170 obtains a virtual interface of id 0.
	ar->vif_priv[0].id = 0;
	
	/* AR9170 struct gets a pointer to the virtual interface info */
	ar->beacon_iter = unique_cvif;
	
	if(ar->beacon_iter == NULL) {
		return -1;
	}
	/* Everything is fine */
	return 0;
}
/*---------------------------------------------------------------------------*/


/* Network setup completion flag. */
static volatile bool net_setup_process_completed_flag;
/*---------------------------------------------------------------------------*/


bool is_net_setup_completed() {
	
	return net_setup_process_completed_flag;
}
/*---------------------------------------------------------------------------*/


static void net_setup_process_poll_handler(void)
{
	if (ar9170_is_mass_storage_device_plugged()) {
		/* The MSC device has been plugged-in. Check whether the WLAN USB 
		 * device is also plugged-in and if yes, signal an error, otherwise
		 * continue with device EJECT procedure.
		 */
		if (!ar9170_is_wlan_device_started()) {
			
			/* The Mass Storage device is plugged. */
			PRINTF("NET_SETUP_PROCESS: Mass Storage device has been plugged.\n");
			
			/* At this point the Mass Storage device has been plugged-in, so we need 
			 * to send the EJECT command to the Mass Storage Device. Signal an error 
			 * if the WLAN device is assumed to be already connected.
			 */
			
			/* Send the EJECT command now. The command will be sent only once. */
			if (ar9170_eject_mass_storage_device_cmd()) {
			
				/* Yield process. Process shall wait for the appearance of the 
				 * WLAN device.
				 */			
				PRINTF("NET_SETUP_PROCESS: EJECT command was sent.\n");	
			}
			/* This code should not normally be executed again, since the 
			 * disconnection of the MSC device will release the flag. But,
			 * it might be, if the disconnection routine is slow, so thanks
			 * to the "eject_lock" flag the EJECT command is sent only once.
			 */
						
		} else {
			
			PRINTF("ERROR: We have not sent an EJECT command, but WLAN device is connected!\n");
			
			/* Post an even for process termination */
			process_post(&net_setup_process, PROCESS_EVENT_EXIT, NULL);
			
			/* Do not poll the process again. This is a serious error. */
			return;
		}	
	
	} else if (ar9170_is_wlan_device_started()) {
		
		/* The WLAN device has been plugged-in. */
		if (!ar9170_eject_is_semaphore_locked()) {
			
			PRINTF("ERROR: WLAN device is started, while Mass Storage EJECT command has not been sent.\n");
			
			/* Post an even for process termination */
			process_post(&net_setup_process, PROCESS_EVENT_EXIT, NULL);
			
			/* Do not poll the process again. */
			return;
		
		} else {
	
			/* Everything is fine. */
			PRINTF("NET_SETUP_PROCESS: AR9170 WLAN device is now plugged-in.\n");
			/* The WLAN USB device is not plugged. So we clear the "unplugged"
			 * flag, which will be set upon device disconnection to inform the
			 * the related system processes that the device will no longer be 
			 * available.
			 */
			ar9170_clear_unplugged_flag();
	
			/* We should now allocate the required resources and run the default 
			 * network configuration. 
			 */	
			if(allocate_mac_resources()) {
	
				PRINTF("ERROR: MAC resources allocation returned errors.\n");
				goto _err_out;		
			}	
			/* We can now enable networking by booting the AR9170 device. */
	
			/* Obtain the reference to the AR9170 device */
			struct ar9170* ar = ar9170_get_device();
	
			if (!ar) {
				PRINTF("ERROR: Device not properly initialized. Network initialization failed.\n");
				goto _err_out;
			}

			/* Initialize and add the network interface. */
			if (add_network_interface(ar)) {
				printf("ERROR: AR9170 device could not be added!\n");
				goto _err_out;
			}
	
			/* Start network setup operation. */
			start_network_setup_operation();	
			
			/* Start the IBSS initialization and networking schedulers. */
			process_start(&ibss_setup_process, NULL);
			process_start(&net_scheduler_process, NULL);
			
			process_post(&net_setup_process, PROCESS_EVENT_CONTINUE, NULL);
			/* Do not poll the process again. */
			return;
		_err_out:
			process_post(&net_setup_process, PROCESS_EVENT_EXIT, NULL);
			/* Do not poll the process again. */
			return;	
		}		
	
	} else {
		
		/* Device has not be plugged yet. Nothing to be done yet. */		
	}				
			
	/* Poll the process again so it runs continuously. */
	process_poll(&net_setup_process);
}
/*---------------------------------------------------------------------------*/


static void net_setup_process_exit_handler(void)
{
	PRINTF("NET_SETUP_PROCESS terminates.\n");
}
/*---------------------------------------------------------------------------*/


static void net_setup_process_display_setup_result() 
{
	char* str1 = (char*)"AR9170 device added!";
	char* str2 = (char*)"Local MAC Address:";
	
	char** _page = (char**)smalloc(3*sizeof(char*));
	uint8_t* _string_lengths = (uint8_t*)smalloc(3*sizeof(uint8_t));
	
	_page[0] = ( char*)smalloc(20*sizeof( char));
	memcpy(_page[0], str1, 20);
	
	_page[1] = ( char*)smalloc(18*sizeof( char));
	memcpy(_page[1], str2, 18);
	
	_page[2] = ( char*)smalloc(18*sizeof( char));
	
	sprintf(&_page[2][0], " %02x:%02x:%02x:%02x:%02x:%02x", unique_vif->addr[0],unique_vif->addr[1],
	unique_vif->addr[2],unique_vif->addr[3],unique_vif->addr[4],unique_vif->addr[5]);
	
	/* Lengths. */
	_string_lengths[0] = 20;
	_string_lengths[1] = 18;
	_string_lengths[2] = 18;
	
	tft_process_request_update(0, _page, _string_lengths, 3);
	
	/* Free memory. */
	sfree(_string_lengths);
	sfree(_page[0]);
	sfree(_page[1]);
	sfree(_page[2]);
	sfree(_page);
}
/*---------------------------------------------------------------------------*/


static void net_setup_process_init() {
	
	/* Initialize flags while waiting for device connection. */
	
	/* Indicate that the EJECT command has not been sent. */
	ar9170_eject_init_lock();
	
	/* Release the CTRL endpoint lock flag. */
	ar9170_usb_ctrl_out_init_lock();
	
	/* Indicate that neither the MSC nor the WLAN USB devices
	 * have been connected so far.
	 */
	ar9170_set_device_stopped();
	
	/* Initially the network setup is not completed. */
	net_setup_process_completed_flag = false;
}
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(net_setup_process, ev, data)
{ 	
	/* Declare the poll handler for the network setup process. */	
	PROCESS_POLLHANDLER(net_setup_process_poll_handler());
	
	/* Declare the exit handler for the network setup process. */
	PROCESS_EXITHANDLER(net_setup_process_exit_handler());
	
	/* Start process */
	PROCESS_BEGIN();
	
	PRINTF("NET_SETUP_PROCESS: Waiting for device connection...\n");
	
	net_setup_process_init();
				
	/* Poll the process for the first time. */
	process_poll(&net_setup_process);
	
	/* Wait until the process can either continue or terminate. */
	PROCESS_WAIT_EVENT_UNTIL((ev == PROCESS_EVENT_EXIT)||(ev == PROCESS_EVENT_CONTINUE));

	if (ev == PROCESS_EVENT_CONTINUE) {	
		
		/* Exit normally. */
		PRINTF("NET_SETUP_PROCESS: Network setup completed successfully!\n");
		/* Set GREEN led signaling network setup completion. */
		ar9170_led_set_state(ar9170_get_device(), AR9170_LED_GREEN);
	
		/* Print NET initialization information on the TFT screen. */
#ifdef WITH_ADAFRUIT_TFT_LCD	
		net_setup_process_display_setup_result();
#endif	
		/* At this point the IBSS network is neither created, nor joined. 
		 * We must signal a notification to the IBSS_SETUP_Process, to 
		 * handle the joining or creation of the default IBSS network.
		 */
		net_setup_process_completed_flag = true;
		goto out;
			
	} else if (ev == PROCESS_EVENT_EXIT) {	
		/* Exit is hear implying an error in the process. */
		PRINTF("NET_SETUP_PROCESS needs to exit due to some erron while setting up networking.\n");
		goto err_out;
	}
	
err_out:	
	PRINTF("Network Initialization completed with errors.\n");
out:
	
	PRINTF("NET_SETUP_PROCESS END.\n");
	PROCESS_END();	
}
/*---------------------------------------------------------------------------*/
				
				