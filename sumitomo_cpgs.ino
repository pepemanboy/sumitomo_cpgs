// CONFIGURABLE DEFINES
// #define MASTER
#define DEBUG

#ifdef MASTER
#include "sumitomo_cpgs_master.h"
CPG_Master cpg;
#else
#include "sumitomo_cpgs_slave.h"
CPG_Slave cpg;
#endif

void setup()
{
  cpg.setup();
}

void loop()
{
  cpg.loop();
}

