/* v1016i: symbols the rain TU needs that must not clash with host_shim.h's own
 * static Serial. Compiled against the minimal rain shim. */
#include "display/botcorps_display.h"
HostPrintD Serial;
HostESP ESP;
unsigned HostESP::getFreeHeap(){ return 200000; }
unsigned HostESP::getMaxAllocHeap(){ return 180000; }
