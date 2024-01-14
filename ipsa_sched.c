/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/******************************************************************************
 * NOTE 1: The FreeRTOS demo threads will not be running continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Linux port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Linux
 * port for further information:
 * https://freertos.org/FreeRTOS-simulator-for-Linux.html
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.  Console output
 * is used in place of the normal LED toggling.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 milliseconds (please read the notes
 * above regarding the accuracy of timing under Linux).
 *
 * The Queue Send Software Timer:
 * The timer is an auto-reload timer with a period of two seconds.  The timer's
 * callback function writes the value 200 to the queue.  The callback function
 * is implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 *
 * Expected Behaviour:
 * - The queue send task writes to the queue every 200ms, so every 200ms the
 *   queue receive task will output a message indicating that data was received
 *   on the queue from the queue send task.
 * - The queue send software timer has a period of two seconds, and is reset
 *   each time a key is pressed.  So if two seconds expire without a key being
 *   pressed then the queue receive task will output a message indicating that
 *   data was received on the queue from the queue send software timer.
 *
 * NOTE:  Console input and output relies on Linux system calls, which can
 * interfere with the execution of the FreeRTOS Linux port. This demo only
 * uses Linux system call occasionally. Heavier use of Linux system calls
 * may crash the port.
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY1    ( tskIDLE_PRIORITY  )
#define mainQUEUE_RECEIVE_TASK_PRIORITY2    ( tskIDLE_PRIORITY  )
#define mainQUEUE_RECEIVE_TASK_PRIORITY3    ( tskIDLE_PRIORITY  )
#define mainQUEUE_RECEIVE_TASK_PRIORITY4   ( tskIDLE_PRIORITY  )
#define mainQUEUE_SEND_TASK_PRIORITY       ( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
 * milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS         pdMS_TO_TICKS( 1000UL )
#define mainTIMER_SEND_FREQUENCY_MS        pdMS_TO_TICKS( 2000UL )

/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH                   ( 2 )

/* The values sent to the queue receive task from the queue send task and the
 * queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK           ( 100UL )
#define mainVALUE_SENT_FROM_TIMER          ( 200UL )

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask( void * pvParameters );
static void prvQueueReceiveTask2( void * pvParameters );
static void prvQueueReceiveTask3( void * pvParameters );
static void prvQueueReceiveTask4( void * pvParameters );
static void prvQueueSendTask( void * pvParameters );

/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void ipsa_sched( void )
{
    const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

    /* Create the queue. */
    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

    if( xQueue != NULL )
    {
        /* Start the two tasks as described in the comments at the top of this
         * file. */
        xTaskCreate( prvQueueReceiveTask,             /* The function that implements the task. */
                     "Rx",                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                     configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task. */
                     NULL,                            /* The parameter passed to the task - not used in this simple case. */
                     mainQUEUE_RECEIVE_TASK_PRIORITY1, /* The priority assigned to the task. */
                     NULL );                          /* The task handle is not required, so NULL is passed. */
	
	
	
	
	xTaskCreate( prvQueueReceiveTask2,             /* The function that implements the task. */
                     "Rx",                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                     configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task. */
                     NULL,                            /* The parameter passed to the task - not used in this simple case. */
                     mainQUEUE_RECEIVE_TASK_PRIORITY2, /* The priority assigned to the task. */
                     NULL );                          /* The task handle is not required, so NULL is passed. */
	
	
	xTaskCreate( prvQueueReceiveTask3,             /* The function that implements the task. */
                     "Rx",                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                     configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task. */
                     NULL,                            /* The parameter passed to the task - not used in this simple case. */
                     mainQUEUE_RECEIVE_TASK_PRIORITY3, /* The priority assigned to the task. */
                     NULL );                          /* The task handle is not required, so NULL is passed. */
                     
        xTaskCreate( prvQueueReceiveTask4,             /* The function that implements the task. */
                     "Rx",                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                     configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task. */
                     NULL,                            /* The parameter passed to the task - not used in this simple case. */
                     mainQUEUE_RECEIVE_TASK_PRIORITY4, /* The priority assigned to the task. */
                     NULL );                          /* The task handle is not required, so NULL is passed. */
	
        xTaskCreate( prvQueueSendTask, "TX", configMINIMAL_STACK_SIZE, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );

        /* Create the software timer, but don't start it yet. */
        xTimer = xTimerCreate( "Timer",                     /* The text name assigned to the software timer - for debug only as it is not used by the kernel. */
                               xTimerPeriod,                /* The period of the software timer in ticks. */
                               pdTRUE,                      /* xAutoReload is set to pdTRUE. */
                               NULL,                        /* The timer's ID is not used. */
                               prvQueueSendTimerCallback ); /* The function executed when the timer expires. */

        if( xTimer != NULL )
        {
            xTimerStart( xTimer, 0 );
        }

        /* Start the tasks and timer running. */
        vTaskStartScheduler();
    }

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks	to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

static void prvQueueSendTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
    const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    for( ; ; )
    {
        /* Place this task in the blocked state until it is time to run again.
        *  The block time is specified in ticks, pdMS_TO_TICKS() was used to
        *  convert a time specified in milliseconds into a time specified in ticks.
        *  While in the Blocked state this task will not consume any CPU time. */
        vTaskDelayUntil( &xNextWakeTime, xBlockTime );

        /* Send to the queue - causing the queue receive task to unblock and
         * write to the console.  0 is used as the block time so the send operation
         * will not block - it shouldn't need to block as the queue should always
         * have at least one space at this point in the code. */
        xQueueSend( xQueue, &ulValueToSend, 0U );
    }
}
/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{
    const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

    /* This is the software timer callback function.  The software timer has a
     * period of two seconds and is reset each time a key is pressed.  This
     * callback function will execute if the timer expires, which will only happen
     * if a key is not pressed for two seconds. */

    /* Avoid compiler warnings resulting from the unused parameter. */
    ( void ) xTimerHandle;

    /* Send to the queue - causing the queue receive task to unblock and
     * write out a message.  This function is called from the timer/daemon task, so
     * must not block.  Hence the block time is set to 0. */
    xQueueSend( xQueue, &ulValueToSend, 0U );
}
/*-----------------------------------------------------------*/




static void prvQueueReceiveTask( void * pvParameters )
{
    uint32_t ulReceivedValue;

    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h.  It will not use any CPU time while it is in the
         * Blocked state. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /* To get here something must have been received from the queue, but
         * is it an expected value?  Normally calling printf() from a task is not
         * a good idea.  Here there is lots of stack space and only one task is
         * using console IO so it is ok.  However, note the comments at the top of
         * this file about the risks of making Linux system calls (such as
         * console output) from a FreeRTOS task. */
        if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
        {
            console_print( "Task 1 is working\n" );
        }
        else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
        {
            
            console_print( "Everything is good !\n" );
        }
        else
        {
            console_print( "Unexpected message\n" );
        }
    }
}

static void prvQueueReceiveTask2( void * pvParameters )
{
    uint32_t ulReceivedValue;

    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h.  It will not use any CPU time while it is in the
         * Blocked state. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /* To get here something must have been received from the queue, but
         * is it an expected value?  Normally calling printf() from a task is not
         * a good idea.  Here there is lots of stack space and only one task is
         * using console IO so it is ok.  However, note the comments at the top of
         * this file about the risks of making Linux system calls (such as
         * console output) from a FreeRTOS task. */
        if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
        {
            console_print( "Task 2 is working\n" );
        }
        else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
        {
            int temps_in_fh = 32+rand() % 50;
            
            double temps_in_dg = (temps_in_fh -32)/9.0*5.0;
            console_print( "température en Fahreneit : %d F, conversion en degrée :%2f°C\n",temps_in_fh,temps_in_dg );
        }
        else
        {
            console_print( "Unexpected message\n" );
        }
    }
}


static void prvQueueReceiveTask3( void * pvParameters )
{
    uint32_t ulReceivedValue;

    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h.  It will not use any CPU time while it is in the
         * Blocked state. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /* To get here something must have been received from the queue, but
         * is it an expected value?  Normally calling printf() from a task is not
         * a good idea.  Here there is lots of stack space and only one task is
         * using console IO so it is ok.  However, note the comments at the top of
         * this file about the risks of making Linux system calls (such as
         * console output) from a FreeRTOS task. */
        if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
        {
            console_print( "Task 3 is working\n" );
        }
        else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
        {
            long int a= 519195165119;
            long int b= 784816654984;
            console_print( "a*b =%ld\n",a*b );
        }
        else
        {
            console_print( "Unexpected message\n" );
        }
    }
}

static void prvQueueReceiveTask4( void * pvParameters )
{
    uint32_t ulReceivedValue;

    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h.  It will not use any CPU time while it is in the
         * Blocked state. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /* To get here something must have been received from the queue, but
         * is it an expected value?  Normally calling printf() from a task is not
         * a good idea.  Here there is lots of stack space and only one task is
         * using console IO so it is ok.  However, note the comments at the top of
         * this file about the risks of making Linux system calls (such as
         * console output) from a FreeRTOS task. */
        if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
        {
            console_print( "Task 4 is working\n" );
        }
        else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
        {
            
        int y[50] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
        23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50};
	int search_n = 10;

	    
	int taille = sizeof(y) / sizeof(y[0]) / 2; // taille de la liste :2
	int i = y[taille];
	int compteur = 2;
		
	while (i != search_n) {
	if (i < search_n) {
		taille = taille + taille / compteur;
		i = y[taille];
		compteur++;
	} else if (i>search_n) {
		    
		taille = taille - taille / compteur;
		i = y[taille];
		compteur++;
	}
	}
		       
            console_print( "le nombre %d a été trouvé en %d itérations.\n",i,compteur-1 );
        }
        else
        {
            console_print( "Unexpected message\n" );
        }
    }
}

/*-----------------------------------------------------------*/
