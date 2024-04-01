;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988-1990
;
;   Title:	VMPoll.Inc - Public services for VMPoll
;
;   Version:	1.00
;
;   Date:	30-Nov-1988
;
;   Author:	ARR
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   30-Nov-1988 ARR Original
;
;==============================================================================

Begin_Service_Table VMPoll

VMPoll_Service	VMPoll_Get_Version, LOCAL
VMPoll_Service	VMPoll_Enable_Disable, LOCAL

End_Service_Table VMPoll
