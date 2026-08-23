# Traffic Simulator

This is the general idea: I would like to build a simulator for traffic flow.
Visually, I envision something simple, a 2D map on which I can build street networks and simulate traffic, agent-based. At gateways which can be simple rectangles, agents are spawned with a certain destination gateway in mind. For the first version, it does not need to simulate a whole street network, rather I thought about simulating different highway intersection designs (cloverleaf, windmill, turbine, and so on).
The challenge is that for this to be useful, driving speeds need to be included. These can come from either user-defined speed limits but also just the natural curvature of the street. 
Streets crossing other streets in bridges, sometimes multiple bridges on top of each other need to be supported.