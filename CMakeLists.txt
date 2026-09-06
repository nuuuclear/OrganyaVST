cmake_minimum_required(VERSION 3.22)
project(OrganyaVST VERSION 0.1.0)

file(GLOB ORGANYA_DRUM_WAVS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/organya/Wave/*.wav")

add_subdirectory(external/JUCE)

juce_add_plugin(OrganyaVST
	COMPANY_NAME "nuuu"
	BUNDLE_ID "com.nuuu.organyavst"
	IS_SYNTH TRUE
	NEEDS_MIDI_INPUT TRUE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS FALSE
	COPY_PLUGIN_AFTER_BUILD FALSE
	PLUGIN_MANUFACTURER_CODE nuuu
	PLUGIN_CODE Orgn
	FORMATS VST3 Standalone
	PRODUCT_NAME "Organya VST"
)

juce_add_binary_data(ResourceData
	NAMESPACE ResourceData
	SOURCES
		resources/organya/WAVE100.wdb
		resources/font.png
		resources/organyavst_back.png
		${ORGANYA_DRUM_WAVS}
)

juce_generate_juce_header(OrganyaVST)

target_sources(OrganyaVST
	PRIVATE
		src/PluginProcessor.cpp
		src/PluginProcessor.h
		src/OrganyaEngine.cpp
		src/OrganyaEngine.h
		src/PluginView.cpp
		src/PluginView.h
)

target_compile_features(OrganyaVST PRIVATE cxx_std_17)

target_compile_definitions(OrganyaVST
	PRIVATE
		JUCE_WEB_BROWSER=0
		JUCE_USE_CURL=0
		JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OrganyaVST
	PRIVATE
		juce::juce_audio_utils
		juce::juce_dsp
		ResourceData
	PUBLIC
		juce::juce_recommended_config_flags
		juce::juce_recommended_lto_flags
		juce::juce_recommended_warning_flags
)
