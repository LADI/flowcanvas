#ifndef _mdaSpecMeterGUI_h_
#define _mdaSpecMeterGUI_h_

#include "vstgui.h"


class CDraw : public CControl
{
public:
	CDraw(CRect& size, float x, CBitmap* background);
	~CDraw();

	void draw(CDrawContext *pContext);
	LvzInt32 x2pix(float x);
	LvzInt32 x22pix(float x);

	float Lpeak, Lrms, Lmin, Rpeak, Rrms, Rmin, Corr;
	float band[2][16];
	LvzInt32 temp;

protected:
	CBitmap* bitmap;
};


class mdaSpecMeterGUI : public AEffGUIEditor
{
public:
	mdaSpecMeterGUI(AudioEffect* effect);
	~mdaSpecMeterGUI();

	long open(void* ptr);
	void idle();
	void close();

private:
	CDraw*   draw;
	CBitmap* background;
	LvzInt32 xtimer;
};


#endif // _mdaSpecMeterGUI_h_

