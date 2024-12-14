#include <stdio.h>
#include <stdint.h>
#include <LPC23xx.H>                   
#include "LCD.h"                        
         
float total_sum = 0;
float avg_value = 0;
float sum_total_adc[4] = {0};        
short AD_saved[4];

void init_timer()
{                     
  T0PR 					= 11;
	T0MR0         = 999;                       	/* 1msec = 1000-1 at 1 MHz */
  T0MCR         = 3;                         	/* Interrupt and Reset on MR0  */
  T0TCR         = 1;                        	/* Timer0 Enable */ 
}
 
void init_adc()
{
  PCONP        |= (1 << 12)|(1<<1);             /* Enable power to AD block and timer0   */
  PINSEL1       = 0x154000;                     /* AD0.0 pin function select   */
}

void read_adc(int i, long bit)
{
	AD0CR  = bit;
	AD0CR |= (1 << 24);        /* Start conversion */
	if(i == 0)
	{
		AD_saved[i] = (AD0DR0>> 6) & 0x3FF;
	}
	else if(i == 1)
	{
		AD_saved[i] = (AD0DR1>> 6) & 0x3FF;
	}
	else if(i == 2)
	{
		AD_saved[i] = (AD0DR2>> 6) & 0x3FF;
	}
	else
	{
		AD_saved[i] = (AD0DR3>> 6) & 0x3FF;
	}
	sum_total_adc[i] += AD_saved[i];
}

void reset_sums()
{
	total_sum = 0;
	sum_total_adc[0] = 0;
	sum_total_adc[1] = 0;
	sum_total_adc[2] = 0;
	sum_total_adc[3] = 0;
}

int main (void) 
{
	int count_100ms = 0;
	int count_200ms = 0;
	int count_1s = 0;
	char cVal[10];
		 
	init_timer();
	init_adc();

	//lcd_init();
	//lcd_clear();
	//lcd_print ("LET'S GO!" );

	while (1) 
	{
		if (T0IR & (1 << 0)) 
		{            
			T0IR = (1 << 0);               /* Clear the MR0 interrupt flag */

			count_100ms++;
			count_200ms++;
			count_1s++;

			/* Every 100ms: Read AD0.0, AD0.1, and AD0.3 */
			if (count_100ms % 100 == 0)
			{      
 				read_adc(1, 0x00200002);
 				read_adc(0, 0x00200001);
 				read_adc(3, 0x00200008);
			}

			/* Every 200ms: Read AD0.2 */
			if (count_200ms % 200 == 0)
			{              
				read_adc(2, 0x00200004);
			}

			/* Every 1 second: Calculate and display sum and average */
			if (count_1s % 1000 == 0)
			{
				total_sum += (sum_total_adc[0]+sum_total_adc[1]+sum_total_adc[2]+sum_total_adc[3]);
				avg_value = total_sum / 35.0;

				lcd_clear();
				sprintf(cVal, "%.1f#%.1f#%.1f#%.1f", (sum_total_adc[0]/10.0)*(3.2/1023), (sum_total_adc[1]/10.0)*(3.2/1023), (sum_total_adc[2]/5.0)*(3.2/1023), (sum_total_adc[3]/10.0)*(3.2/1023));
				set_cursor(0, 0);
				lcd_print(cVal);

				sprintf(cVal, "Total Avg: %.2f", avg_value*(3.2/1023));
				set_cursor(0, 1);
				lcd_print(cVal);

				reset_sums();
			}
		}
	}
	return 0;
}