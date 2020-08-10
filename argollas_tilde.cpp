#include "c74_msp.h"

#include "rings/dsp/part.h"
#include "rings/dsp/strummer.h"
#include "rings/dsp/string_synth_part.h"

#include <iostream>

using namespace c74::max;

static t_class* this_class = nullptr;

inline double constrain(double v, double vMin, double vMax) {
	return std::max<double>(vMin, std::min<double>(vMax, v));
}

inline int min(int a, int b) {
	return (a < b) ? a : b;
}

/** Returns the maximum of `a` and `b` */
inline int max(int a, int b) {
	return (a > b) ? a : b;
}

inline int clamp(int x, int a, int b) {
	return min(max(x, a), b);
}

inline short TO_SHORTFRAME(float v) { return (short(v * 16384.0f)); }
inline float FROM_SHORTFRAME(short v) { return (float(v) / 16384.0f); }

struct t_argollas {
	t_pxobject x_obj;
	//enum ParamIds {
	double param_polyphony;
	double param_resonator;

	double param_frequency;
	double param_structure;
	double param_brightness;
	double param_damping;
	double param_position;

	double param_brightness_mod;
	double param_frequency_mod;
	double param_damping_mod;
	double param_structure_mod;
	double param_position_mod;

	//uint16_t reverb_buffer[32768] = {};
	rings::Part part;
	rings::StringSynthPart string_synth;
	rings::Strummer strummer;
	rings::Patch patch;

	bool strum = false;
	bool lastStrum = false;

	int polyphonyMode = 0;
	rings::ResonatorModel model = rings::RESONATOR_MODEL_MODAL;
	bool easterEgg = false;

	t_argollas();
	void init();
	void step();
	void changeRate();

	long sf = 64;
	long sr = 48000.0;

	float* ibuf;
	float* outbuf;
	float* auxbuf;

};

t_argollas::t_argollas() {
}

void t_argollas::init() {
	memset(&strummer, 0, sizeof(strummer));
	memset(&part, 0, sizeof(part));
	memset(&string_synth, 0, sizeof(string_synth));

	strummer.Init(0.01, sr / 24);
	/*part.Init(reverb_buffer);
	string_synth.Init(reverb_buffer);*/

}

void t_argollas::changeRate() {
	ibuf = new float[sf];
	outbuf = new float[sf];
	auxbuf = new float[sf];
	strummer.Init(0.01, sr / 24);
}

void t_argollas::step() {

		// Polyphony
	int polyphony = 1 << polyphonyMode;
	if (part.polyphony() != polyphony)
	part.set_polyphony(polyphony);

	patch.position = param_position;
	patch.structure = param_structure;
	patch.brightness = param_brightness;
	patch.damping = param_damping;

	rings::PerformanceState performance_state;
	performance_state.tonic = 12.0 + param_frequency;

	performance_state.internal_exciter = false;
	performance_state.internal_strum = false;
	performance_state.internal_note = false;

	performance_state.strum = strum && !lastStrum;
	lastStrum = strum;
	strum = false;
	float structure = param_structure;
	performance_state.chord = clamp((int) roundf(structure * (rings::kNumChords - 1)), 0, rings::kNumChords - 1);

	strummer.Process(ibuf, 24, &performance_state);
	part.Process(performance_state, patch, ibuf, outbuf, auxbuf, sf);

}

void* argollas_new(void) {
	t_argollas* self = (t_argollas*) object_alloc(this_class);

	dsp_setup((t_pxobject*)self, 1);
	outlet_new(self, "signal");
	outlet_new(self, "signal");
	inlet_new(self, NULL);

	//self->x_obj.z_misc = Z_NO_INPLACE;
	self->init();

	return (void *)self;
}


void argollas_perform64(t_argollas* self, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam) {
    double    *in = ins[0];     // first inlet
    //float    *in2 = ins[1];     // first inlet
    double    *out = outs[0];   // first outlet
    double    *out2 = outs[1];   // first outlet

	if (self->sf!=sampleframes){
		self->sf=sampleframes;
		self->changeRate();
	}

	for (int i=0; i<sampleframes; ++i){
		self->ibuf[i] = *in++;
	}

	self->step();

	for (int i = 0; i < sampleframes; i++) {
		*out++ = self->outbuf[i];
		*out2++ = self->auxbuf[i];
	}

}

void argollas_free(t_argollas* self) {
	dsp_free((t_pxobject*)self);
}

void argollas_dsp64(t_argollas* self, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags) {
	object_method_direct(void, (t_object*, t_object*, t_perfroutine64, long, void*),
						 dsp64, gensym("dsp_add64"), (t_object*)self, (t_perfroutine64)argollas_perform64, 0, NULL);

	if (self->sr!=samplerate)
	{
		self->sr=samplerate;
		self->changeRate();
	}
}


void argollas_assist(t_argollas* self, void* unused, t_assist_function io, long index, char* string_dest) {
	if (io == ASSIST_INLET) {
		switch (index) {
			case 0: 
				strncpy(string_dest,"(signal) IN", ASSIST_STRING_MAXSIZE); 
				break;
			case 1: 
				strncpy(string_dest,"(parameters) IN", ASSIST_STRING_MAXSIZE); 
				break;
		}
	}
	else if (io == ASSIST_OUTLET) {
		switch (index) {
			case 0: 
				strncpy(string_dest,"(signal) Out", ASSIST_STRING_MAXSIZE); 
				break;
			case 1: 
				strncpy(string_dest,"(signal) Aux", ASSIST_STRING_MAXSIZE); 
				break;
		}
	}
}

void argollas_easteregg(t_argollas *x, double f)
{
	/*x->easterEgg = f>0.5f;*/
	/*if (x->easterEgg)
		x->string_synth.set_fx((rings::FxType) x->model);
	else*/
		x->part.set_model(x->model);

}


void argollas_polyphony(t_argollas *x, double f)
{
	x->param_polyphony = f;
	x->polyphonyMode = f;

}

void argollas_resonator(t_argollas *x, double f)
{
	x->model = (rings::ResonatorModel) f;
	x->part.set_model(x->model);
}

void argollas_frequency(t_argollas *x, double f)
{
	x->param_frequency = f;
}

void argollas_structure(t_argollas *x, double f)
{
	x->param_structure = f;
}

void argollas_brightness(t_argollas *x, double f)
{

	x->param_brightness = f;

	/*object_post(&x->m_obj.z_ob, "param_brightness");
	std::string s = std::to_string(x->sf);
	object_post(&x->m_obj.z_ob, s.c_str());*/

}

void argollas_damping(t_argollas *x, double f)
{
	x->param_damping = f;

	/*object_post(&x->m_obj.z_ob, "param_brightness");
	std::string s = std::to_string(x->param_damping);
	object_post(&x->m_obj.z_ob, s.c_str());*/

}

void argollas_damping_mod(t_argollas *x, double f)
{
	x->param_damping_mod = f;
}

void argollas_position(t_argollas *x, double f)
{
	x->param_position = f;
}

void argollas_brightness_mod(t_argollas *x, double f)
{
	x->param_brightness_mod = f;
}

void argollas_frequency_mod(t_argollas *x, double f)
{
	x->param_frequency_mod = f;
}

void argollas_structure_mod(t_argollas *x, double f)
{
	x->param_structure_mod = f;
}

void argollas_position_mod(t_argollas *x, double f)
{
	x->param_position_mod = f;
}

/*
void argollas_inputbrightness_mod(t_argollas *x, double f)
{
	x->input_brightness_mod = f;
}

void argollas_inputfrequency_mod(t_argollas *x, double f)
{
	x->input_frequency_mod = f;
}

void argollas_inputdamping_mod(t_argollas *x, double f)
{
	x->input_damping_mod = f;
}

void argollas_inputstructure_mod(t_argollas *x, double f)
{
	x->input_structure_mod = f;
}

void argollas_inputposition_mod(t_argollas *x, double f)
{
	x->input_position_mod = f;
}



void argollas_inputpitch(t_argollas *x, double f)
{
	x->input_pitch = f;
}

void argollas_inputin(t_argollas *x, double f)
{
	x->input_in = f;
}*/

void argollas_strum(t_argollas *x, double f)
{
	x->strum = f > 0.5f;
}


void ext_main(void* r) {
	this_class = class_new("argollas~", (method)argollas_new, (method)argollas_free, sizeof(t_argollas), NULL, A_GIMME, 0);

	class_addmethod(this_class,(method) argollas_dsp64, "dsp64",	A_CANT,		0);

	class_addmethod(this_class,(method) argollas_polyphony,"polyphony",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_resonator,"resonator",A_DEFFLOAT,0);

	class_addmethod(this_class,(method) argollas_frequency,"frequency",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_structure,"structure",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_brightness,"brightness",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_damping,"damping",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_position,"position",A_DEFFLOAT,0);

	class_addmethod(this_class,(method) argollas_brightness_mod,"brightness_mod",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_frequency_mod,"frequency_mod",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_damping_mod,"damping_mod",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_structure_mod,"structure_mod",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_position_mod,"position_mod",A_DEFFLOAT,0);
	//enum inputids {

	class_addmethod(this_class,(method) argollas_easteregg,"easteregg",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_strum,"strum",A_DEFFLOAT,0);
	/*class_addmethod(this_class,(method) argollas_inputpitch,"pitch",A_DEFFLOAT,0);
	class_addmethod(this_class,(method) argollas_inputin,"in",A_DEFFLOAT,0);*/
	class_addmethod(this_class,(method) argollas_assist, "assist",	A_CANT,		0);

	class_dspinit(this_class);
	class_register(CLASS_BOX, this_class);
}