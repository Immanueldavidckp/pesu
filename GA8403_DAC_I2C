/*@!Encoding:1252*/
variables
{
  /* --------------------------------------------------
     1. TIMERS & MESSAGES
  -------------------------------------------------- */
  msTimer timerVCU;
  msTimer timerIPC;
  msTimer timerBCM;
  
  // NEW: Timer specifically for the 500ms Fault Memory Test
  msTimer bcmTimeoutTestTimer; 
  
  const int CYCLE_VCU = 20;
  const int CYCLE_IPC = 100;
  const int CYCLE_BCM = 100;

  message VCU15 msgVCU;                 
  message IPC_HU_RC7 msgIPC;            
  message BCM_HUICTrailer_RC1 msgBCM;   

  /* --------------------------------------------------
     2. PANEL-READY VARIABLES
  -------------------------------------------------- */
  // VCU Physical Values
  int val_VCUPositionDisplay = 0x80; // Defaults to Park
  
  // IPC Physical Values
  int val_IllumOnOffCtrlStatus = 0;
  int val_IllumOnOffCtrl       = 1;
  int val_IllumDimLvlStatus    = 0;
  int val_IllumDimLvl          = 100;

  // BCM Physical Values
  int val_PowerModeStateStatus      = 0;
  int val_PowerModeState            = 6;
  int val_PositionLmpTelltaleStatus = 0;
  int val_PositionLmpTelltale       = 1;

  /* --------------------------------------------------
     3. TRANSMISSION STATE FLAGS (1 = ON, 0 = OFF)
  -------------------------------------------------- */
  int txActive_VCU = 1;
  int txActive_IPC = 1;
  int txActive_BCM = 1;

  /* --------------------------------------------------
     4. INTERNAL COUNTERS
  -------------------------------------------------- */
  byte vcuAliveCnt = 0;
  byte expectedPrndCnt = 0; 
}

/* ==================================================
   INITIALIZATION
================================================== */
on start
{
  write("=== Rest of Bus Simulator Started ===");
  setTimer(timerVCU, CYCLE_VCU);
  setTimer(timerIPC, CYCLE_IPC);
  setTimer(timerBCM, CYCLE_BCM);
}

/* ==================================================
   CYCLIC TRANSMISSION LOGIC
================================================== */
on timer timerVCU
{
  byte msgData[7];
  
  if (txActive_VCU == 1)
  {
    msgVCU.VCUGSAPositionDisplay = val_VCUPositionDisplay;
    msgVCU.VCUGSARangeLockReq = 0;
    
    msgVCU.VCUGSAPosAliveCounter = vcuAliveCnt;
    vcuAliveCnt++;
    if(vcuAliveCnt > 15) vcuAliveCnt = 0;

    msgData[0] = msgVCU.byte(0); msgData[1] = msgVCU.byte(1);
    msgData[2] = msgVCU.byte(2); msgData[3] = msgVCU.byte(3);
    msgData[4] = msgVCU.byte(4); msgData[5] = msgVCU.byte(5);
    msgData[6] = msgVCU.byte(6);
    msgVCU.VCUGSAPosChecksum = CalcCRC8(msgData, 7);

    output(msgVCU);
    setTimer(timerVCU, CYCLE_VCU);
  }
}

on timer timerIPC
{
  if (txActive_IPC == 1)
  {
    msgIPC.IllumOnOffCtrlStatus     = val_IllumOnOffCtrlStatus;
    msgIPC.IllumOnOffCtrl           = val_IllumOnOffCtrl;
    msgIPC.IllumDimLvlSettingStatus = val_IllumDimLvlStatus;
    msgIPC.IllumDimLvlSetting       = val_IllumDimLvl;
    
    output(msgIPC);
    setTimer(timerIPC, CYCLE_IPC);
  }
}

on timer timerBCM
{
  if (txActive_BCM == 1)
  {
    msgBCM.PowerModeStateStatus      = val_PowerModeStateStatus;
    msgBCM.PowerModeState            = val_PowerModeState;
    msgBCM.PositionLmpTelltaleStatus = val_PositionLmpTelltaleStatus;
    msgBCM.PositionLmpTelltale       = val_PositionLmpTelltale;
    
    output(msgBCM);
    setTimer(timerBCM, CYCLE_BCM);
  }
}

/* ==================================================
   DYNAMIC VCU FEEDBACK & MONITORING (TC007)
================================================== */
on message PRND_CSM1 // Listens to ID 0x189
{
  /* 1. Update VCU Gear State dynamically based on PRND Request */
  if(this.PRNDCombiSwPosReqStatus == 1) // 1 = Plausible
  {
    switch(this.PRNDCombiSwPosReq)
    {
      case 0: val_VCUPositionDisplay = 0x80;  break; // Park
      case 1: val_VCUPositionDisplay = 0x100; break; // Reverse
      case 2: val_VCUPositionDisplay = 0x200; break; // Neutral
      case 3: val_VCUPositionDisplay = 0x400; break; // Drive
      case 5: val_VCUPositionDisplay = 0x80;  break; // Park Repeat
      case 6: val_VCUPositionDisplay = 0x100; break; // Reverse Repeat
      case 7: val_VCUPositionDisplay = 0x200; break; // Neutral Repeat
      case 8: val_VCUPositionDisplay = 0x400; break; // Drive Repeat
    }
  }

  /* 2. Monitor PRND Sequence Counter */
  if (this.MSG_CNT_31 != expectedPrndCnt)
  {
    write("[WARNING] PRND_CSM1 Counter out of sync! Expected: %d, Received: %d", expectedPrndCnt, this.MSG_CNT_31);
  }
  
  expectedPrndCnt = this.MSG_CNT_31 + 1;
  if (expectedPrndCnt > 15) expectedPrndCnt = 0;
}

/* ==================================================
   KEYBOARD CONTROLS FOR TIMEOUT TESTING
================================================== */
on key 'v' { txActive_VCU = 0; cancelTimer(timerVCU); write("[TIMEOUT] VCU Stopped."); }
on key 'c' { txActive_VCU = 1; setTimer(timerVCU, CYCLE_VCU); write("[RESUME] VCU Started."); }

on key 'i' { txActive_IPC = 0; cancelTimer(timerIPC); write("[TIMEOUT] IPC Stopped."); }
on key 'o' { txActive_IPC = 1; setTimer(timerIPC, CYCLE_IPC); write("[RESUME] IPC Started."); }

on key 'b' { txActive_BCM = 0; cancelTimer(timerBCM); write("[TIMEOUT] BCM Stopped."); }
on key 'n' { txActive_BCM = 1; setTimer(timerBCM, CYCLE_BCM); write("[RESUME] BCM Started."); }

// NEW: Automated sequence to guarantee Fault Logging
on key 't' 
{ 
  txActive_BCM = 0; 
  cancelTimer(timerBCM); 
  write("[TEST START] BCM Stopped. Waiting 600ms for DUT to log the DTC..."); 
  setTimer(bcmTimeoutTestTimer, 600); // 500ms test requirement + 100ms buffer
}

on timer bcmTimeoutTestTimer
{
  write("[TEST READY] 600ms elapsed. BCM timeout should now be confirmed in DUT fault memory.");
  write("[TEST READY] Read DTCs now using UDS (e.g., 19 02 09). Press 'n' to resume BCM later.");
}

/* ==================================================
   UTILITY FUNCTIONS
================================================== */
byte CalcCRC8(byte data[], int len)
{
  byte crc = 0xFF;
  int i, bit;
  for (i = 0; i < len; i++)
  {
    crc = crc ^ data[i];
    for (bit = 0; bit < 8; bit++)
    {
      if ((crc & 0x80) == 0x80) crc = (crc << 1) ^ 0x1D;
      else crc = crc << 1;
    }
  }
  return crc ^ 0xFF;
}