/**
* Xadow NFC can work in 2 modes:
* 1) adapter, can read uid, read NDEF data, write NDEF, erase NDEF memory
*             supported card: Mifare classic, Mifare ultralight
* 2) emulate as a TAG, with 106 kbps DEP, support setting the uid of the emulated tag.
*/



#ifndef _XADOW_NFC_H_
#define _XADOW_NFC_H_

#include "mbed.h"

/*!
	\def NFC_DEVICE_ADDR
	The I2C address of NFC

	\def NFC_SCAN_ID
	The id of scan data, the format is 0, 0, 0, Device address

	\def NFC_ADAPTER_INIT_ID
	The id of NFC initialize as a adapter

	\def NFC_ADAPTER_GET_UID_ID
	The id of NFC adapter to get uid

	\def NFC_ADAPTER_READ_ID
	The id of NFC adapter to read NDEF message

	\def NFC_ADAPTER_WRITE_ID
	The id of NFC adapter to write NDEF message

	\def NFC_ADAPTER_ERASE_ID
	The id of NFC adapter to erase NDEF message

	\def NFC_EMULATE_INIT_ID
	The id of NFC emulate as a tag
*/

/*
 *	Data format : ID, Length (n bytes, form 1 to n), Data1, Data2, ... ,Data n
 * For example, get the scan data.
 * First, Send: 0x00(NFC_SCAN_ID),0x00(Data length) to device.
 * Second, Receive scan data(ID + Data length + Data 1, Data 2, Data 3, Data 4 = 6 bytes).
 * Third, The scan data begin from the third data of received.
 * End
 */

#define NFC_DEVICE_ADDR    			(0x06<<1) //
#define NFC_SCAN_ID					0 //
#define NFC_ADAPTER_INIT_ID			1 //
#define NFC_ADAPTER_GET_UID_ID		2 //
#define NFC_ADAPTER_READ_ID			3 //
#define NFC_ADAPTER_WRITE_ID		4 //
#define NFC_ADAPTER_ERASE_ID		5 //
#define NFC_EMULATE_INIT_ID			6 //

/**
 *  \brief Get the status of the NFC device.
 *
 *  \return Return TRUE or FALSE. TRUE is on line, FALSE is off line.
 *
 */
unsigned char nfc_check_online(void);

/**
 *  \brief Initialize NFC as a adapter, that can read and write a tag.
 *
 *  \return Return Null.
 *
 */
void nfc_adapter_init(void);

/**
 *  \brief Get the uid data from a tag.
 *
 *  \return Return the pointer of uid data, the format is  NFC_ADAPTER_GET_UID_ID, Data length, Data 1, Data2, ...
 *
 */
unsigned char *nfc_adapter_get_uid(void);

/**
 *  \brief Read the NDEF data from a tag.
 *
 *  \return Return the pointer of NEDF data, the format is  NFC_ADAPTER_READ_ID, Data length, Data 1, Data2, ...
 *
 */
unsigned char *nfc_adapter_read(void);

/**
 *  \brief Write the NDEF data to a tag.
 *
 *  \param [in] *string the NEDF data want to write. For example, "Xadow kit nfc test!"
 *  \return Return the pointer of NEDF data, the format is  NFC_ADAPTER_READ_ID, Data length, Data 1, Data2, ...
 *
 */
void nfc_adapter_write(char *string);

/**
 *  \brief Earse the NDEF data.
 *
 *  \return Return Null.
 *
 */
void nfc_adapter_erase(void);

/**
 *  \brief Emulate NFC as a target
 *
 * \param [in] *id the target uid, the first data is fixed, so the format is 0x??, 0x??, 0x??(3 bytes)
 *  \return Return Null.
 *
 */
void nfc_emulate_init(unsigned char *id);


#endif
