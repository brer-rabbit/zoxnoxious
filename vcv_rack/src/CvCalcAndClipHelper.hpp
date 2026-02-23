#pragma once

namespace zox {

// helper method to avoid a ton of:
//
//  v = params[CUTOFF_KNOB_PARAM].getValue() + inputs[CUTOFF_INPUT].getVoltage() / 10.f;
//  controlMsg->frame[outputDeviceId].samples[cvChannelOffset + CUTOFF] = clamp(v, 0.f, 1.f);
//  if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + CUTOFF] != v) {
//      cutoffClipTimer = clipTime;
//  }
//
// this pattern is really common.  This helper avoid the pattern.
// 

using CvTransform = float (*)(float);

struct CvRoute {
    int knobParam;
    int inputId;
    int channel;
    float* timer;
    CvTransform transform; // may be nullptr for identity transform
};


inline void processCvRoutes(
    const CvRoute* routes,
    int count,
    float clipTime,
    int cvChannelOffset,
    float* frame,
    Param* params,
    Input* inputs)
{
    for (int i = 0; i < count; ++i) {
        const auto& r = routes[i];

        float v = params[r.knobParam].getValue()
          + inputs[r.inputId].getVoltage() / 10.f;

        if (r.transform) {
            v = r.transform(v);
        }

        bool clipped = (v < 0.f) || (v > 1.f);
        float clamped = clamp(v, 0.f, 1.f);
        frame[cvChannelOffset + r.channel] = clamped;

        if (clipped) {
            *r.timer = clipTime;
        }
    }
}


inline float dualLinearSwitch0_8(float v) {
    return v < 0.8f ? v * 0.6f : 2.6f * v - 1.6f;
}


} // namespace zox
