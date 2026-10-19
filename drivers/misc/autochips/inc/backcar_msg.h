#ifndef __BACKCAR__MSG__H_
#define __BACKCAR__MSG__H_

/****************************************************************************************/

#define ARM2_STATUS_UNKNOWN         0
#define ARM2_STATUS_BACKING_CAR     1
#define ARM2_STATUS_NO_BACK_CAR     2



typedef enum _BACKCARMSG
{
    MSG_UNKNOWN = 0,
    MSG_ARM2_RESPONSE,
    MSG_ANDROID_APP_READY,
    MSG_ANDROID_STOP_ARM2,
}BACKCARMSG;


/****************************************************************************************/


#endif
