/*
 * main.cpp
 *
 *  Created on: Oct 5, 2016
 *      Author: per
 */

#include <Arduino.h>

#define ARDUINO 1

#include <FastLED.h>

#include <lib433mhz.h>

#define NUM_LEDS (232)

static const uint8_t globalIntensity = 255;

static void blankFastLED(CRGBArray<NUM_LEDS>& leds) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

class Scene {
public:
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) = 0;
    virtual ~Scene() {}
    Scene* next;
};

class ConstantColor : public Scene {
private:
    CRGB mColor;
public:
    ConstantColor(const CRGB& color) : mColor(color) {}

    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
        leds.fill_solid(mColor);
        FastLED.show(globalIntensity);
        return time < 400;
    }

    virtual ~ConstantColor() {}
};

class Rainbow : public Scene {
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
        leds.fill_rainbow(millis() / 5, 2);
        FastLED.show(globalIntensity);
        return time < 400;
    }
};

static const CRGB DiscoColors[] = {
        CRGB::Red,
        CRGB::Blue,
        CRGB::Green,
        CRGB::Purple,
        CRGB::Yellow,
        CRGB::Turquoise,
        CRGB::Pink,
        CRGB::Orange
};

static const uint8_t numDiscoColors = 8;

class Disco : public Scene {
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
        if((time % 50) == 0) {
            const int sets = 8;
            for(int i = 0; i < sets; ++i) {
                const int num = NUM_LEDS / sets;
                leds(i * num, (i + 1) * num - 1).fill_solid(DiscoColors[random8(numDiscoColors)]);
            }
        }

        FastLED.show(globalIntensity);

        return time < 400;
    }
};

class Fader : public Scene {
private:
    struct State {
        CRGB color;
        uint8_t level;
        int8_t dir;
    };

    State states[2];

    uint8_t count;

    static bool fade(State& state, CPixelView<CRGB> leds) {
        static int scale = 5;

        bool retval = false;

        leds.fill_solid(state.color).fadeLightBy(state.level);

        state.level += state.dir*scale;
        if(state.level == 0 && state.dir == -1) {
            state.dir = 1;
        } else if(state.level == 255 && state.dir == 1) {
            state.color = DiscoColors[random8(numDiscoColors)];
            state.dir = -1;
            retval = true;
        }

        return retval;
    }
public:
    Fader() : count(0) {
        states[0].color = DiscoColors[random8(numDiscoColors)];
        states[0].level = 0;
        states[0].dir = 1;

        states[1].color = DiscoColors[random8(numDiscoColors)];
        states[1].level = 255;
        states[1].dir = -1;
    }

    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {

        if(fade(states[0], leds(0, NUM_LEDS / 2 - 1))) {
            count++;
        }
        fade(states[1], leds(NUM_LEDS / 2, NUM_LEDS - 1));

        FastLED.show(globalIntensity);

        if(count == 4) {
            count = 0;
            return false;
        } else {
            return true;
        }
    }
};

class Flow : public Scene {
private:
    uint16_t front;
    bool inited;
    CRGB color;

    static CRGB fadeTo(const CRGB& from, const CRGB& to, uint8_t level) {
        CRGB retval;

        retval.r = (from.r * (255 - level) + to.r * level) / 256;
        retval.g = (from.g * (255 - level) + to.g * level) / 256;
        retval.b = (from.b * (255 - level) + to.b * level) / 256;

        return retval;
    }

public:
    Flow() : front(0), inited(false), color(CRGB::Black) {
    }
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
        if(!inited) {
            leds.fill_solid(CRGB::Black);
        }

        front = (time * 5) % NUM_LEDS;

        if((time % (NUM_LEDS / 5)) == 0) {
            color = DiscoColors[random8(numDiscoColors)];
        }

        uint16_t i = front;

        while(true) {
            const uint8_t fade = max(0, i - front)*128/NUM_LEDS;
            const uint8_t level = max(0, 255-fade);

            leds[i % NUM_LEDS] = fadeTo(color, CRGB::Black, level);

            i++;

            if(i == front+NUM_LEDS) {
                break;
            }
        }

        FastLED.show(globalIntensity);

        return time < NUM_LEDS;
    }
};

void warmWhite(CRGBArray<NUM_LEDS>& leds) {
    leds.fill_solid(CRGB::White);
    FastLED.show(globalIntensity);
}

bool getRemotePulse() {
    static uint8_t debounce = 0;
    if(peek() && receive433MHz(0x00899381, 3) && debounce == 0) {
        debounce = 5;
        return true;
    } else {
        debounce = debounce == 0 ? 0 : debounce - 1;
        return false;
    }
}

class Scheduler {
private:
    Scene* scenes;
    Scene* currentScene;
    uint16_t time;

    void next() {
        if(currentScene->next) {
            currentScene = currentScene->next;
        } else {
            currentScene = scenes;
        }
    }
public:
    Scheduler() : scenes(nullptr), currentScene(nullptr), time(0) {}

    void run(CRGBArray<NUM_LEDS>& leds) {
        if(!currentScene->run(time, leds)) {
            next();
            time = 0;
        } else {
            time++;
        }
    }

    void stop() {}
    void start() {
        currentScene = scenes;
        time = 0;
    }

    void add(Scene* scene) {
        if(scenes == nullptr) {
            scenes = scene;
            currentScene = scene;
            scene->next = nullptr;
        } else {
            scene->next = scenes;
            scenes = scene;
        }
    }
};

int main() {
    bool discoMode = false;
    CRGBArray<NUM_LEDS> leds;

    init();

    FastLED.addLeds<APA102, 11, 13, BGR>(leds, NUM_LEDS);
    FastLED.setCorrection(TypicalLEDStrip);
    FastLED.setTemperature(Tungsten40W);

    blankFastLED(leds);

    warmWhite(leds);

    Scheduler scheduler;

    ConstantColor blue(CRGB::Blue);
    ConstantColor red(CRGB::Red);
    ConstantColor green(CRGB::Green);
    ConstantColor purple(CRGB::Purple);
    Rainbow rb;
    Disco d;
    Fader f;
    Flow w;

    //scheduler.add(&blue);
    //scheduler.add(&green);
    //scheduler.add(&red);
    //scheduler.add(&purple);
    scheduler.add(&rb);
    scheduler.add(&d);
    scheduler.add(&f);
    scheduler.add(&w);

    for(;;) {
#if 1
        if(getRemotePulse()) {
            if(discoMode) {
                scheduler.stop();
                warmWhite(leds);
            } else {
                scheduler.start();
            }

            discoMode = !discoMode;
        }
#else
        discoMode = true;
#endif
        if(discoMode) {
            scheduler.run(leds);
        }

        delay(10);
    }

    return 0;
}

