#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/wrappers/GameObject/BallWrapper.h"

class FreestylePackPlugin : public BakkesModPlugin {
private:
    float lastSpeed = 0.0f;
    float maxVanillaSpeed = 15000.0f; // 540 kph limit
    struct VelocityState { float x, y, z, speed; } lastVelocity = {0.0f, 0.0f, 0.0f, 0.0f};

public:
    void onLoad() override {
        cvarManager->registerCvar("fs_superpinch", "1", "Enable Super Pinches", true, true, 0, true, 1);
        cvarManager->registerCvar("fs_multiplier", "3.5", "Pinch Speed", true, true, 1.0f, true, 10.0f);
        cvarManager->registerCvar("fs_uncapper", "1", "Uncap 540kph Speed Limit", true, true, 0, true, 1);

        gameWrapper->HookEvent("Function TAGame.Car_TA.SetVehicleInput", 
            [this](std::string eventName) { OnPhysicsTick(); });
    }

    void onUnload() override {
        gameWrapper->UnhookEvent("Function TAGame.Car_TA.SetVehicleInput");
    }

    void OnPhysicsTick() {
        ServerWrapper server = gameWrapper->GetCurrentGameState();
        if (!server || server.is_null()) return;

        auto balls = server.GetBalls();
        if (balls.Count() == 0) return;
        BallWrapper ball = balls.Get(0);
        if (!ball || ball.is_null()) return;

        Vector vel = ball.GetVelocity();
        float currentSpeed = std::sqrt(vel.X*vel.X + vel.Y*vel.Y + vel.Z*vel.Z);

        // --- 1. SUPER PINCH LOGIC ---
        if (cvarManager->getCvar("fs_superpinch").getBoolValue()) {
            float acceleration = currentSpeed - lastSpeed;
            if (acceleration > 400.0f && currentSpeed > 1000.0f) {
                float mult = cvarManager->getCvar("fs_multiplier").getFloatValue();
                vel.X *= mult;
                vel.Y *= mult;
                vel.Z *= mult;
                ball.SetVelocity(vel);
                currentSpeed = currentSpeed * mult;
            }
        }
        lastSpeed = currentSpeed;

        // --- 2. BALL SPEED UNCAPPER LOGIC ---
        if (cvarManager->getCvar("fs_uncapper").getBoolValue() && currentSpeed >= (maxVanillaSpeed - 10.0f)) {
            if (lastVelocity.speed > currentSpeed && currentSpeed > 0.1f) {
                float scale = lastVelocity.speed / currentSpeed;
                vel.X *= scale; 
                vel.Y *= scale; 
                vel.Z *= scale;
                ball.SetVelocity(vel);
                currentSpeed = lastVelocity.speed;
            }
        }

        lastVelocity = { vel.X, vel.Y, vel.Z, currentSpeed };
    }
};

BAKKESMOD_PLUGIN(FreestylePackPlugin, "Freestyle Tools Pack", "1.0", PLUGIN_FREEPLAY)
