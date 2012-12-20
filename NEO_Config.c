/*****************************************************************************
**																	        **
**		N E O _ C O N F I G  -  PC/SC utility to write NEO config record    **
**																			**
**		Date		/ Rev		/ Sign	/ Remark							**
**		2012-12-01	/ 0.0.0		/ J E	/ Main								**
**																			**
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <neo_if.h>

#pragma pack(push, 1)
#include <ykdef.h>
#pragma pack(pop)

#ifdef __APPLE__
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif

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

static void dumpHex(const char *descr, BYTE *buf, int bcnt)
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

int main(void)
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

    // Establish context and find reader(s)

    rc = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (rc != SCARD_S_SUCCESS) {
        printf("SCardEstablishContext failed, rc=%08lx", rc);
        getchar();
        return 1;
    }

    rc = SCardListReaders(hContext, NULL, NULL, &dwReaders);
    if (rc != SCARD_S_SUCCESS) {
        SCardReleaseContext(hContext);
        getchar();
        return 1;
    }

    if (dwReaders > sizeof(rdrBuf)) dwReaders = sizeof(rdrBuf);

    rc = SCardListReaders(hContext, NULL, rdrBuf, &dwReaders);
    if (rc != SCARD_S_SUCCESS) {
        SCardReleaseContext(hContext);
        printf("SCardListReaders failed, rc=%08lx", rc);
        getchar();
        return 1;
    }

    // Connect to card (if any)

    rc = SCardConnect(hContext, rdrBuf, SCARD_SHARE_SHARED, 
                      SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (rc != SCARD_S_SUCCESS) {
        SCardReleaseContext(hContext);
        printf("SCardConnect failed, rc=%08lx", rc);
        getchar();
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

    // Transmit set config c ommand

    apdu.apdu2.cla = 0;
    apdu.apdu2.ins = INS_YK2_REQ;
    apdu.apdu2.p1 = SLOT_DEVICE_CONFIG;
    apdu.apdu2.p2 = 0;
    apdu.apdu2.lc = sizeof(DEVICE_CONFIG);

    apdu.apdu2.config.mode = MODE_OTP; // | MODE_FLAG_EJECT;
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

    printf("\n\nPress any key to quit...");
    getchar();

    return 0;
}
