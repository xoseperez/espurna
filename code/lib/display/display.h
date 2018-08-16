#ifdef INCLUDE_PLUGIN1
#ifndef _PLUGIN_H
#define _PLUGIN_H


//*---------------------------------------------------------------------------
//* Espurna overrides
//* -------------------------------------------------------------------------
//*  Here put espurna plugin specific overrides
//#define RELAY_PROVIDER          RELAY_PROVIDER_HW655
//#define SERIAL_BAUDRATE         9600

//---------------------------------------------------------------------------
//* Plugin defines
//----------------------------------------------------------------------------
//* plese refer to plugin1.ino for more help and inline documentaion
//* Plugin enabled flag (enabling or disabling execution)
#define PLUGIN1_ENABLE             0
//* sample plugin MTQQ topic
#define MQTT_TOPIC_PLUGIN1          "DISPLAY"
//* Sample plugin reporting interval (0 no reporting)
#define PLUGIN_REPORT_EVERY        5
//* Sample plugin parameter values
#define PLUGIN_PARAMETER_1         0

//------------------------------------------------------------
//* Plugin public interface
//------------------------------------------------------------
//* declare the plugin setup function (used by custom.h)
void plugin1Setup();
//* get plugin enabled state
bool plugin1Enabled();

#endif
#endif
