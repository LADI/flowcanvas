#include "mdaSpecMeterGUI.h"
#include "mdaSpecMeter.h"
#include <X11/Xlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <math.h>

CResTable pngResources = { { 128, "mdaSpecMeter.png" } };

mdaSpecMeterGUI::mdaSpecMeterGUI(AudioEffect * effect)
	: AEffGUIEditor(effect)
	, background(NULL)
{
	background = new CBitmap(128);
	rect.right = (LvzInt16) background->getWidth();
	rect.bottom = (LvzInt16) background->getHeight();
}


mdaSpecMeterGUI::~mdaSpecMeterGUI()
{
	delete background;
}


long
mdaSpecMeterGUI::open(void *ptr)
{
	AEffGUIEditor::open(ptr);

	CPoint offs(0, 0);
	CRect size(0, 0, background->getWidth(), background->getHeight());
	frame = new CFrame(size, ptr, this);

	size.offset(0, 0);
	draw = new CDraw(size, 0.0f, background);
	frame->addView(draw);

	return true;
}


void
mdaSpecMeterGUI::close()
{
	delete frame;
	frame = 0;
}


void
mdaSpecMeterGUI::idle()
{
	LvzInt32 xnow = ((mdaSpecMeter *)effect)->counter;
	if(xnow != xtimer)
	{
		xtimer = xnow;

		//if(draw) temp = draw->temp;
		//if(label) label->setLabel(xtimer);

		if(draw) //copy data from effect (this can't be the best way!)
		{
			draw->Lpeak = ((mdaSpecMeter *)effect)->Lpeak;
			draw->Lmin  = ((mdaSpecMeter *)effect)->Lmin;
			draw->Lrms  = ((mdaSpecMeter *)effect)->Lrms;
			draw->Rpeak = ((mdaSpecMeter *)effect)->Rpeak;
			draw->Rmin  = ((mdaSpecMeter *)effect)->Rmin;
			draw->Rrms  = ((mdaSpecMeter *)effect)->Rrms;
			draw->Corr  = ((mdaSpecMeter *)effect)->Corr;
			for(LvzInt32 i=0; i<13; i++)
			{
				draw->band[0][i] = ((mdaSpecMeter *)effect)->band[0][i];
				draw->band[1][i] = ((mdaSpecMeter *)effect)->band[1][i];
			}
			draw->setDirty(true); //trigger redraw
		}
	}

	AEffGUIEditor::idle();
}


CDraw::CDraw(CRect & size, float value, CBitmap * background) : CControl(size)
{
	bitmap = background;

	Lpeak = Lmin = Lrms = Rpeak = Rmin = Rrms = Corr = 0.0f;
	for (LvzInt32 i = 0; i < 16; i++)
		band[0][i] = band[1][i] = 0.0f;

	setValue(value);
}


CDraw::~CDraw()
{
}


void
CDraw::draw(CDrawContext *pContext)
{
	LvzInt32 r, p;
	CRect block;
	CRect rect(0, 0, bitmap->getWidth(), bitmap->getHeight());

	bitmap->draw(pContext, rect);
	/*
	   pContext->setFillColor(kGreenCColor);

	   p = x2pix(Lmin);
	   block(p - 3, 10, p - 1, 18);
	   pContext->fillRect(block);

	   p = x2pix(Rmin);
	   block(p - 3, 18, p - 1, 26);
	   pContext->fillRect(block);
	   */
	pContext->setFillColor(kBlackCColor);

	r = x22pix(Lrms);  if(r > 454) r = 454;
	p = x2pix(Lpeak);  if(p > 454) p = 454;
	block(r - 1, 10, p - 1, 18);
	pContext->fillRect(block);
	block(p - 1, 10, 478, 18);
	if(p < 454) pContext->fillRect(block);

	r = x22pix(Rrms);  if(r > 454) r = 454;
	p = x2pix(Rpeak);  if(p > 454) p = 454;
	block(r - 1, 18, p - 1, 26);
	pContext->fillRect(block);
	block(p - 1, 18, 478, 26);
	if(p < 454) pContext->fillRect(block);

	//block(x2pix(Rpeak), 18, 478, 26);
	//buf->fillRect(block);

	block(235, 42, 244, 134 - (LvzInt32)(90 * Corr));
	pContext->fillRect(block);

	LvzInt32 i, x1=2, x2=256; //octave bands
	float dB;
	for(i=0; i<13; i++)
	{
		dB = band[0][i];
		block(x1, 42, x1+18, 49 - (LvzInt32)(20.0 * log(dB)));
		pContext->fillRect(block);
		x1 += 17;

		dB = band[1][i];
		block(x2, 42, x2+18, 49 - (LvzInt32)(20.0 * log(dB)));
		pContext->fillRect(block);
		x2 += 17;
	}
}


LvzInt32
CDraw::x2pix(float x)
{
	float dB = x;
	LvzInt32 p = 478;

	if(x > 0.00000005f) dB = 8.6858896f * (float)log(x); else dB = -146.0f;
	if(dB < -20.0) 
		p = 293 + (LvzInt32)(2.0f * dB);
	else
		p = 453 + (LvzInt32)(10.0f * dB);

	return p;
}


LvzInt32
CDraw::x22pix(float x) //power version for squared summed
{
	float dB = x;
	LvzInt32 p = 478;

	if(x > 0.00000005f) dB = 4.3429448f * (float)log(x); else dB = -146.0f;
	if(dB < -20.0)
		p = 293 + (LvzInt32)(2.0f * dB);
	else
		if(dB < 0.0f) p = 453 + (LvzInt32)(10.0f * dB);

	return p;
}

