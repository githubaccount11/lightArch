// Tools.h

#ifndef _TOOLS_h
#define _TOOLS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class ToolsClass
{
 protected:


 public:
	void init();
};

extern ToolsClass Tools;

#endif

