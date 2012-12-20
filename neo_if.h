/*
Copyright (c) 2012 Yubico AB
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
