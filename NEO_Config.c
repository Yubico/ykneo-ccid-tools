/*
 * Copyright (c) 2012 Yubico AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif

#include "neo_if.h"
#include "ykdef.h"

#include "cmdline.h"

typedef struct {
	BYTE cla;
	BYTE ins;
	BYTE p1;
	BYTE p2;
} APDU_1;

typedef struct {
	BYTE cla;
	BYTE ins;
	BYTE p1;
	BYTE p2;
	BYTE lc;
	union {
		BYTE data[0x100];
		DEVICE_CONFIG config;
	};
} APDU_2;

static void
dumpHex(const char *descr, BYTE *buf, int bcnt)
{
	int i, j;
	BYTE *p = buf;

	printf("%s: %d bytes\n", descr, bcnt);

	for (i = 0; ; i += 0x10) {
		printf("%04x:", i);
		for (j = 0; j < 0x10; j++) {
			if (j < bcnt)
				printf("%c%02x", (j == 8) ? '-' : ' ', *p++);
			else
				printf("   ");
		}
		printf(" <");
		for (j = 0; j < 0x10; j++, buf++) {
			if (j < bcnt)
				putchar(isprint(*buf) ? *buf : '.');
			else
				putchar(' ');
		}
		printf(">\n");
		if (bcnt <= 0x10) break;
		bcnt -= 0x10;
	}
}

static int
modeswitch (int verbose, int autocommit, int mode)
{
  LONG rc;
  SCARDCONTEXT hContext;
  SCARDHANDLE hCard;
  DWORD dwReaders, dwActiveProtocol, dwRecvLength;
  char rdrBuf[1024];
  BYTE neoAID[] = NEO_AID;
  BYTE firstSeq;
  union {
    APDU_1 apdu1;
    APDU_2 apdu2;
    BYTE buf[1];
  } apdu;
  union {
    BYTE buf[0x100];
    STATUS status;
    struct {
      STATUS status;
      DEVICE_CONFIG config;
    } select;
  } rAPDU;
  int usb_mode = 0;
  int usb_mode_seen = false;
  int c;
  char commitbuf[256]; size_t commitlen;

  // Establish context and find reader(s)

  rc = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
  if (rc != SCARD_S_SUCCESS) {
    fprintf(stderr, "error: SCardEstablishContext failed, rc=%08lx\n", rc);
    return 1;
  }

  rc = SCardListReaders(hContext, NULL, NULL, &dwReaders);
  if (rc != SCARD_S_SUCCESS) {
    fprintf(stderr, "error: SCardListReaders failed, rc=%08lx\n", rc);
    SCardReleaseContext(hContext);
    return 1;
  }

  if (dwReaders > sizeof(rdrBuf))
    dwReaders = sizeof(rdrBuf);

  rc = SCardListReaders(hContext, NULL, rdrBuf, &dwReaders);
  if (rc != SCARD_S_SUCCESS) {
    fprintf(stderr, "error: SCardListReaders failed, rc=%08lx\n", rc);
    SCardReleaseContext(hContext);
    return 1;
  }

  // Connect to card (if any)

  rc = SCardConnect(hContext, rdrBuf, SCARD_SHARE_SHARED,
		    SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
  if (rc != SCARD_S_SUCCESS) {
    fprintf(stderr, "error: SCardConnect failed, rc=%08lx\n", rc);
    SCardReleaseContext(hContext);
    return 1;
  }

  // Transmit NEO select command

  apdu.apdu2.cla = 0;
  apdu.apdu2.ins = 0xa4;  // GP select
  apdu.apdu2.p1 = 0x04;
  apdu.apdu2.p2 = 0;
  apdu.apdu2.lc = sizeof(neoAID);
  memcpy(apdu.apdu2.data, neoAID, sizeof(neoAID));

  dwRecvLength = sizeof(rAPDU);
  rc = SCardTransmit(hCard, SCARD_PCI_T1, (BYTE *) &apdu, 5 + sizeof(neoAID),
		     NULL, rAPDU.buf, &dwRecvLength);

  if (rc == SCARD_S_SUCCESS) {
    dumpHex("\nSCardTransmit [NEO select aid]", rAPDU.buf, dwRecvLength);

    // Parse STATUS and DEVICE_CONFIG records

    printf("\nVersion:       %d.%d.%d\n", rAPDU.select.status.versionMajor,
	   rAPDU.select.status.versionMinor, rAPDU.select.status.versionBuild);
    printf("Seq:           %d\n", rAPDU.select.status.pgmSeq);
    printf("Mode:          %02x\n", rAPDU.select.config.mode & MODE_MASK);
    printf("Flags:         %02x\n", rAPDU.select.config.mode & (~MODE_MASK));
    printf("CR timeout:    %d\n", rAPDU.select.config.crTimeout);
    printf("Eject time:    %d\n", rAPDU.select.config.autoEjectTime);
    firstSeq = rAPDU.select.status.pgmSeq;
  } else
    printf("SCardTransmit(1) failed\n");

  fprintf(stderr, "\nCommit? (y/n) [n]: ");
  if (autocommit) {
    strcpy(commitbuf, "yes");
    puts(commitbuf);
  } else {
    fgets(commitbuf, sizeof(commitbuf), stdin);
  }
  commitlen = strlen(commitbuf);
  if (commitbuf[commitlen - 1] == '\n')
    commitbuf[commitlen - 1] = '\0';

  // Transmit set config c ommand

  apdu.apdu2.cla = 0;
  apdu.apdu2.ins = INS_YK2_REQ;
  apdu.apdu2.p1 = SLOT_DEVICE_CONFIG;
  apdu.apdu2.p2 = 0;
  apdu.apdu2.lc = sizeof(DEVICE_CONFIG);

  apdu.apdu2.config.mode = mode;
  apdu.apdu2.config.crTimeout = DEFAULT_CHAL_TIMEOUT;
  apdu.apdu2.config.autoEjectTime = 0;

  dwRecvLength = sizeof(rAPDU);
  rc = SCardTransmit(hCard, SCARD_PCI_T1, (BYTE *) &apdu, 5 + apdu.apdu2.lc,
		     NULL, rAPDU.buf, &dwRecvLength);

  if (rc == SCARD_S_SUCCESS) {
    dumpHex("\nSCardTransmit [NEO write config]", rAPDU.buf, dwRecvLength);

    // Parse STATUS record again

    printf("\nSeq:           %d\n", rAPDU.select.status.pgmSeq);
    printf("Update %s", (rAPDU.select.status.pgmSeq == (firstSeq + 1)) ?
	   "successful" : "failed");
  } else
    printf("SCardTransmit(2) failed\n");

  // Disconnect and clean up

  SCardDisconnect(hCard, SCARD_LEAVE_CARD);
  SCardReleaseContext(hContext);

  return 0;
}

int
main(int argc, char *argv[])
{
  struct gengetopt_args_info args_info;
  long mode;
  char *endptr;

  if (cmdline_parser (argc, argv, &args_info) != 0)
    return EXIT_FAILURE;

  mode = strtol (args_info.mode_arg, &endptr, 16);
  if (*endptr != '\0'
      || !(mode == 0x00 || mode == 0x01 || mode == 0x02
	   || mode == 0x80 || mode == 0x81 || mode == 0x82))
    {
      fprintf (stderr, "%s: invalid mode: %s\n", argv[0], args_info.mode_arg);
      return EXIT_FAILURE;
    }

  if (modeswitch (args_info.verbose_flag,
		  args_info.yes_flag,
		  mode) != 0)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
