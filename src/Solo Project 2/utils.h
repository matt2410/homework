#pragma once

/* Returns 1 when a point lies inside a center-based rectangle, otherwise -1. */
int IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);
/* Returns nonzero when a point lies inside or on a circle. */
int IsCircleClicked(float circle_center_x, float circle_center_y, float diameter, float click_x, float click_y);
