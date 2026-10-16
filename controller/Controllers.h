// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <SDL2/SDL.h>
#include <QPointF>
#include <QString>
#include <array>
#include <vector>
#include <cmath>
#include <stdexcept>
#include "CoopIPC.h"
namespace coop {
struct Pad { SDL_JoystickID id; QString name; QString guid; QString serial; };
struct ControlState { uint32_t keys=0xFFF, touch=0; bool toggle=false, exit=false; };
struct MenuState { bool accept=false, up=false, down=false; };
class Controllers {
    std::array<SDL_GameController*,2> handles{{nullptr,nullptr}};
    std::array<SDL_JoystickID,2> assigned{{-1,-1}};
    std::array<QPointF,2> cursors{{{128,96},{128,96}}};
    std::array<bool,2> toggleHeld{{false,false}};
    std::array<Uint32,2> exitSince{{0,0}};
    bool assignedBefore=false;
public:
    // Standard SDL GameController buttons; configurable from launcher settings.
    std::array<int,12> buttons{{SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_X}};
    Controllers() {
        SDL_SetMainReady();
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
            throw std::runtime_error(SDL_GetError());
    }
    ~Controllers() { clear(); SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER); }
    void clear() {
        for (auto& h: handles) { if(h) SDL_GameControllerClose(h); h=nullptr; }
        assigned={{-1,-1}}; toggleHeld={{false,false}}; exitSince={{0,0}};
    }
    std::vector<Pad> list() {
        SDL_PumpEvents(); SDL_GameControllerUpdate();
        std::vector<Pad> result;
        for(int i=0;i<SDL_NumJoysticks();i++) if(SDL_IsGameController(i)) {
            char guid[33]{}; SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i),guid,33);
            auto* h=SDL_GameControllerOpen(i);
            const char* serial=h ? SDL_GameControllerGetSerial(h) : nullptr;
            result.push_back({SDL_JoystickGetDeviceInstanceID(i),QString::fromUtf8(SDL_GameControllerNameForIndex(i)),QString::fromUtf8(guid),serial?QString::fromUtf8(serial):QString()});
            if(h) SDL_GameControllerClose(h);
        }
        return result;
    }
    void assign(SDL_JoystickID a, SDL_JoystickID b) {
        if(a<0 || b<0 || a==b) throw std::runtime_error("Zwei unterschiedliche Controller auswählen.");
        clear(); 
        for(int p=0;p<2;p++) {
            const auto wanted=p?b:a;
            for(int i=0;i<SDL_NumJoysticks();i++) if(SDL_JoystickGetDeviceInstanceID(i)==wanted && SDL_IsGameController(i)) handles[p]=SDL_GameControllerOpen(i);
            if(!handles[p]) { clear(); throw std::runtime_error("Controller nicht mehr verfügbar."); }
            assigned[p]=wanted;
        }
        assignedBefore=true;
    }
    SDL_JoystickID id(int p) const { return assigned.at(p); }
    bool connected(int p) const { return handles.at(p) && SDL_GameControllerGetAttached(handles.at(p)); }
    QPointF cursor(int p) const { return cursors.at(p); }
    void resetGestures() { toggleHeld={{false,false}}; exitSince={{0,0}}; }
    MenuState menuState() const {
        auto read=[](SDL_GameController* handle) -> MenuState {
            if(!handle || !SDL_GameControllerGetAttached(handle)) return {};
            return {SDL_GameControllerGetButton(handle,SDL_CONTROLLER_BUTTON_A)!=0,
                    SDL_GameControllerGetButton(handle,SDL_CONTROLLER_BUTTON_DPAD_UP)!=0,
                    SDL_GameControllerGetButton(handle,SDL_CONTROLLER_BUTTON_DPAD_DOWN)!=0};
        };
        // After assignment only player 1 owns menu input, even after unplugging.
        if(assignedBefore) return read(handles[0]);
        // Bootstrap navigation remains available before the first assignment.
        for(int i=0;i<SDL_NumJoysticks();i++) if(SDL_IsGameController(i)) {
            auto* handle=SDL_GameControllerOpen(i);auto state=read(handle);
            if(handle) SDL_GameControllerClose(handle);
            return state;
        }
        return {};
    }
    ControlState read(int p, float seconds=1.0f/60) {
        ControlState out;
        auto* h=handles.at(p);
        // Never fall back to another device when disconnected or indices shift.
        if(!h || !SDL_GameControllerGetAttached(h)) { toggleHeld[p]=false; exitSince[p]=0; return out; }
        auto down=[&](SDL_GameControllerButton b){ return SDL_GameControllerGetButton(h,b)!=0; };
        bool toggle=down(SDL_CONTROLLER_BUTTON_BACK)&&down(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        bool exit=down(SDL_CONTROLLER_BUTTON_BACK)&&down(SDL_CONTROLLER_BUTTON_START);
        // Consume physical shortcut buttons before applying the user's mapping.
        auto consumed=[&](int button){
            return (toggle && (button==SDL_CONTROLLER_BUTTON_BACK || button==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) ||
                   (exit && (button==SDL_CONTROLLER_BUTTON_BACK || button==SDL_CONTROLLER_BUTTON_START));
        };
        for(int k=0;k<12;k++) if(buttons[k]>=0 && buttons[k]<SDL_CONTROLLER_BUTTON_MAX && !consumed(buttons[k]) && down(SDL_GameControllerButton(buttons[k]))) out.keys &= ~(1u<<k);
        float ax=SDL_GameControllerGetAxis(h,SDL_CONTROLLER_AXIS_LEFTX)/32768.0f;
        float ay=SDL_GameControllerGetAxis(h,SDL_CONTROLLER_AXIS_LEFTY)/32768.0f;
        if(ax>0.35f) out.keys &= ~(1u<<4);
        if(ax < -0.35f) out.keys &= ~(1u<<5);
        if(ay>0.35f) out.keys &= ~(1u<<7);
        if(ay < -0.35f) out.keys &= ~(1u<<6);
        float x=SDL_GameControllerGetAxis(h,SDL_CONTROLLER_AXIS_RIGHTX)/32768.0f;
        float y=SDL_GameControllerGetAxis(h,SDL_CONTROLLER_AXIS_RIGHTY)/32768.0f;
        auto dead=[](float v){ return std::abs(v)<0.18f?0.0f:v; };
        cursors[p].setX(qBound(0.0,cursors[p].x()+dead(x)*180*qMin(seconds,0.05f),255.0));
        cursors[p].setY(qBound(0.0,cursors[p].y()+dead(y)*180*qMin(seconds,0.05f),191.0));
        out.touch=coop::touch(qRound(cursors[p].x()),qRound(cursors[p].y()),down(SDL_CONTROLLER_BUTTON_RIGHTSTICK));
        out.toggle=toggle&&!toggleHeld[p]; toggleHeld[p]=toggle;
        if(exit) { if(!exitSince[p]) exitSince[p]=SDL_GetTicks(); out.exit=SDL_GetTicks()-exitSince[p]>=2000; }
        else exitSince[p]=0;
        return out;
    }
    void update() { SDL_PumpEvents(); SDL_GameControllerUpdate(); }
};
}
