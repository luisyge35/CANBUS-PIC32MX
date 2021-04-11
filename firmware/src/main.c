#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include <sys/kmem.h>

#define ADC_PIN_MAP(pin)

typedef enum{
    IDLE = 0,
    RUNNING = 1,
    ERROR = 2
} SMState;

#define TICKMILISECOND  (CORE_TIMER_FREQUENCY/1000)*50 
#define BAMOCAR_RX_ADDR (0x182)
#define TORQUE_REGID    (0x30)
#define ACC_1           (ADC_INPUT_POSITIVE_AN23)
#define ACC_2           (ADC_INPUT_POSITIVE_AN27)
#define ADC_VREF        (3.3f)
#define ADC_MAX_COUNT   (1023U)

// Private functions
static void SetState(uint8_t newState);
static void HandleCanMessage(void);
void CAN_1_Handler( void );
uint32_t ADCGet(uint8_t channel);

volatile uint16_t acceleratorValue;
volatile uint16_t acceleratorValue2;
volatile float Acc1,Acc2;
volatile int8_t Acc1Deg, Acc2Deg;
float r;
int8_t t;
uint8_t data[8] = {0};
uint32_t tick = 0;
uint32_t tickGet = 0;
volatile uint8_t state = IDLE;
static CAN_RX_MSG can1RxMsg[2][32];
CAN_TX_RX_MSG_BUFFER *rxMessage = NULL;

void __ISR(_CAN_1_VECTOR, ipl1SOFT) CAN_1_Handler (void)
{
    HandleCanMessage();
    CAN1_InterruptHandler();
}


void HandleCanMessage(){
    rxMessage = (CAN_TX_RX_MSG_BUFFER *)PA_TO_KVA1(C1FIFOUA1);
    if (rxMessage->msgData[0] == 0xFF){
        SetState(RUNNING);
    }
}

void SetState(uint8_t newState){
    state = newState;
}
    
uint32_t ADCGet(uint8_t channel){
    
    ADC_InputSelect(ADC_MUX_A, channel, ADC_INPUT_NEGATIVE_VREFL);
    ADC_SamplingStart();
    CORETIMER_DelayMs(10);
    ADC_ConversionStart();
    while(!ADC_ResultIsReady());
    return ADC_ResultGet(ADC_RESULT_BUFFER_0);
}

int main ( void )
{
    SYS_Initialize ( NULL );

    while ( true )
    {
        switch(state){
            case IDLE:
                break;
            case RUNNING:
                if (CORETIMER_CounterGet() - tick > TICKMILISECOND){
                    tick = CORETIMER_CounterGet();
                    
                    // Get ADC Values
                    acceleratorValue = (uint16_t)ADCGet(ACC_1);
                    acceleratorValue2 = (uint16_t)ADCGet(ACC_2);
                    
                    // Translate to volts
                    Acc1 = (float)acceleratorValue * ADC_VREF / ADC_MAX_COUNT;
                    Acc2 = (float)acceleratorValue2 * ADC_VREF / ADC_MAX_COUNT;
                    
                    // Translate to degrees
                    Acc1Deg = (int8_t)(68.18*Acc1-112.5);
                    r = (int8_t)(45.0*Acc1-112.5);
                    t = (int8_t)r;
                    Acc2Deg = (int8_t)(68.18*Acc2-112.5);
                    LED_Toggle();
                    
                    /*data[0] = (uint8_t)((acceleratorValue & 0xFF00) >> 8);
                    data[1] = (uint8_t)(acceleratorValue & 0x00FF);
                    data[2] = (uint8_t)((acceleratorValue2 & 0xFF00) >> 8);
                    data[3] = (uint8_t)(acceleratorValue2 & 0x00FF);*/
                    
                    data[0] = Acc1Deg;
                    data[1] = Acc2Deg;
                    /*data[2] = (uint8_t)((Acc2Deg & 0xFF00) >> 8);
                    data[3] = (uint8_t)(Acc2Deg & 0x00FF);*/
                    CANSendBuffer(BAMOCAR_RX_ADDR, 5, TORQUE_REGID, data);
                }
                break;
            default: 
                break;
        }

        /*
        CORETIMER_DelayMs(100);
        ADCGet(ACC_1);
        
        data[0] = (uint8_t)((acceleratorValue & 0xFF00) >> 8);
        data[1] = (uint8_t)(acceleratorValue & 0x00FF);
        data[2] = (uint8_t)((acceleratorValue2 & 0xFF00) >> 8);
        data[3] = (uint8_t)(acceleratorValue2 & 0x00FF);
        CANSendBuffer(BAMOCAR_RX_ADDR, 5, TORQUE_REGID, data);
        
        LED_Toggle();*/
        
        /*if(C1FIFOINT1bits.RXNEMPTYIF == 1){
            //CANSendBuffer(BAMOCAR_RX_ADDR, 3, TORQUE_REGID, data);
            LED_Toggle();
            C1FIFOCON1SET = 0x2000;
        }*/
        
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

