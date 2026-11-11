#include "plugin.hpp"
#include <vector> 

struct ShiftRegister : Module {


	dsp::SchmittTrigger Trigger;

	float values[64];

	static const int SIZE = 64;

	std::vector<std::pair<int, int>> regions;

	int regionLengths[4] = { 16,16,16,16 };

	std::atomic<int> positionRegions[4];

	float voice1Transposition = 0.f;
	float voice2Transposition = 0.f;
	float voice3Transposition = 0.f;
	float voice4Transposition = 0.f;

	float transpose1cv = 0.f;
	float transpose2cv = 0.f;
	float transpose3cv = 0.f;
	float transpose4cv = 0.f;

	enum ParamId {
		SAMPLECHANCE_PARAM,

		REGION_1_LENGTH,
		REGION_2_LENGTH,
		REGION_3_LENGTH,
		REGION_4_LENGTH,

		POSITION_1,
		POSITION_2,
		POSITION_3,
		POSITION_4,

		TRANSPOSITION_1,
		TRANSPOSITION_2,
		TRANSPOSITION_3,
		TRANSPOSITION_4,

		PARAMS_LEN
	};

	enum InputId {
		CLOCK_INPUT,

		SAMPLE_CV_INPUT,

		REGION_1_LENGTH_CV,
		REGION_2_LENGTH_CV,
		REGION_3_LENGTH_CV,
		REGION_4_LENGTH_CV,

		POSITION_1_CV,
		POSITION_2_CV,
		POSITION_3_CV,
		POSITION_4_CV,

		TRANSPOSITION_1_CV,
		TRANSPOSITION_2_CV,
		TRANSPOSITION_3_CV,
		TRANSPOSITION_4_CV,

		INPUTS_LEN
	};

	enum OutputId {
		OUT_1,
		OUT_2,
		OUT_3,
		OUT_4, 
		OUTPUTS_LEN
	};

	enum LightId {
		SEQ_LIGHT,
		SEQ_LIGHT_LAST = SEQ_LIGHT + 3,
		LIGHTS_LEN
	};

	ShiftRegister() {

		for (int i = 0; i < 4; i++) {
			positionRegions[i].store(i + 1);
		}

		for (int i = 0; i < SIZE; i++) {
			values[i] = 0.f;
		}

		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	    configParam(SAMPLECHANCE_PARAM, 0.f, 100.f, 0.f, "Sample Chance");

		configParam(REGION_1_LENGTH, 1.f, 64.f, 16.f, "Region 1 Size");
		paramQuantities[REGION_1_LENGTH]->snapEnabled = true;
		configParam(REGION_2_LENGTH, 1.f, 64.f, 16.f, "Region 2 Size");
		paramQuantities[REGION_2_LENGTH]->snapEnabled = true;
		configParam(REGION_3_LENGTH, 1.f, 64.f, 16.f, "Region 3 Size");
		paramQuantities[REGION_3_LENGTH]->snapEnabled = true;
		configParam(REGION_4_LENGTH, 1.f, 64.f, 16.f, "Region 4 Size");
		paramQuantities[REGION_4_LENGTH]->snapEnabled = true;

		configParam(POSITION_1, 0.f, 63.f, 0.f, "Sequence Position 1");
		paramQuantities[POSITION_1]->snapEnabled = true;
		configParam(POSITION_2, 0.f, 63.f, 16.f, "Sequence Position 2");
		paramQuantities[POSITION_2]->snapEnabled = true;
		configParam(POSITION_3, 0.f, 63.f, 32.f, "Sequence Position 3");
		paramQuantities[POSITION_3]->snapEnabled = true;
		configParam(POSITION_4, 0.f, 63.f, 48.f, "Sequence Position 4");
		paramQuantities[POSITION_4]->snapEnabled = true;

		configParam(TRANSPOSITION_1, -24.f, 24.f, 0.f, "Voice 1 Transposition");
		paramQuantities[TRANSPOSITION_1]->snapEnabled = true;
		configParam(TRANSPOSITION_2, -24.f, 24.f, 0.f, "Voice 2 Transposition");
		paramQuantities[TRANSPOSITION_2]->snapEnabled = true;
		configParam(TRANSPOSITION_3, -24.f, 24.f, 0.f, "Voice 3 Transposition");
		paramQuantities[TRANSPOSITION_3]->snapEnabled = true;
		configParam(TRANSPOSITION_4, -24.f, 24.f, 0.f, "Voice 4 Transposition");
		paramQuantities[TRANSPOSITION_4]->snapEnabled = true;

		configInput(CLOCK_INPUT, "Clock");
		
		configInput(SAMPLE_CV_INPUT, "Sample and Hold Signal");

		for (int i = 0; i < 4; i++) {
			configInput(REGION_1_LENGTH_CV + i, "Region CV");
			configInput(POSITION_1_CV + i, "Position CV");
			configInput(TRANSPOSITION_1_CV + i, "Transposition CV");
		}
		
		configOutput(OUT_1, "sequence position 1");
		configOutput(OUT_2, "sequence position 2");
		configOutput(OUT_3, "sequence position 3");
		configOutput(OUT_4, "sequence position 4");

	}

	int getRegionForPosition(int pos) {
		for (int i = 0; i < (int)regions.size(); i++) {
			if (pos >= regions[i].first &&
				pos <= regions[i].second) {
				return i + 1;
			}
		}

		return 0;
	}

	void buildRegions() {
		regions.clear();

		int start = 0;

		for (int i = 0; i < 4; i++) {

			int len = regionLengths[i];

			int remaining = SIZE - start;

			if (remaining <= 0)
				break;

			len = std::min(len, remaining);

			int end = start + len - 1;

			regions.push_back({ start, end });

			start = end + 1;
		}
	}

	void shiftRegion(int start, int end) {

		float last = values[end];

		for (int i = end; i > start; i--) {
			values[i] = values[i - 1];
		}

		values[start] = last;
	}

	void process(const ProcessArgs& args) override {

		voice1Transposition = params[TRANSPOSITION_1].getValue() / 12;
		voice2Transposition = params[TRANSPOSITION_2].getValue() / 12;
		voice3Transposition = params[TRANSPOSITION_3].getValue() / 12;
		voice4Transposition = params[TRANSPOSITION_4].getValue() / 12;

		int pos1 = params[POSITION_1].getValue();
		int pos2 = params[POSITION_2].getValue();
		int pos3 = params[POSITION_3].getValue();
		int pos4 = params[POSITION_4].getValue();

		if (inputs[POSITION_1_CV].isConnected()) {
			float pos1cv = std::fabs(inputs[POSITION_1_CV].getVoltage());
			pos1 = clamp((int)std::round(rescale(pos1cv, 0.f, 10.f, 1.f, 63.f)), 1, 63);
		}

		if (inputs[POSITION_2_CV].isConnected()) {
			float pos2cv = std::fabs(inputs[POSITION_2_CV].getVoltage());
			pos2 = clamp((int)std::round(rescale(pos2cv, 0.f, 10.f, 1.f, 63.f)), 1, 63);
		}

		if (inputs[POSITION_3_CV].isConnected()) {
			float pos3cv = std::fabs(inputs[POSITION_3_CV].getVoltage());
			pos3 = clamp((int)std::round(rescale(pos3cv, 0.f, 10.f, 1.f, 63.f)), 1, 63);
		}

		if (inputs[POSITION_4_CV].isConnected()) {
			float pos4cv = std::fabs(inputs[POSITION_4_CV].getVoltage());
			pos4 = clamp((int)std::round(rescale(pos4cv, 0.f, 10.f, 1.f, 63.f)), 1, 63);
		}

		if (Trigger.process(inputs[CLOCK_INPUT].getVoltage(),0.1f,2.0f)) {

			float region1cv = inputs[REGION_1_LENGTH_CV].isConnected()
				? inputs[REGION_1_LENGTH_CV].getVoltage()
				: 0.f;
			float region1knob = params[REGION_1_LENGTH].getValue();
			int region1len = clamp((int)std::round(region1knob + region1cv), 1, 64);
			regionLengths[0] = region1len;

			float region2cv = inputs[REGION_2_LENGTH_CV].isConnected()
				? inputs[REGION_2_LENGTH_CV].getVoltage()
				: 0.f;
			float region2knob = params[REGION_2_LENGTH].getValue();
			int region2len = clamp((int)std::round(region2knob + region2cv), 1, 64);
			regionLengths[1] = region2len;

			float region3cv = inputs[REGION_3_LENGTH_CV].isConnected()
				? inputs[REGION_3_LENGTH_CV].getVoltage()
				: 0.f;
			float region3knob = params[REGION_3_LENGTH].getValue();
			int region3len = clamp((int)std::round(region3knob + region3cv), 1, 64);
			regionLengths[2] = region3len;

			float region4cv = inputs[REGION_4_LENGTH_CV].isConnected()
				? inputs[REGION_4_LENGTH_CV].getVoltage()
				: 0.f;
			float region4knob = params[REGION_4_LENGTH].getValue();
			int region4len = clamp((int)std::round(region4knob + region4cv), 1, 64);
			regionLengths[3] = region4len;

			buildRegions();

			float probability = params[SAMPLECHANCE_PARAM].getValue() / 100.f;

			if (random::uniform() < probability) {

				// SAMPLE MODE

				for (int i = SIZE - 1; i > 0; i--) {
					values[i] = values[i - 1];
				}

				values[0] = inputs[SAMPLE_CV_INPUT].getVoltage();

			}
			else {

				// SHIFT MODE

				for (auto region : regions) {
					shiftRegion(region.first, region.second);
				}
			}
			transpose1cv = inputs[TRANSPOSITION_1_CV].isConnected()
				? inputs[TRANSPOSITION_1_CV].getVoltage() * 12.f
				: 0.f;
			transpose2cv = inputs[TRANSPOSITION_2_CV].isConnected()
				? inputs[TRANSPOSITION_2_CV].getVoltage() * 12.f
				: 0.f;
			transpose3cv = inputs[TRANSPOSITION_3_CV].isConnected()
				? inputs[TRANSPOSITION_3_CV].getVoltage() * 12.f
				: 0.f;
			transpose4cv = inputs[TRANSPOSITION_4_CV].isConnected()
				? inputs[TRANSPOSITION_4_CV].getVoltage() * 12.f
				: 0.f;
		}

		voice1Transposition = clamp(std::round((voice1Transposition * 12) + transpose1cv), -24.f, 24.f) / 12.f;
		voice2Transposition = clamp(std::round((voice2Transposition * 12) + transpose2cv), -24.f, 24.f) / 12.f;
		voice3Transposition = clamp(std::round((voice3Transposition * 12) + transpose3cv), -24.f, 24.f) / 12.f;
		voice4Transposition = clamp(std::round((voice4Transposition * 12) + transpose4cv), -24.f, 24.f) / 12.f;

		float reg1 = values[pos1] + voice1Transposition;
		float reg2 = values[pos2] + voice2Transposition;
		float reg3 = values[pos3] + voice3Transposition;
		float reg4 = values[pos4] + voice4Transposition;

		positionRegions[0].store(getRegionForPosition(pos1));
		positionRegions[1].store(getRegionForPosition(pos2));
		positionRegions[2].store(getRegionForPosition(pos3));
		positionRegions[3].store(getRegionForPosition(pos4));

		outputs[OUT_1].setVoltage(reg1);
		outputs[OUT_2].setVoltage(reg2);
		outputs[OUT_3].setVoltage(reg3);
		outputs[OUT_4].setVoltage(reg4);

	}
};

struct RegionReadout : TransparentWidget {
	ShiftRegister* module;
	int voice;

	RegionReadout(ShiftRegister* module, int voice) {
		this->module = module;
		this->voice = voice;

		box.size = Vec(12, 12);
	}

	void draw(const DrawArgs& args) override {
		if (!module)
			return;

		int region = module->positionRegions[voice].load();

		char text[4];

		if (region == 0)
			snprintf(text, sizeof(text), "-");
		else
			snprintf(text, sizeof(text), "%d", region);

		nvgFontSize(args.vg, 12);
		nvgFillColor(args.vg, nvgRGB(255, 255, 255));

		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		nvgText(
			args.vg,
			box.size.x / 2,
			box.size.y / 2,
			text,
			NULL
		);
	}
};


struct ShiftRegisterWidget : ModuleWidget {
	ShiftRegisterWidget(ShiftRegister* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Sequencer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(
			mm2px(Vec(18, 15)),
			module,
			ShiftRegister::CLOCK_INPUT
		));

		addInput(createInputCentered<PJ301MPort>(
			mm2px(Vec(18, 25)),
			module,
			ShiftRegister::SAMPLE_CV_INPUT
		));

		addParam(createParamCentered<RoundBlackKnob>(
			mm2px(Vec(63, 25)),
			module,
			ShiftRegister::SAMPLECHANCE_PARAM
		));

		for (int i = 0; i < 4; i++) {
			float x = 18.f + i * 15.f;
			addInput(createInputCentered<PJ301MPort>(
				mm2px(Vec(x, 55)),
				module,
				ShiftRegister::POSITION_1_CV + i
			));
		}

		for (int i = 0; i < 4; i++) {
			float x = 18.f + i * 15.f;
			addInput(createInputCentered<PJ301MPort>(
				mm2px(Vec(x, 85)),
				module,
				ShiftRegister::TRANSPOSITION_1_CV + i
			));
		}

		for (int i = 0; i < 4; i++) {
			float x = 18.f + i * 15.f;
			addInput(createInputCentered<PJ301MPort>(
				mm2px(Vec(x, 115)),
				module,
				ShiftRegister::REGION_1_LENGTH_CV + i
			));
		}

		for (int i = 0; i < 4; i++) {
			float x = 18.f + i * 15.f;

			addParam(createParamCentered<RoundBlackKnob>(
				mm2px(Vec(x, 45)),
				module,
				ShiftRegister::POSITION_1 + i
			));

			auto display = new RegionReadout(module, i);
			display->box.pos = mm2px(Vec(x - 1.6f, 36.f));
			addChild(display);
		}

		for (int i = 0; i < 4; i++) {
			float x = 18.f + i * 15.f;
			addParam(createParamCentered<RoundBlackKnob>(
				mm2px(Vec(x, 75)),
				module,
				ShiftRegister::TRANSPOSITION_1 + i
			));
		}

		for (int i = 0; i < 4; i++) {
			float x = 18.f + i * 15.f;
			addParam(createParamCentered<RoundBlackKnob>(
				mm2px(Vec(x, 105)),
				module,
				ShiftRegister::REGION_1_LENGTH + i
			));
		}


		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18, 125)), module, ShiftRegister::OUT_1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33, 125)), module, ShiftRegister::OUT_2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48, 125)), module, ShiftRegister::OUT_3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63, 125)), module, ShiftRegister::OUT_4));


	}
};


Model* modelShiftRegisterSequencer = createModel<ShiftRegister, ShiftRegisterWidget>("ShiftRegisterSequencer");