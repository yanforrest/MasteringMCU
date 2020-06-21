/* 
 * stm32f407xx_i2c_driver.c
 */

#include "stm32f407xx_i2c_driver.h"

static void I2C_GenerateStartCondition(I2C_RegDef_t * pI2Cx);
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t * pI2Cx, uint8_t SlaveAddr);
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t * pI2Cx, uint8_t SlaveAddr);
static void I2C_ClearADDRFlag(I2C_Handle_t * pI2CHandle));
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t * pI2CHandle);
static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t * pI2CHandle);

static void I2C_GenerateStartCondition(I2C_RegDef_t * pI2Cx)
{
	pI2Cx->CR1 |= ( 1 << I2C_CR1_START );
}

static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t * pI2Cx, uint8_t SlaveAddr)
{
	  
	SlaveAddr = SlaveAddr << 1;
	SlaveAddr &= ~(1);            // SlaveAddr is Slave address + r/nw bit = 0;
	pI2Cx->DR = SlaveAddr;
}

static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t * pI2Cx, uint8_t SlaveAddr)
{
	  
	SlaveAddr = SlaveAddr << 1;
	SlaveAddr |= 1;            // SlaveAddr is Slave address + r/nw bit = 0;
	pI2Cx->DR = SlaveAddr;
}


static void I2C_ClearADDRFlag(I2C_Handle_t * pI2CHandle)
{
	uint32_t dummyRead;
	// check for device mode 
	if(pI2CHandle->pI2Cx->SR2 & ( 1<< I2C_SR2_MSL)){
		// device is in master mode
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX){
			//first disable the ack 
			IW2C_ManageAcking(pI2CHandle->pI2Cx, DISABLE);
			
			// clear the ADDR flag ( read SR1, read SR2)
			dummy_read = pI2Cx->SR1;
			dummy_read = pI2Cx->SR2;
			(void) dummy_read; // this operation prevent warings from the compiler for doing nothing to a variable. 
		}else{
			// clear the ADDR flag ( read SR1, read SR2)
			dummy_read = pI2Cx->SR1;
			dummy_read = pI2Cx->SR2;
			(void) dummy_read; 
		}
	}else {
		//device is in slave mode
					// clear the ADDR flag ( read SR1, read SR2)
			dummy_read = pI2Cx->SR1;
			dummy_read = pI2Cx->SR2;
			(void) dummy_read; 
	}
}
void I2C_GenerateStopCondition(I2C_RegDef_t * pI2Cx)
{
	pI2Cx->CR1 |= ( 1 << I2C_CR1_STOP );
}

void I2C_SlaveEnableDisableCallbackEvents(I2C_RegDef_t *pI2Cx,  uint8_t EnOrDi)
{
		if(EnOrDi == ENABLE)
		{
			pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);
			pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);
			pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
		}else{
			pI2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);
			pI2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);
			pI2Cx->CR2 &= ~(1 << I2C_CR2_ITERREN);
	
		}
		

	
}

/*****************************************
 *   @fn         - I2C_PeripheralControl
 *
 ******************************************/
void I2C_PeripheralControl(I2C_RegDef_t * pI2Cx, uint8_t EnOrDi)
{
		if(EnOrDi == ENABLE)
		{
			pI2Cx->CR1 |= (1 << I2C_CR1_PE);
		}else{
			pI2Cx->CR1 &= ~(1 << 0);
		}
}


/*********************************************************************
 * @fn      		  - I2C_PeriClockControl
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              -

 */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(pI2Cx == I2C1)
		{
			I2C1_PCLK_EN();
		}else if (pI2Cx == I2C2)
		{
			I2C2_PCLK_EN();
		}else if (pI2Cx == I2C3)
		{
			I2C3_PCLK_EN();
		}
	}
	else
	{
		//TODO
	}

}


/*****************************************
 *   @fn         - I2C_Init
 *
 ******************************************/

void I2C_Init(I2C_Handle_t *pI2CHandle)
{
	uint32_t tempreg = 0;
	
	// enable the clock for the i2cx peripheral 
	I2C_PeriClockControl(pI2CHanlde->pI2Cx, ENABLE);
	
	//ack control bit
	tempreg |= pI2CHandle ->I2C_Config.I2C_AckControl << 10;
	pI2CHandle->pI2Cx->CR1 = tempreg;
	
	// configure the FREQ field of CR2
	tempreg = 0;
	tempreg |= RCC_GetPCLK1Value() / 1000000U;
	pI2CHandle->pI2Cx->CR2 = (tempreg & 0x3F);
	
	// program the device own address
	tempreg = 0;
	tempreg |= pI2CHandle->I2C_Config.I2C_DeviceAddress << 1;
	tempreg |= (1 << 14);
	pI2CHandle->pI2Cx->OAR1 = tempreg;
	
	
	// CCR calculations
	uint16_t ccr_value = 0;
	tempreg = 0;
	if(pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM){
		// mode is slow mode
		ccr_value = RCC_GetPCLK1Value()/(2 * pI2CHandle->I2C_Config.I2C_SCLSpeed);
		tempreg |= (ccr_value * 0xFFF);
	}else{
		//mode is fast mode
		tempreg |= ( 1<<15 );
		tempreg |= ( pI2CHandle->I2C_Config.I2C_FMDutyCycle << 14 );
		if(pI2CHandle->I2C_Config.I2C_FMDutyCycle == I2C_FM_DUTY_2){
			ccr_value =  RCC_GetPCLK1Value()/(3 * pI2CHandle->I2C_Config.I2C_SCLSpeed);
		}else{
			ccr_value =  RCC_GetPCLK1Value()/(25 * pI2CHandle->I2C_Config.I2C_SCLSpeed);
		}
		tempreg |= (ccr_value * 0xFFF);
	}
	pI2CHandle->pI2Cx->CCR = tempreg;
	
	// TRISE Configuration 
	if(pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM){
		// mode is slow mode
		tempreg = ( RCC_GetPCLK1Value() / 1000000U ) + 1;
	}else{
		// mode is fast mode 
		tempreg = ( (RCC_GetPCLK1Value() * 300 ) / 1000000000U ) + 1;
	}
	pI2CHandle->pI2Cx->TRISE = (tempreg & 0x3F);
	
}

/*****************************************
 *   @fn         - I2C_IDenit
 *
 ******************************************/

void I2C_DeInit(I2C_Handle_t *pI2CHandle)
{
	
	
}


uint8_tI2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)
{
	if(pI2Cx->SR1 & FlagName){
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*****************************************
 *   @fn         - I2C_MasterSendData
 *
 ******************************************/

void I2C_MasterSendData(I2C_Handle_t * pI2CHandle, uint8_t *pTxBuffer, uint32_t Len, uint8_t SlaveAddr, unit8_t Sr )
{
	 // 1. Generate the START condition
	 I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
	 
	 // 2. confirm that start generation is completed by checking the SB flag in the SR1
	 //    Note: Until SB is cleared SCL will be stretched (pulled to LOW)
	 while(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB));
	 
	 // 3. Send the address of the slave with r/nw bit set to W(0) ( total 8 bits )
	 I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr)
	 
	 // 4. Confirm that address phase is completed by checking the ADDR flag in the SR1
	  while(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR));
	  
	 // 5. clear the ADDR flag according to its software sequence
	 //    Note: Until ADDR is cleared SCL will be stretched ( pulled to LOW)
	 I2C_ClearADDRFlag(pI2CHandle);
	 
	 // 6. send the data until Len becomes 0 
	 while(Len > 0)
	 {
	 		while(!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE)); // Wait until TXE is set
	 		pI2CHandle->pI2Cx->DR = *pTxBuffer;
	 		pTxBuffer++;
	 		Len--;
	 }
	 // 7. when Len becomes zero wait for TXE=1 and BTF =1 before generating the STOP condition
	 //    Note: TXE = 1, BTF = 1, means that both SR and DR are empty and next transmission should begin 
	 //    When BTF=1 SCL will be stretched ( pulled to LOW)
	 while(!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE)); 
	 while(!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_BTF)); 
	 // 8. Generate STOP condition and master need not to wait for the completion of stop condition. 
	 //    Note: generating STOP, automatically clears the BTF
	 if(Sr == I2C_DISABLE_SR)
	 	I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
	 
}

/*****************************************
 *   @fn         - I2C_MasterReceiveData
 *
 ******************************************/
void I2C_MasterReceiveData(I2C_Handle_t * pI2CHandle, uint8_t *pRxBuffer, uint32_t Len, uint8_t SlaveAddr, unit8_t Sr )
{
		// 1. Generate the START condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
		
		// 2. confirm that start generation is completed by checking the SB flag in the SR1
		//    Note: Until SB is cleared SCL will be stretched (pulled to LOW)
		while(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB));
		
		// 3. send the address of the slave with r/nw bit set to R(1) (total 8 bits0
		I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr);
		
		// 4. wait until address phase is completed by checking the ADDR flag in the SR1
		while(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR));
		 
		//procedure to read only 1 byte from slave
		if( Len == 1)
		{
			//Disable Acking 
			I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);
			
		
			//Clear the ADDR flag 
			I2C_ClearADDRFlag(pI2CHandle);
			 
			// wait until RXNE  becomes 1
			while( !I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE) ); 

			// generate STOP condition 
			if(Sr == I2C_DISABLE_SR)
				I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
	
			// read data into buffer
			*pRxBuffer = pI2CHandle->pI2Cx->DR;
			
		}
		
		//procedure to read more than 1 byte from slave
		if( Len > 1){
				// clear the ADDR flag
				I2C_ClearADDRFlag(pI2CHandle);
				
				
				//read the data until Len becomes zero 
				for ( uint32_t i = Len;  i> 0; i--)
				{
					//wait until RXNE becomes 1
					while( !I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE) );
					
					if(i ==2) // if last 2 buytes are remaining
					{
							//Disable Acking 
							I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);
							
							// generate STOP condition 
							if(Sr == I2C_DISABLE_SR)
								I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
						
					}
					//read the data from data register in to buffer
					*pRxBuffer = pI2CHandle->pI2Cx->DR;
					
					//increment the buffer address
					pRxBuffer++;
				}
				
		}
		
		//re-enable ACKing
		if( pI2CHandle->I2C_Config.I2C_AckControl == I2C_ACK_ENABLE){
			I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
		}
		
}


void I2C_ManageAcking(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
		if(EnorDi==I2C_ACK_ENABLE){
			// enable the ack 
			pI2Cx->CR1 |= ( 1 << I2C_CR1_ACK );
			
		}else{
			// disable the ack 
			pI2Cx->CR1 &= ~( 1 << I2C_CR1_ACK );
		}
			
}



void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{

	if(EnorDi == ENABLE)
	{
		if(IRQNumber <= 31)
		{
			//program ISER0 register
			*NVIC_ISER0 |= ( 1 << IRQNumber );

		}else if(IRQNumber > 31 && IRQNumber < 64 ) //32 to 63
		{
			//program ISER1 register
			*NVIC_ISER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96 )
		{
			//program ISER2 register //64 to 95
			*NVIC_ISER3 |= ( 1 << (IRQNumber % 64) );
		}
	}else
	{
		if(IRQNumber <= 31)
		{
			//program ICER0 register
			*NVIC_ICER0 |= ( 1 << IRQNumber );
		}else if(IRQNumber > 31 && IRQNumber < 64 )
		{
			//program ICER1 register
			*NVIC_ICER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 6 && IRQNumber < 96 )
		{
			//program ICER2 register
			*NVIC_ICER3 |= ( 1 << (IRQNumber % 64) );
		}
	}

}


void I2C_IRQPriorityConfig(uint8_t IRQNumber,uint32_t IRQPriority)
{
	//1. first lets find out the ipr register
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section  = IRQNumber %4 ;

	uint8_t shift_amount = ( 8 * iprx_section) + ( 8 - NO_PR_BITS_IMPLEMENTED) ;

	*(  NVIC_PR_BASE_ADDR + iprx ) |=  ( IRQPriority << shift_amount );

}



/**********************************************************
 *   @fn         - I2C_MasterSendDataIT
 *
 **********************************************************/

uint8_t I2C_MasterSendDataIT(I2C_Handle_t * pI2CHandle, uint8_t *pTxBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr );
{
	uint8_t busystate = pI2CHandle->TxRxState;
	if((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX)){
		pI2CHandle->pTxBuffer = pTxBuffer ;
		pI2CHandle->TxLen = Len ;
		pI2CHandle->TxRxState = I2C_BUSY_IN_TX ; //Rxsize is used in the ISR code to manage the data reception
		pI2CHandle->DevAddr = SlaveAddr ;
		pI2CHandle->Sr = Sr;
		
		//Implement code to Generate START Condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
		
		//Implement code to enable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2  |= (1 << I2C_CR2_ITBUFEN);
		
		//Implement code to enable ITEVTEN Control Bit
		pI2CHandle->pI2Cx->CR2  |= (1 << I2C_CR2_ITEVTEN);
		
		//Implement code to enable ITERREN Control Bit
		pI2CHandle->pI2Cx->CR2  |= (1 << I2C_CR2_ITERREN);
	} 	
	return busystate;
}
/**********************************************************
 *   @fn         - I2C_MasterReceiveDataIT
 *
 ***********************************************************/
 
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t * pI2CHandle, uint8_t *pRxBuffer, uint32_t Len, uint8_t SlaveAddr, unit8_t Sr );
{
	uint8_t busystate = pI2CHandle->TxRxState;
	if((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX)){
		pI2CHandle->pRxBuffer = pRxBuffer ;
		pI2CHandle->RxLen = Len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_RX ; //Rxsize is used in the ISR code to manage the data reception
		pI2CHandle->RxSizer = Len;
		pI2CHandle->DevAddr = SlaveAddr;
		pI2CHandle->Sr = Sr;
		
		//Implement code to Generate START Condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
		
		//Implement code to enable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2  |= (1 << I2C_CR2_ITBUFEN);
		
		//Implement code to enable ITEVFEN Control Bit
		pI2CHandle->pI2Cx->CR2  |= (1 << I2C_CR2_ITEVTEN)
		
		//Implement code to enable ITERREN Control Bit
		pI2CHandle->pI2Cx->CR2  |= (1 << I2C_CR2_ITERREN);
	} 
	return busystate;
}


static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t * pI2CHandle)
{
	if(pI2CHandle->TxLen > 0){
		//1. load the data into DR
		pI2CHandle->DR = *(pI2CHandle->pTxBuffer);
		//2. decrement the TxLen	
		pI2CHandle->TxLen-- ; 
		//3. Increment the buffer address	
		pI2CHandle->TxBuffer++; 		
	}
}

static  void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t * pI2CHandle)
{
	//we have to do the data reception
	if(pI2CHandle->RxSize == 1){
		*pI2CHandle->pRxBuffer = pI2CHandle->pI2Cx->DR;
		pI2CHandle->RxLen--;			
	}
				
	if(pI2CHandle->RxSize > 1){
		if(pI2CHandle->RxLen == 2){
			// clear the ack bit 
			I2C_ManageAcking(pI2CHandle->pI2Cx, DISABLE);
		}
					
		// read DR
		*pI2CHandle->pRxBuffer = pI2CHandle->pI2Cx->DR;
		pI2CHandle->pRxBuffer++;
		pI2CHandle->RxLen--;			
	}
				
	if(pI2CHandle->RxLen == 0){
		// close the I2C data reception and notify the application
					
		// 1. generate the stop condition
		if(pI2CHandle->Sr == I2C_DISABLE_SR)
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
						
		//2. Close the I2C rx
		I2C_CloseReceiveData(pI2CHandle);
					
		//3. Notify the application 
		I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_RX_CMPLT);
	}
}


void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
	//Implement the code to diable ITBUFEN Control Bit 
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);
	
	//Implement the code to diable ITEVFEN Control Bit 
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVFEN);
	 
	pI2CHandle->pI2Cx->TxRxState = I2C_READY;
	pI2CHandle->pI2Cx->pRxBuffer = NULL:
	pI2CHandle->pI2Cx->RxLen = 0;
	pI2CHandle->pI2Cx->RxSize = 0;
	if(pI2CHandle->I2C_Config.I2C_AckControl == I2C_ACK_ENABLE)
		I2C_ManageAcking(pI2CHandle->pI2Cx, ENABLE);
}

void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
	//Implement the code to diable ITBUFEN Control Bit 
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);
	
	//Implement the code to diable ITEVFEN Control Bit 
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVFEN);
	 
	
	pI2CHandle->pI2Cx->TxRxState = I2C_READY;
	pI2CHandle->pI2Cx->pTxBuffer = NULL:
	pI2CHandle->pI2Cx->TxLen = 0;
}


void I2C_SlaveSendData(I2C_RegDef_t *pI2C, uint8_t data)
{
	pI2C->DR = data; 
}
void I2C_SlaveReceiveData(I2C_RegDef_t *pI2C);
{
	return (uint8_t) pI2C->DR;
}


void I2C_EV_IRQHandling(I2C_Handle_t * pI2CHandle)
{
  
	// Interrupt handling for both master and slave mode of a device
	uint32_t temp1, temp2, temp3; 
	
	temp1 = pI2CHandle->pI2Cx->CR2 & ( 1 << I2C_CR2_ITEVTEN );
	temp2 = pI2CHandle->pI2Cx->CR2 & ( 1 << I2C_CR2_ITBUFEN );

	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_SB);	
	// 1. Handle for interrupt generated by SB event
	// Note: SB flag is only applicable in Master mode 
	if( temp1 & temp3){
		//The interrupt is generated because of SB event 
		//This block will not be executed in slave mode because for slave SB is always zero 
		// In this block lets executed the address phase
		if(pI2CHandle->TxRxState==I2C_BUSY_IN_TX){
			I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, pI2CHandle->DevAddr);
		} else if(pI2CHandle->TxRxState==I2C_BUSY_IN_RX){
			I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, pI2CHandle->DevAddr);
		}		
	}
	
  temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << U2C_SR1_ADDR );	
	// 2. Handle for interrupt generated by ADDR event 
	// Note: When master mode : address is sent 
	//       When slave mode  : address matched with own address
	if( temp1 & temp3){
		//ADDR flag is set
		I2C_ClearADDRFlag(pI2CHandle);
	}	
	
	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_BTF );	
	// 3. Handle for interrupt generated by BTF( Byte Transfer Finished) event 
	if( temp1 & temp3){
		//BTF flag is set
		if(pI2CHandle->TxRxState==I2C_BUSY_IN_TX){
			// make sure that TXE is also set.
			if(pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_TXE))
			{
				//BTF, TXE=1
				if(pI2CHandle->TxLen == 0){
					//1. generate the STOP condition.
					if(pI2CHandle->Sr== I2C_DISABLE_SR)
						I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
					//2. reset all the member elements of the handle structure. 
					I2C_CloseSendData(pI2CHandle);
					//3. notify the application about transmission complete
					I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
				}
			}
			
		} else if(pI2CHandle->TxRxState==I2C_BUSY_IN_RX){
			;
		}		
	}
		
	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << U2C_SR1_STOPF );	
	// 4. Handle for interrupt generated by STOPF event 
	// Note : Stop detection flage is applicable only slave mode . For master this flag will never be set
	// The below code block will not be executed by master since STOPF will not set in master mode
	if( temp1 & temp3){
		//STOPF flag is set
		//Clear the STOPF (i.e.)  1) read SR1 2) Write to CR1
		 pI2CHandle->pI2Cx->CR1 |= 0x0000;
		 //Notify the application that STOP is detected
		 I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_STOP);
		
	}
	
	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << U2C_SR1_TXE );
	// 5. Handle for interrupt generated by TXE event 
	if( temp1 & temp2 & temp3){
		// check for device mode 
		// make sure the device in slave mode
		if( pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_MSL )){
			// for master
			//TXE flag is set
			// we have to do the data transmission
			if(pI2CHandle->TxRxState==I2C_BUSY_IN_TX){
				I2C_MasterHandleTXEInterrupt(pI2CHandle);
			}
		}else{
	    // for slave
	    //make sure that the slave is really in transmitter mode
	    if( 	pI2CHandle->pI2CVx->SR2 &(1<<I2C_SR2_TRA)){
	    	I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_REQ);
	    }
		}
	}	
	
	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_RXNE );
	// 6. Handle for interrupt generated by RXNE event
	if( temp1 & temp2 & temp3){
		//check device mode. 
		if(pI2CHandle->pI2Cx->SR2 & (1<< I2C_SR2_MSL)){
			// the device is master
			
			//RXNE flag is set
			if(pI2CHandle->TxRxState ==I2C_BUSY_IN_RX){
				I2C_MasterHandleRXNEInterrupt(pI2CHandle);
			}
		}else{
	    // for slave
	    //make sure that the slave is really in receive mode
	    if(!(pI2CHandle->pI2CVx->SR2 &(1<<I2C_SR2_TRA))){
	    	I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_RCV);
	    }
		}
	}		
}


void I2C_ER_IRQHandling(I2C_Handle_t * pI2CHandle)
{
	uint32_t temp1, temp2;
	
	//Know the status of ITERREN contrl bit in the CR2
	temp2 = (pI2CHandle->pI2Cx->CR2) & ( 1 << I2C_CR2_ITERREN);
	
	/**************check for Bus error ***************************/
	temo1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_BERR);
	if (temp1 && temp2) {
		// This is Bus error
		
		//Implement the code to clear the buses error flag
		pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_BERR);
		// Implement the code to notify the application about the error
		I2C_ApplicationEventCallback( pI2CHandle, I2C_ERROR_BERR);
	}
	
	/************** check for arbitration lost error **************/
		temo1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_ARLO);
	if (temp1 && temp2) {
		
	}
		
}
