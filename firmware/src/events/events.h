#ifndef PARTICLE_EVENTS_H
#define PARTICLE_EVENTS_H

void fireEnvSensorsEvent(int temp, int humidity);
void fireBatteryEvent(int battery);
void fireTowerSendEvent();
void fireHelloPingEvent();
void fireHelloFromZEvent();

#endif