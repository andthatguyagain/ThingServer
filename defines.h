#ifndef DEFINES_H
#define DEFINES_H

#define DEFAULT_PORT 50000

#define ACTIVES_PRANGE_START 50001

#define ACTIVES_PRANGE_END 55000

#define CNTRL_PRANGE_START 56000

#define CNTRL_PRANGE_END 57500

#define PING_T 10000 /*ping intetval in ms*/

#define DELIMIT ":"

#define ACTIVE "active"

#define CNTL "cntrl"

#define AUTH "auth"

#define SEND "send"   /** forward a message to a connected client */

#define SRVREQ "request"  /** RESERVED request specific action from server RESERVED  */

#define SWTOG "toggle"  /** toggle switch on/of SWTOG|SWITCH_ID **/

#define SWPULSE "pulse " /** pulse switch, PULSE|SWITCH_ID|DURATION */

#define  TOG 1

#define ON   1

#define OFF  0

#define BRDCAST "broadcast"

#endif // DEFINES_H
