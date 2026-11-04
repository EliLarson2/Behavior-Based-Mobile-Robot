# Mobile Robot Demonstration Videos

This directory links to physical demonstrations of the mobile-robotics behaviors developed throughout the project.

The videos were originally recorded as engineering demonstrations during development and laboratory testing.

The GitHub documentation provides the technical explanation, while these videos show the behaviors operating on the physical robot.

---

## IR Obstacle Avoidance

**Video:** YOUTUBE_URL

### Demonstrated Behavior

The robot continuously moves through the environment while monitoring its left and right infrared obstacle sensors.

When an obstacle is detected:

```text
IR Detection
     |
     v
Stop
     |
     v
Reverse
     |
     v
Turn Away
     |
     v
Resume Cruise
```

The direction of the recovery maneuver depends on which IR sensor detects the obstacle.

This behavior served as the foundation for the later behavior-based control system.

---

## Light-Responsive Control

**Video:** [YOUTUBE_URL](https://www.youtube.com/shorts/lTN7m1CO_ic)

### Demonstrated Behavior

Two photoresistors measure light intensity on the left and right sides of the robot.

Different sensor-to-motor couplings produce different behaviors.

The experiments include:

- Excitatory same-side control
- Excitatory crossed control
- Inhibitory same-side control
- Inhibitory crossed control
- Orbiting a light source

These experiments demonstrate that relatively small changes in sensor-to-motor mapping can produce substantially different emergent robot behavior.

---

## Sonar Obstacle Avoidance

**Video:** YOUTUBE_URL

### Demonstrated Behavior

The ultrasonic sensor detects obstacles before they enter the range of the IR sensors.

```text
Distant Obstacle
       |
       v
Ultrasonic Detection
       |
       v
Gradual Steering Correction
       |
       v
Continue Navigation
```

Sonar enables smoother early avoidance, while the IR system remains active as a higher-priority close-range safety behavior.

---

## Autonomous Exploration

**Video:** [YOUTUBE_URL](https://www.youtube.com/shorts/R1nIKHBMqZE)

### Behavior Stack

```text
CRUISE
   <
SONAR_AVOID
   <
IR_AVOID
```

The robot continuously explores the environment without external control.

---

## Light-Seeking With Obstacle Avoidance

**Video:** [YOUTUBE_URL](https://www.youtube.com/shorts/vTtlNDzEta8)

### Behavior Stack

```text
CRUISE
   <
LIGHT_FOLLOW
   <
SONAR_AVOID
   <
IR_AVOID
```

The robot moves toward a light source while continuing to respond to obstacles.

This demonstration shows multiple goals operating simultaneously:

```text
Navigation Objective
        +
Obstacle Avoidance
        +
Collision Prevention
```

---

## Escape and Recovery

**Video:** [YOUTUBE_URL](https://www.youtube.com/shorts/nYax3AeUtw8)

### Behavior Stack

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

Repeated obstacle detections indicate that normal avoidance may not be making meaningful progress.

The robot responds with a stronger recovery maneuver:

```text
Repeated Collision Detection
            |
            v
       ESCAPE Trigger
            |
            v
          Backup
            |
            v
           Turn
            |
            v
   Resume Normal Behavior
```

This prevents the robot from remaining indefinitely in corners or repetitive obstacle-response loops.

