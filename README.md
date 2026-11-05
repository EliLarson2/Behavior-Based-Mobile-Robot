# Behavior-Based Autonomous Mobile Robot

An autonomous differential-drive robot developed in Embedded C using **behavior-based control**.

The robot combines close-range infrared sensing, photoresistor-based light tracking, ultrasonic ranging, differential-drive control, and fixed-priority behavior arbitration. It can explore its environment, steer toward a light source, respond to obstacles at multiple ranges, and execute a simple recovery maneuver after repeated collision encounters.

The main challenge was not implementing any single sensor or behavior. It was getting several independently useful behaviors to work together on a physical robot without allowing navigation goals to override collision avoidance.

## Video Demonstrations

Physical demonstrations of the robot are available in [Video Demonstrations](Video_Demonstrations.md).

The demonstrations show the sensing and control behaviors developed throughout the project, including obstacle avoidance, light-responsive navigation, autonomous exploration, and recovery behavior.

---

## Final Integrated System

The cleaned implementation under [`src/`](src/) is the main portfolio version of the project.

The firmware separates the system into three main pieces:

- [`sensors.c`](src/sensors.c) acquires IR, photoresistor, and ultrasonic measurements.
- [`behaviors.c`](src/behaviors.c) evaluates those measurements and selects a response.
- [`main.c`](src/main.c) controls the update order, applies the final motor command, and displays the active behavior.

The shared types in [`robot_types.h`](src/robot_types.h) provide a clear interface between sensing and control:

```c
typedef struct
{
    BOOL left_IR;
    BOOL right_IR;

    ADC_SAMPLE photo_L;
    ADC_SAMPLE photo_R;

    float sonar_dist;
} SENSOR_DATA;

typedef struct
{
    int speed_L;
    int speed_R;
    ROBOT_STATE state;
} MOTOR_ACTION;
```

Sensor functions update one `SENSOR_DATA` object, while the behaviors read that shared state and modify one `MOTOR_ACTION`.

This keeps hardware acquisition out of the behavior functions. A behavior only needs to know what the robot currently senses and what motor response it wants—it does not need to independently configure the ADC, ping the ultrasonic sensor, or read the IR hardware.

That separation made the final system easier to integrate and debug because sensor problems, arbitration problems, and motor-command problems could be investigated independently.

---

## Fixed-Priority Behavior Arbitration

The robot uses a simple fixed-priority controller. Each pass through the main loop evaluates the behaviors from lowest to highest priority:

```c
CRUISE(&action);
LIGHT_FOLLOW(&action, &sensors);
SONAR_AVOID(&action, &sensors);
IR_AVOID(&action, &sensors);
ESCAPE(&action);
```

The resulting priority order is:

```text
CRUISE < LIGHT_FOLLOW < SONAR_AVOID < IR_AVOID < ESCAPE
```

`CRUISE` establishes the default forward command. If enough light is present, `LIGHT_FOLLOW` can change the left and right speeds to steer toward it. If sonar detects an obstacle, `SONAR_AVOID` takes precedence over normal navigation and light tracking.

The two highest-priority behaviors are more decisive. `IR_AVOID` directly stops the robot, backs up, and turns away from a close-range obstacle. `ESCAPE` executes a stronger recovery maneuver after repeated IR events.

This ordering reflects the role of each behavior:

| Behavior | Role |
| --- | --- |
| `CRUISE` | Default forward movement |
| `LIGHT_FOLLOW` | Goal-directed navigation |
| `SONAR_AVOID` | Earlier obstacle response |
| `IR_AVOID` | Immediate close-range collision response |
| `ESCAPE` | Recovery after repeated collision encounters |

The priority structure allows the robot to pursue a light source without disabling obstacle avoidance. Navigation provides the goal, but safety and recovery behaviors are allowed to interrupt it.

---

## Complementary Sensors

The robot uses three sensing methods because no single sensor handled every situation well.

The photoresistors provide directional information for light-seeking behavior. Ultrasonic sensing identifies obstacles before the robot reaches collision range, but its response depends on obstacle alignment. The left and right IR sensors provide a more decisive close-range trigger.

### Periodic Sensor Acquisition

Each sensing function maintains its own timer object. In the integrated application, the configured update intervals are:

| Sensor | Update interval |
| --- | ---: |
| IR sensors | 125 ms |
| Photoresistors | 150 ms |
| Ultrasonic sensor | 100 ms |

These are independent periodic updates rather than one simultaneous sensor read. When a sensor timer expires, its function updates the corresponding fields in `SENSOR_DATA`.

The ultrasonic system is initialized alongside the ADC:

```c
void sensors_init(void)
{
    ADC_open();
    ADC_set_VREF(ADC_VREF_AVCC);
    USONIC_open();
}
```

---

## Light Following

Light tracking uses two photoresistors connected through the robot's ADC subsystem.

During testing, the right sensor consistently read slightly higher than the left under similar lighting. The final acquisition code applies a correction before storing the right-side measurement:

```c
ADC_set_channel(ADC_CHAN3);
s->photo_R = ADC_sample() - 30;

ADC_set_channel(ADC_CHAN4);
s->photo_L = ADC_sample();
```

The light-following behavior first checks whether the average light level is above an activation threshold of `150`. If it is, the robot compares the left and right readings.

```c
unsigned int avg =
    (s->photo_L + s->photo_R) / 2;

if (avg > LIGHT_MIN)
{
    int diff =
        (int)s->photo_L - (int)s->photo_R;
```

A differential deadband of `±25` prevents the controller from reacting to small differences between the sensors. Without a deadband, minor measurement changes near equal illumination caused unnecessary steering corrections.

When the difference exceeds the deadband, the robot reduces the speed on one side while leaving the other side at the normal following speed. When the readings are close, both sides remain at the same speed.

This produces simple light-seeking behavior without requiring a map or planned path.

---

## Ultrasonic Obstacle Avoidance

The ultrasonic sensor provides longer-range obstacle awareness.

The acquisition code converts the sensor's timing result into microseconds and then estimates distance:

```c
SWTIME ticks = USONIC_ping();

unsigned long time_us =
    10UL * (unsigned long)ticks;

s->sonar_dist =
    0.01724f * (float)time_us;
```

During characterization, the sensor produced reasonable measurements when aimed directly at an obstacle:

| Actual distance | Measurement 1 | Measurement 2 | Wider angle |
| ---: | ---: | ---: | --- |
| 20 cm | 19.8 cm | 21.2 cm | No reading |
| 30 cm | 27.1 cm | 29.0 cm | No reading |
| 40 cm | 38.5 cm | 41.0 cm | No reading |

The poor wider-angle returns were an important result. They showed why sonar should not be treated as the robot's only collision sensor.

In the integrated controller, sonar acts as an early warning. When a valid distance falls below `20 cm`, `SONAR_AVOID` applies a steering correction before the robot reaches the close-range IR response.

```c
if (s->sonar_dist < 20.0f &&
    s->sonar_dist > 0.0f)
{
    a->speed_L = 250;
    a->speed_R = 350;
    a->state = SONAR_AVOIDING;
}
```

This behavior is intentionally less aggressive than the IR response. It attempts to steer away while the robot can still make a smooth correction.

---

## Close-Range IR Avoidance

The left and right IR sensors handle immediate obstacle encounters.

When either sensor activates, `IR_AVOID` stops the stepper outputs, backs the robot away, and turns based on which side detected the obstacle.

- A left-side detection produces a right turn.
- A right-side detection produces a left turn.
- If both sensors are active, the robot backs away.

Unlike light following and sonar steering, this is a predefined motion sequence rather than a continuous speed adjustment. Once a close obstacle is detected, the robot needs a decisive response instead of another gradual correction.

Each IR response also increments a repeated-event counter:

```c
if (s->left_IR || s->right_IR)
{
    a->state = IR_AVOIDING;
    repeated_ir_events++;

    STEPPER_stop(
        STEPPER_BOTH,
        STEPPER_BRK_ON);

    backup();
```

This counter supports the robot's recovery behavior.

---

## Escape and Recovery

Simple reactive avoidance can put a robot into a repeated cycle, particularly near corners or closely spaced obstacles.

The integrated controller uses a straightforward recovery rule: after at least three IR events, `ESCAPE` executes another backup and turns right before resetting the counter.

```c
if (repeated_ir_events >= 3)
{
    a->state = ESCAPING;

    backup();
    turn_right();

    repeated_ir_events = 0;
}
```

This is intentionally a simple recovery mechanism. It does not estimate the robot's position or formally determine whether the robot is stuck. It only recognizes that repeated close-range responses have occurred and applies a stronger maneuver than normal avoidance.

Even with that limitation, it improved the robot's ability to move away from situations where the basic IR response repeatedly encountered another obstacle.

---

## Motor Output

Once the lower-priority steering behaviors have established the left and right speeds, `apply_motor_action()` sends the command to the differential-drive stepper interface:

```c
STEPPER_move_stnb(
    STEPPER_BOTH,
    STEPPER_FWD, 10000,
    a->speed_L, 400,
    STEPPER_BRK_OFF,
    STEPPER_FWD, 10000,
    a->speed_R, 400,
    STEPPER_BRK_OFF);
```

The control system therefore has one final motor-action interface even though several behaviors may contribute to the decision.

This became especially useful during integration. Behaviors could be adjusted without duplicating the normal continuous-drive command throughout the code.

---

## Physical Debugging

Debugging the robot required more than inspecting variables in code. I needed to understand which behavior was active while the robot was moving.

The LCD displays the current state as one of:

- `CRUISE`
- `LIGHT FOLLOW`
- `SONAR AVOID`
- `IR AVOID`
- `ESCAPE`

This made it easier to distinguish between several possible problems during physical testing.

If the robot turned unexpectedly, I could determine whether the issue came from a sensor triggering incorrectly, a threshold being crossed, a higher-priority behavior activating, or the final motor response not matching the intended action.

That visibility was particularly useful when tuning the light deadband, sonar threshold, and behavior priority. A behavior can work correctly by itself and still interact poorly with the rest of the system.

---

## Development Milestones

The project was developed incrementally so individual sensing and control capabilities could be tested before integration.

| Milestone | Main addition |
| --- | --- |
| [`IR_avoidance/`](src/IR_avoidance/) | Cruise, close-range collision response, and early escape concepts |
| [`light_control/`](src/light_control/) | ADC photoresistor acquisition and Braitenberg-style light control |
| [`sonar_avoidance/`](src/sonar_avoidance/) | Ultrasonic ranging and longer-range obstacle response |
| [`integrated_tasks/`](src/integrated_tasks/) | Combined navigation, avoidance, arbitration, and recovery |

The milestone folders preserve the development progression. The cleaned files directly under `src/` are the main integrated implementation.

---

## What I Took Away From the Project

The most important lesson was that autonomous robotics is a systems-integration problem.

Each sensor had limitations. The photoresistors required calibration and a deadband. Sonar worked well at favorable angles but could miss angled obstacles. IR was useful at close range but reacted later. Combining them produced a more useful system than relying on any one sensor.

The same was true of the behaviors. Cruise, light following, sonar avoidance, and IR avoidance could each work independently while still producing unexpected results when combined. Their priority, trigger thresholds, steering strength, and recovery logic had to be tested on the physical robot.

This project gave me practical experience moving beyond individual sensor drivers and into the more difficult question: **how should sensing, decision-making, and actuation interact when the robot is operating in the real world?**

---

## What I Would Improve Today

The first improvement I would make is adding filtering and hysteresis around sensor-driven transitions. A moving median or similar filter could reduce occasional sonar variation, while separate activation and release thresholds could prevent behaviors from rapidly switching near a boundary.

I would also replace the repeated-IR-event counter with stronger evidence of whether the robot is actually making progress. Wheel motion or odometry could distinguish between several obstacle detections during normal exploration and a robot that is physically trapped.

Telemetry would make tuning more systematic. Logging photoresistor readings, sonar distance, IR states, selected behavior, and final motor commands would make it easier to reconstruct why the robot made a particular decision.

A longer-term revision could add odometry and state-aware navigation. I would keep the reactive behaviors for immediate safety, but combine them with a better estimate of robot motion and progress.

---

## Repository Structure

```text
.
├── README.md
├── Video_Demonstrations.md
└── src/
    ├── main.c
    ├── robot_types.h
    ├── sensors.c
    ├── sensors.h
    ├── behaviors.c
    ├── behaviors.h
    │
    ├── IR_avoidance/
    │   ├── README.md
    │   └── source.c
    │
    ├── light_control/
    │   ├── README.md
    │   └── source.c
    │
    ├── sonar_avoidance/
    │   ├── README.md
    │   └── source.c
    │
    └── integrated_tasks/
        ├── README.md
        └── source.c
```

---

## Background

This project was completed individually for **Mobile Robotics I at the University of Nebraska–Lincoln**.

The course milestones introduced the sensors and behaviors incrementally. The final integrated implementation brought them together into one autonomous robot with shared sensor data, fixed-priority behavior arbitration, differential-drive control, recovery logic, and on-robot debugging.

I include it in my portfolio because it shows the part of robotics I find most interesting: building individual sensing and control functions, then working through the much harder problem of making them behave reliably as one physical system.
