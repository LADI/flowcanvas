#ifndef __LVZ_AUDIOEFFECT_HPP
#define __LVZ_AUDIOEFFECT_HPP

class AudioEffect;

class AEffEditor {
public:
	AEffEditor (AudioEffect* eff)
		: effect(eff)
		, URI("NULL")
		, pluginURI("NULL")
	{}

	virtual long open(void* ptr) { return true; }
	virtual void close() {}

	virtual void idle() {}
	virtual void postUpdate() {}

	virtual const char* getURI()       { return URI; }
	virtual void setURI(const char* u) { URI = u; }

	virtual const char* getPluginURI()              { return pluginURI; }
	virtual void        setPluginURI(const char* u) { pluginURI = u; }

protected:
	AudioEffect* effect;
	const char*  URI;
	const char*  pluginURI;
};

#endif // __LVZ_AUDIOEFFECT_HPP
