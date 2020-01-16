# Harry the Tire-Stacking-Machine
These are codes for a 14-week robot project in course AER201 Engineering Design (2019 Jan.-Apr.) at the University of Toronto. [AER201 Course Website](http://aer201.aerospace.utoronto.ca/) <br/>
<img src="https://github.com/zhouyuq6/Harry-Tire-Stacking-Machine/blob/master/Document/pic%20of%20robot.jpg" align="center" width="500">

## Course Overview
Students worked in a team of three on a realistic request for a proposal. 
The team followed the standard developing procedure for an engineering project and provided their client (Prof. Emami) 
a project proposal, bi-weekly updates, a demonstration/competition of a fully functional prototype, and a final project report. 
## Project Overview
“Harry” was developed in response to a request for proposal (RFP) put forth by a tire
manufacturing company. The RFP presented the need for a tire-stacking robot no larger
than 50 x 50 x 50 cm3, weighing less than 8 kg, and costing less than $230 CAD. The
machine was to drive autonomously, detect poles, and deploy a specific number of tires
onto them based on the number of tires and the poles’ distance from the previously
detected pole.<br/>
Harry uses a differential drive system to move alongside a line of poles. It uses
four IR sensors to perceive poles and count the number of tires on them. It
deploys tires onto poles by passing it down a hook. The hook can positioned precisely
over the pole via an extension mechanism. Harry has maximum dimensions of
49.5 x 46.4 x 32 cm3 , weighs 5.4 kg, and costs $218.34 CAD.<br/>
A major limitation of Harry is imprecise hook positioning. The IR sensors which
determine hook positioning are sensitive to ambient light and distance to the pole being
detected. The hook positioning may be improved by implementing systemic changes
that enable the utilization of the more robust break beam sensors.
## Code Introduction
There are two parts of the code. `integrate_new.X` includes codes for PIC18F2550 and other peripherals on its develop board, which was designed for this course project.
PIC is the main controller of the robot and responsible for driving and deploying controls using DC and Stepper motors, the main algorithm of the robot, and user interface through I/O devices.
Another part `runFin` is for Arduino Nano, which receives information from IR sensors, detects the number of tires and position of poles, and then sends back to PIC.
`Document` includes project documentations for the client.
