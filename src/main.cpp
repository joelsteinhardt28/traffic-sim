#include "RoadViewer.hpp"
#include "CurveViewer.hpp"

int main() {
    // CurveViewer viewer(true, "Traffic Flow Simulation - Bezier Curve Viewer");
    RoadViewer viewer(true, "Traffic Flow Simulation - Road Network Viewer");
    viewer.run();
    return 0;
}