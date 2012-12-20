/*****************************************************************************
**																	        **
**		N E O _ I F  -  NEO Javacard applet interface                       **
**																			**
**		Date		/ Rev		/ Sign	/ Remark							**
**		12-10-29	/ 0.0.0		/ J E	/ Main								**
**																			**
*****************************************************************************/

#ifndef	__NEO_IF_H_INCLUDED__
#define	__NEO_IF_H_INCLUDED__

#define NEO_AID     { 0xa0, 0x00, 0x00, 0x05, 0x27, 0x20, 0x01, 0x01 }

#define INS_YK2_REQ             0x01    // General request (cmd in P1)
#define INS_YK2_OTP             0x02    // Generate OTP (slot in P1)
#define INS_YK2_STATUS          0x03    // Read out status record
#define INS_YK2_NDEF            0x04    // Read out NDEF record

#define NEO_CONFIG_1            0x00    // Configuration 1
#define NEO_CONFIG_2            0x01    // Configuration 2

#define FORMAT_ASCII            0x00    // Output format is ascii
#define FORMAT_ASCII_NO_FMT     0x01    // Ascii, no formatting characters
#define FORMAT_SCANCODE         0x02    // Output format is scan codes

#define SW_BUTTON_REQD          0x6985  // ISO7816 Access condition not met

#endif  // __NEO_IF_H_INCLUDED__
