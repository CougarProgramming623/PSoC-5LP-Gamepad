/******************************************************************************
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of
the License, or (at your option) any later verison.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MECHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA
***************************************************************************** */

/*****************************************************************************
*  Project Name: CyController1
*  Project Revision: 1.00
*  Software Version: PSoC Creator 3.2 SP1
*  Device Tested: CY8C5888LTI-LP097
*  Compilers Tested: ARM GCC
*  Related Hardware: CY8CKIT-059
***************************************************************************** */
/* Project Description:
 * This project will create a simple USB HID joystick for FRC. It has
 * 8 analog inputs
 * 22 digital inputs for buttons
 * 12 digital indicator outputs
***************************************************************************** */

#include <project.h>
#include <stdio.h>

/* 8 bytes for analog axis data */
#define ANALOG_SIZE 3
/* 4 bytes for 32 button inputs */
#define BUTTON_SIZE 4
/* 1 byte for 2 output indicators such as LEDs */
#define OUTPUT_SIZE 1

/* Endpoint 1 is IN (analog and buttons) */
#define IN_EP  1
/* Endpiont 2 is OUT (indicators) */
#define OUT_EP 2

void StartUp (void); 		/* Function prototype for component startup API */
void ReadAnalog (void);     /* Function prototype for reading and altering data for joystick information */
void ReadButtons (void);    /* Function prototype for reading and altering data for digital button information */
void SetOutputs (void);     /* Prototype for driving digital outputs */

static uint16 AnalogRaw[ANALOG_SIZE];   /* Raw ADC measurements */
static int16  AnalogData[ANALOG_SIZE];   /* Scaled ADC measurements */
static int8 USB_Input_Data[ANALOG_SIZE + BUTTON_SIZE]; /* USB data to send to the PC */
static uint8 USB_Output_Data[OUTPUT_SIZE]; /* USB data received from the PC */

int main()
{    
    uint16 outCount;
    
    StartUp(); 	/* Calls the proper start API for all the components */
	
	for(;;)
    {
    	while(!USBFS_1_bGetEPAckState(IN_EP)); 	/* Wait for ACK before loading data */
        
		ReadAnalog();	/* Calls function to read analog inputs */
        ReadButtons();	/* Calls function to monitor button presses */
        
        /*Check to see if the IN Endpoint is empty. If so, load it with Input data to be tranfered */
        if(USBFS_1_GetEPState(IN_EP) == USBFS_1_IN_BUFFER_EMPTY)
        {
		    /* Load latest analog and button data into the IN EP and send */
            USBFS_1_LoadInEP(IN_EP, (uint8 *)USB_Input_Data, sizeof(USB_Input_Data));   
        }
        
        /* Check to see if the OUT Endpoint is full from a recieved transaction. */
        if(USBFS_1_GetEPState(OUT_EP) == USBFS_1_OUT_BUFFER_FULL)
        {
            /* Get the number of bytes recieved */
            outCount = USBFS_1_GetEPCount(OUT_EP);
            /* Read the OUT endpoint and store data in OUT_Data_Buffer */
            USBFS_1_ReadOutEP(OUT_EP, USB_Output_Data, outCount);
            /* Re-enable OUT endpoint */
            USBFS_1_EnableOutEP(OUT_EP);
        }
        
        SetOutputs();   /* Set output values based on the data received from the out EP */
    }
}

void StartUp (void)
{
	uint8 i;
    
    /* Intialize USB Input EP data array */
    for (i=0; i < sizeof(USB_Input_Data); i++)
    {
        USB_Input_Data[i] = 0;
    }
    /* Intialize USB Output EP data array */
    for (i=0; i < sizeof(USB_Output_Data); i++)
    {
        USB_Output_Data[i] = 0;
    }
    
    CYGlobalIntEnable;           					/* Enable Global interrupts */
    ADC_Start();        					        /* Initialize ADC */
	ADC_StartConvert();								/* End ADC conversion */    
	USBFS_1_Start(0, USBFS_1_DWR_VDDD_OPERATION);	/* Start USBFS operation/device 0 and with 5V operation */
	while(!USBFS_1_bGetConfiguration())
    {
        /* Wait for Device to enumerate */
    }			
    
    USBFS_1_LoadInEP(IN_EP, (uint8 *)USB_Input_Data, sizeof(USB_Input_Data)); /* Loads an inital value into EP1 and sends it out to the PC */
    USBFS_1_EnableOutEP(OUT_EP); /* Enable the output endpoint */
}

void ReadAnalog (void)
{
	uint8 i;
    
    for(i=0; i<ANALOG_SIZE; i++)
    {
        AnalogRaw[i] = ADC_GetResult16(i) >> 4;     /* Get ADC reading and keep just the 8 upper bits */
        AnalogData[i] = AnalogRaw[i] - 127;         /* Adjust axis to center the reading */
        if(AnalogData[i] > 127)
        {
            AnalogData[i] = 127;
        }
        if(AnalogData[i] < -127)
        {
            AnalogData[i] = -127;
        }
        
        /* Move analog data to USB array to prepare for sending */
        USB_Input_Data[i] = AnalogData[i];
    }
}

void ReadButtons (void)
{
	/* Read button values from status registers into USB array to prepare for sending */
    USB_Input_Data[ANALOG_SIZE]   = ButtonReg1_Read(); /* First button byte is after the last analog value */
    USB_Input_Data[ANALOG_SIZE+1] = ButtonReg2_Read();
    USB_Input_Data[ANALOG_SIZE+2] = ButtonReg3_Read();
    USB_Input_Data[ANALOG_SIZE+3] = ButtonReg4_Read();
}

void SetOutputs (void)
{
    /* Write data received from USB to the output control registers */   
    OutputReg1_Write(USB_Output_Data[0]);
    //OutputReg2_Write(USB_Output_Data[1]);
}

/* End of File */

