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

#define NUM_LEDS 300

static void blankFastLED(CRGBArray<NUM_LEDS>& leds) {
    fill_solid(leds, 300, CRGB::Black);
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
        FastLED.show(40);
        return time < 400;
    }

    virtual ~ConstantColor() {}
};

class Rainbow : public Scene {
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
        leds.fill_rainbow(millis()/10, 2);
        FastLED.show(40);
        return time < 400;
    }
};

class Disco : public Scene {
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
    	if((time % 50) == 0) {
			static const CRGB DiscoColors[] = {
					CRGB::Red,
					CRGB::Blue,
					CRGB::Green,
					CRGB::Purple,
					CRGB::Yellow,
					CRGB::Turquoise,
					CRGB::Pink
			};
			const int sets = 12;
			for(int i = 0; i < sets; ++i) {
				const int num = NUM_LEDS / sets;
				leds(i*num,(i+1)*num-1).fill_solid(DiscoColors[random8(7)]);
			}
    	}

		FastLED.show(40);

        return time < 400;
    }
};

void warmWhite(CRGBArray<NUM_LEDS>& leds) {
	leds.fill_solid(CRGB::White);
	FastLED.show();
}

bool getRemotePulse() {
    return receive433MHz(0x00899381, 3);
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
    void start() {}

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

    blankFastLED(leds);

    warmWhite(leds);

    Scheduler scheduler;

    ConstantColor blue(CRGB::Blue);
    ConstantColor red(CRGB::Red);
    ConstantColor green(CRGB::Green);
    ConstantColor purple(CRGB::Purple);
    Rainbow rb;
    Disco d;

    scheduler.add(&blue);
    scheduler.add(&green);
    scheduler.add(&red);
    scheduler.add(&purple);
    scheduler.add(&rb);
    scheduler.add(&d);

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


