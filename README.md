# Behavior-Based Autonomous Mobile Robot

An autonomous mobile robot developed in Embedded C using **behavior-based control (BBC)**, combining infrared collision sensing, photoresistor light tracking, ultrasonic ranging, differential-drive control, and fixed-priority behavior arbitration.

The project was developed incrementally across several Mobile Robotics laboratory projects. Each stage introduced a new sensing or control capability before those capabilities were integrated into a final autonomous system capable of:

- Exploring an environment autonomously
- Following a light source
- Detecting obstacles at multiple ranges
- Avoiding collisions
- Recovering from corners and repeated obstacle encounters

The main engineering challenge was not simply implementing each behavior individually, but getting multiple behaviors with different priorities, sensors, and response characteristics to work together on a physical robot.

---

## Project Demonstrations

Physical demonstrations of the robot are available here:

**Video_Demonstrations/README.md**

The demonstrations include behaviors developed throughout the project such as:

- IR obstacle avoidance
- Light-responsive navigation
- Sonar-based obstacle avoidance
- Autonomous exploration
- Light-seeking with obstacle avoidance
- Escape and recovery behavior

---

# Development Progression

The final autonomous system was built incrementally.

| Stage | Capability Added |
|---|---|
| **IR Avoidance** | Close-range obstacle detection, cruise behavior, collision avoidance, and escape concepts |
| **Light Control** | ADC-based photoresistor sensing and Braitenberg-style sensor-to-motor control |
| **Sonar Avoidance** | Ultrasonic time-of-flight ranging and smoother long-range obstacle avoidance |
| **Integrated Tasks** | Fixed-priority behavior arbitration, exploration, light seeking, collision avoidance, and recovery |

This progression allowed individual behaviors to be developed and tested before combining them into the final system.

The original milestone code is preserved under:

```text
src/
├── IR_avoidance/
├── light_control/
├── sonar_avoidance/
└── integrated_tasks/
```

The cleaned integrated implementation is contained directly under `src/`.

---

# Final System Architecture

The robot uses several sensors to build a simple representation of its immediate environment.

```text
        IR Sensors
            |
            |
Photoresistors -----> SENSOR_DATA
            |              |
            |              |
Ultrasonic Sensor ---------+
                           |
                           v
                  +----------------+
                  | Robot Behaviors |
                  +----------------+
                    |    |    |    |
                    v    v    v    v
                 CRUISE LIGHT SONAR IR
                    \    |    |    /
                     \   |    |   /
                      v  v    v  v
                   +--------------+
                   | Arbitration  |
                   +--------------+
                          |
                          v
                    MOTOR_ACTION
                          |
                          v
                  Differential Drive
```

Sensor acquisition is separated from behavior logic so each behavior operates on the same shared sensor state.

---

# Sensor Data Structure

Sensor measurements are collected into a shared structure:

```c
typedef struct
{
    BOOL left_IR;
    BOOL right_IR;

    ADC_SAMPLE photo_L;
    ADC_SAMPLE photo_R;

    float sonar_dist;
} SENSOR_DATA;
```

This provides one interface between sensor acquisition and the behavior system.

Rather than having each behavior independently read hardware peripherals, sensor functions update `SENSOR_DATA`, and behaviors make decisions from that shared information.

---

# Sensors

## Infrared Sensors

The left and right IR sensors provide close-range obstacle detection.

IR sensing is used for immediate collision response, where the robot needs to take decisive action instead of gradually adjusting its trajectory.

Typical behavior:

```text
Obstacle on Left
       |
       v
Stop / Reverse
       |
       v
Turn Right
```

and vice versa for an obstacle on the right.

---

## Photoresistors

Two photoresistors are connected through the robot's ADC subsystem.

Early testing characterized the response under several lighting conditions:

| Condition | Right Sensor | Left Sensor |
|---|---:|---:|
| Ambient table | 771 / 3.73 V | 741 / 3.46 V |
| Ambient floor | 607 / 3.01 V | 543 / 2.73 V |
| Bright light | 1023 / 5.00 V | 1023 / 5.00 V |
| Sunlight | 1023 / 5.00 V | 1023 / 5.00 V |
| Dark room | 10 / 0.05 V | 13 / 0.06 V |

Testing showed that the right sensor consistently read slightly higher than the left.

A calibration offset was therefore introduced in later versions of the sensor-acquisition code.

```c
ADC_set_channel(ADC_CHAN3);
s->photo_R = ADC_sample() - 30;

ADC_set_channel(ADC_CHAN4);
s->photo_L = ADC_sample();
```

This became important once the sensors were used for differential steering.

---

## Ultrasonic Sensor

The ultrasonic sensor provides longer-range obstacle detection using time-of-flight.

```c
ticks = USONIC_ping();

time_us =
    10 * ((unsigned long int)ticks);

s->sonar_dist =
    0.01724f * time_us;
```

The conversion estimates distance from the measured ultrasonic round-trip time.

### Sonar Characterization

| Actual Distance | Measurement 1 | Measurement 2 | Wider Angle |
|---:|---:|---:|---|
| 20 cm | 19.8 cm | 21.2 cm | No Reading |
| 30 cm | 27.1 cm | 29.0 cm | No Reading |
| 40 cm | 38.5 cm | 41.0 cm | No Reading |

The sonar sensor worked well when aligned with an obstacle but became significantly less reliable at wider angles.

This made sonar useful for **early warning and smooth steering**, while the IR sensors remained necessary for reliable close-range collision avoidance.

---

# Behavior-Based Control

The final robot uses **Behavior-Based Control** rather than one large centralized control algorithm.

Each behavior performs one relatively simple task.

Examples include:

```text
CRUISE
LIGHT_FOLLOW
SONAR_AVOID
IR_AVOID
ESCAPE
```

Complex robot behavior emerges from the interaction and prioritization of these simpler behaviors.

---

# Servo vs. Ballistic Behaviors

Two general types of behavior are used.

## Servo Behaviors

Servo behaviors continuously adjust outputs based on sensor measurements.

Examples:

- `LIGHT_FOLLOW`
- `SONAR_AVOID`

These behaviors produce smoother corrections.

## Ballistic Behaviors

Ballistic behaviors execute a predefined motion sequence when triggered.

Examples:

- `IR_AVOID`
- `ESCAPE`

A typical ballistic obstacle response might be:

```text
Stop
  |
  v
Reverse
  |
  v
Turn
  |
  v
Resume Normal Behavior
```

These behaviors are useful when an immediate, decisive maneuver is required.

---

# Fixed-Priority Arbitration

The final system uses the following behavior priority:

```text
CRUISE
   <
LIGHT_FOLLOW
   <
SONAR_AVOID
   <
IR_AVOID
   <
ESCAPE
```

Behaviors execute from lowest to highest priority.

A lower-priority behavior may establish an initial motor command, but a higher-priority behavior can overwrite that command if its triggering conditions are active.

For example:

```text
CRUISE says:
"Drive forward"

LIGHT_FOLLOW says:
"Turn toward the light"

SONAR_AVOID says:
"Obstacle ahead, steer away"

IR_AVOID says:
"Collision imminent, reverse"

ESCAPE says:
"We are stuck, execute recovery"
```

Safety and recovery behaviors therefore override normal navigation.

---

# Light Following

Light-seeking navigation uses differential sensing between the left and right photoresistors.

```c
void LIGHT_FOLLOW(
    volatile MOTOR_ACTION *a,
    volatile SENSOR_DATA *s)
{
    unsigned int avg =
        (s->photo_L + s->photo_R) / 2;

    if (avg > 150)
    {
        int diff =
            s->photo_L - s->photo_R;

        if (diff > 25)
        {
            a->speed_L = 250;
            a->speed_R = 350;
        }
        else if (diff < -25)
        {
            a->speed_L = 350;
            a->speed_R = 250;
        }
        else
        {
            a->speed_L = 350;
            a->speed_R = 350;
        }
    }
}
```

Two important parameters were introduced:

| Parameter | Value | Purpose |
|---|---:|---|
| Activation threshold | 150 | Determines whether sufficient light is present |
| Differential deadband | ±25 | Prevents unnecessary steering when sensors are nearly equal |

Without the deadband, small fluctuations around equal light intensity caused the robot to oscillate between steering directions.

---

# Sonar Avoidance

The sonar behavior provides smoother obstacle avoidance before the robot reaches the IR collision range.

```c
void SONAR_AVOID(
    volatile MOTOR_ACTION *a,
    volatile SENSOR_DATA *s)
{
    if (s->sonar_dist < 20.0f &&
        s->sonar_dist > 0.0f)
    {
        a->speed_L = 250;
        a->speed_R = 350;
    }
}
```

The threshold was set to approximately:

```text
20 cm
```

This creates a gradual steering response rather than immediately executing a reverse-and-turn maneuver.

---

# IR Avoidance

IR avoidance handles obstacles at close range.

Unlike `SONAR_AVOID`, this behavior is ballistic.

The response depends on which IR sensor detects the obstacle.

| Detection | Response |
|---|---|
| Left IR | Reverse and turn right |
| Right IR | Reverse and turn left |
| Both IR sensors | Reverse away from obstacle |

Because immediate collision avoidance is more important than normal navigation, `IR_AVOID` receives higher priority than both light following and sonar avoidance.

---

# Escape and Recovery

A major failure mode in simple autonomous robots occurs when repeated obstacle responses place the robot into a loop.

For example:

```text
Obstacle
   |
Avoid
   |
Obstacle
   |
Avoid
   |
Obstacle
   |
   ...
```

The `ESCAPE` behavior was added to detect repeated collision events and execute a stronger recovery maneuver.

```text
Repeated IR Detections
        |
        v
Virtual Stasis Detected
        |
        v
Backup
        |
        v
Turn
        |
        v
Resume Normal Behaviors
```

This allows the robot to recover from corners and other situations where basic reactive avoidance is insufficient.

---

# Final Control Loop

The final system combines periodic sensor acquisition with ordered behavior execution:

```c
while (1)
{
    IR_sense(&sensors, 125);
    Photo_sense(&sensors, 150);
    SONAR_sense(&sensors, 100);

    CRUISE(&action);
    LIGHT_FOLLOW(&action, &sensors);
    SONAR_AVOID(&action, &sensors);
    IR_AVOID(&action, &sensors);
    ESCAPE(&action);

    LCD_debug(&action);

    TMRSRVC_delay(100);
}
```

Sensor update periods are intentionally not identical.

| Sensor | Update Period |
|---|---:|
| IR | 125 ms |
| Photoresistors | 150 ms |
| Sonar | 100 ms |

The timing reflects the different roles and limitations of the sensors.

---

# Autonomous Tasks

## 1. Autonomous Exploration

The robot continuously explores the environment while avoiding obstacles.

```text
CRUISE
   <
SONAR_AVOID
   <
IR_AVOID
```

`CRUISE` provides normal forward motion.

`SONAR_AVOID` provides early steering corrections.

`IR_AVOID` provides the final close-range collision response.

---

## 2. Light-Seeking Navigation

The robot searches for and follows a light source while continuing to avoid obstacles.

```text
CRUISE
   <
LIGHT_FOLLOW
   <
SONAR_AVOID
   <
IR_AVOID
```

This allows the robot to pursue a navigation objective without disabling safety behaviors.

---

## 3. Escape and Recovery

The final task adds recovery behavior:

```text
CRUISE
   <
LIGHT_FOLLOW
   <
SONAR_AVOID
   <
IR_AVOID
   <
ESCAPE
```

This allows the robot to recover when lower-level behaviors repeatedly fail to make progress.

---

# Debugging

LCD output was used to show the currently active behavior.

```c
if (a->state == IR_AVOIDING)
{
    LCD_printf("IR AVOID\n");
}
else if (a->state == SONAR_AVOIDING)
{
    LCD_printf("SONAR AVOID\n");
}
else if (a->state == LIGHT_HOMING)
{
    LCD_printf("LIGHT FOLLOW\n");
}
else
{
    LCD_printf("CRUISE\n");
}
```

This was particularly useful when debugging arbitration.

If the robot moved incorrectly, the LCD allowed me to distinguish between:

```text
Bad Sensor Reading
       vs.
Wrong Behavior Trigger
       vs.
Incorrect Priority
       vs.
Motor-Control Problem
```

That made physical debugging substantially easier.

---

# Engineering Challenges

## Sonar Noise

Ultrasonic measurements occasionally fluctuated or failed at unfavorable angles.

Periodic sampling and threshold tuning reduced unstable responses.

## Light-Sensor Calibration

The two photoresistors did not produce identical measurements under the same lighting conditions.

A calibration offset was applied to reduce this mismatch.

## Light-Tracking Oscillation

Small left/right changes near equal light levels caused steering jitter.

A deadband reduced unnecessary corrections.

## Behavior Interaction

Individually correct behaviors did not automatically produce a correct overall robot.

Priority order and steering strength had to be tuned experimentally.

## Stuck Conditions

Reactive obstacle avoidance sometimes produced repeated response loops.

The escape behavior introduced a higher-level recovery mechanism.

---

# Development Progression

The project was developed through several milestones.

```text
Basic IR Stop
      |
      v
IR Avoidance + Cruise
      |
      v
Adaptive Escape
      |
      v
Photoresistor ADC Measurement
      |
      v
Braitenberg Light Control
      |
      v
Ultrasonic Ranging
      |
      v
Sonar Avoidance
      |
      v
Behavior Arbitration
      |
      v
Autonomous Exploration
      |
      v
Light-Seeking Navigation
      |
      v
Escape and Recovery
```

The milestone implementations are retained under:

```text
src/
├── IR_avoidance/
├── light_control/
├── sonar_avoidance/
└── integrated_tasks/
```

---

# Repository Structure

```text
Behavior-Based-Mobile-Robot/
│
├── README.md
│
├── src/
│   ├── main.c
│   ├── robot_types.h
│   ├── sensors.c
│   ├── sensors.h
│   ├── behaviors.c
│   ├── behaviors.h
│   │
│   ├── IR_avoidance/
│   │   ├── README.md
│   │   └── source.c
│   │
│   ├── light_control/
│   │   ├── README.md
│   │   └── source.c
│   │
│   ├── sonar_avoidance/
│   │   ├── README.md
│   │   └── source.c
│   │
│   └── integrated_tasks/
│       ├── README.md
│       └── source.c
│
└── Video_Demonstrations/
    └── README.md
```

---

# What I Learned

This project provided practical experience with:

- Embedded C
- Autonomous robotics
- ADC sensor acquisition
- Infrared sensing
- Ultrasonic time-of-flight ranging
- Photoresistor calibration
- Differential-drive control
- Braitenberg vehicles
- Behavior-based control
- Servo behaviors
- Ballistic behaviors
- Fixed-priority arbitration
- Sensor timing
- Threshold tuning
- Deadbands
- Physical-system debugging
- Autonomous recovery logic
- Integrating independently developed subsystems

The most important lesson was that autonomous robotics is fundamentally a **systems-integration problem**.

A sensor, behavior, or controller can work correctly in isolation while still producing unexpected behavior when combined with the rest of the system.

---

# Future Improvements

If I continued developing the robot, I would investigate:

### Sensor Filtering

Add median or moving-average filtering to ultrasonic measurements.

### Hysteresis

Use separate activation and deactivation thresholds for behaviors to reduce oscillation.

### Odometry

Use wheel-motion estimates to better detect when the robot is physically stuck.

### Telemetry

Stream:

- Sensor values
- Active behavior
- Motor commands
- Sonar distance
- Light readings

over a serial connection for logging and analysis.

### Dynamic Arbitration

Investigate behavior scoring or dynamic arbitration rather than relying entirely on fixed priorities.

### Mapping and Localization

Add:

- Encoders
- Odometry
- IMU data
- Mapping
- Localization

to progress from reactive navigation toward state-aware autonomous navigation.

---

# Background

This project was developed for **Mobile Robotics I** at the University of Nebraska-Lincoln.

The project was completed individually and developed incrementally across several laboratory assignments.

The final implementation focuses on **integration and emergent behavior**, combining previously developed sensor and control modules into a complete autonomous robotic system.
