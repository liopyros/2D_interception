# 2D_interception
After not touching C++ since high school, I decided to revisit the programming language to create a 2D Interception program in the Command Prompt.

A target object (red) is given a velocity and acceleration vector proportional to the clicking and dragging motion of my mouse. The object proceeds to "launch" and accelerates until it reaches a predefined height, after which it is then only subject to gravity (meant to mimic a missile becoming ballistic after burnout). While that object is in free fall, clicking on the intercepting object (green) calculates and initiates the velocity and acceleration vectors required to hit the target (cyan indicates non-gravitational acceleration for both objects).
