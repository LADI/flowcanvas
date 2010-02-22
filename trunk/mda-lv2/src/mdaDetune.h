//See associated .cpp file for copyright and other info


#define NPARAMS  4  ///number of parameters
#define NPROGS   4  ///number of programs
#define BUFMAX   4096

#ifndef __mdaDetune_H
#define __mdaDetune_H

#include "audioeffectx.h"

class mdaDetuneProgram
{
public:
  mdaDetuneProgram();
private:
  friend class mdaDetune;
  float param[NPARAMS];
  char name[32];
};


class mdaDetune : public AudioEffectX
{
public:
  mdaDetune(audioMasterCallback audioMaster);
  ~mdaDetune();

  virtual void  process(float **inputs, float **outputs, LvzInt32 sampleFrames);
  virtual void  processReplacing(float **inputs, float **outputs, LvzInt32 sampleFrames);
  virtual void  setProgram(LvzInt32 program);
  virtual void  setProgramName(char *name);
  virtual void  getProgramName(char *name);
  virtual bool getProgramNameIndexed (LvzInt32 category, LvzInt32 index, char* name);
  virtual void  setParameter(LvzInt32 index, float value);
  virtual float getParameter(LvzInt32 index);
  virtual void  getParameterLabel(LvzInt32 index, char *label);
  virtual void  getParameterDisplay(LvzInt32 index, char *text);
  virtual void  getParameterName(LvzInt32 index, char *text);
  virtual void  suspend();
  virtual void  resume();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual LvzInt32 getVendorVersion() { return 1000; }

protected:
  mdaDetuneProgram *programs;

  ///global internal variables
  float *buf, *win;       //buffer, window
  LvzInt32  buflen;           //buffer length
  float bufres;           //buffer resolution display
  float semi;             //detune display
  LvzInt32  pos0;             //buffer input
  float pos1, dpos1;      //buffer output, rate
  float pos2, dpos2;      //downwards shift
  float wet, dry;         //ouput levels
};

#endif
