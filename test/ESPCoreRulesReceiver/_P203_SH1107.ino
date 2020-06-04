//#######################################################################################################
//#################################### Plugin 002: M5 Stick #############################################
//#######################################################################################################

/*
 * Commands:
 * oled row,col,size,message
 * oledcmd,clear
 * oledcmd,brigthness <0-255>
*/

#ifdef USES_P203
#define P203_BUILD            6
#define PLUGIN_203
#define PLUGIN_ID_203         203

#include <SPI.h>
#include <U8g2lib.h>
U8G2_SH1107_64X128_F_4W_HW_SPI P203_u8g2(U8G2_R1,14,27,33);

boolean Plugin_203(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P203 - M5 Stick<TD>");
        printWebTools += P203_BUILD;
        break;
      }
          
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("OledInit")))
        {
          success = true;
          int br = parseString(params,1).toInt();
          P203_u8g2.begin();
          P203_u8g2.clearBuffer();
          if(br)
            P203_u8g2.setContrast(br);
          P203_u8g2.setFont(u8g2_font_logisoso42_tf); 
        }
        if (cmd.equalsIgnoreCase(F("Oled")))
        {
          success = true;
          int row = parseString(params,1).toInt();
          int col = parseString(params,2).toInt();
          int size = parseString(params,3).toInt();
          int msgpos = getParamStartPos(params, 4);
          String msg = "";
          if (msgpos != -1){
            msg = params.substring(msgpos);
          }
          if(size == 20)
            P203_u8g2.setFont(u8g2_font_logisoso20_tf);
          if(size == 42)
            P203_u8g2.setFont(u8g2_font_logisoso42_tf); 
          P203_u8g2.drawStr(col,row,msg.c_str());
          P203_u8g2.sendBuffer();
        }
        if (cmd.equalsIgnoreCase(F("OledCMD")))
        {
          success = true;
          String subcmd = parseString(params,1);
          if (subcmd.equalsIgnoreCase(F("Clear"))){
            P203_u8g2.clearBuffer();
            P203_u8g2.sendBuffer();
          }
          if (subcmd.equalsIgnoreCase(F("Brightness"))){
            int br = parseString(params,2).toInt();
            P203_u8g2.setContrast(br);
          }
        }
        break;
      }
  }
  return success;
}
#endif

