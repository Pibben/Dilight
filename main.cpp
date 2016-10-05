/*
 * main.cpp
 *
 *  Created on: Oct 5, 2016
 *      Author: per
 */

#include <Arduino.h>

#define ARDUINO 1

#include <FastLED.h>

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
        return time < 30;
    }

    virtual ~ConstantColor() {}
};

class Rainbow : public Scene {
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
        leds.fill_rainbow(millis()/10, 2);
        FastLED.show(40);
        return time < 30;
    }
};

class Disco : public Scene {
    virtual bool run(uint16_t time, CRGBArray<NUM_LEDS>& leds) {
        static const CRGB DiscoColors[] = {
                CRGB::Red,
                CRGB::Blue,
                CRGB::Green,
                CRGB::Purple,
                CRGB::Yellow,
                CRGB::Turquoise,
                CRGB::Pink
        };

        leds.fill_rainbow(millis()/10, 2);
        FastLED.show(40);
        return time < 30;
    }
};

void warmWhite() {
}

bool getRemotePulse() {
    return false;
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

    warmWhite();

    Scheduler scheduler;

    ConstantColor blue(CRGB::Blue);
    ConstantColor red(CRGB::Red);
    ConstantColor green(CRGB::Green);
    ConstantColor purple(CRGB::Purple);
    Rainbow rb;

    scheduler.add(&blue);
    scheduler.add(&green);
    scheduler.add(&red);
    scheduler.add(&purple);
    scheduler.add(&rb);

    for(;;) {
#if 0
        if(getRemotePulse()) {
            if(discoMode) {
                scheduler.stop();
                warmWhite();
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

        delay(100);
    }

    return 0;
}


