#include "sys.h"  




void delayUsSoftware(unsigned int nus)
{
    unsigned int Delay = nus * 7;//7.2/10
	for(int i = 0;i<Delay;i++)
	{
		__nop();
	}
    //do{__nop();}while (Delay --);
}
void delayMsSoftware(unsigned int nms)
{
    unsigned int Delay = nms * 1000;
    delayUsSoftware(Delay);
}







