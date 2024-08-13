- Resize vehicle after every move which changes the centerpoint. We want to
  keep the vehicle in the center as much as possible.
- Add a button to save the vehicle to a file (trivial)
- Support saving vehicle to an STFS file for use on the console
- Add a new bitmask at 6 bits per cell for part connectivity. Each of the 6
  bits indicate if one of its sides is connectable.

- Look into controller support via GLFW. I think D-pads are exposed as "hat"
  switches, the triggers are one-axis "joysticks", and the actual sticks are
  2-axis joysticks.
- Consider making a 6-axis snapping camera view like the game
- Try to improve ease of depth perception (maybe some shadows?)

# Selection
Other tweaks:
- In scenarios where we move the entire bitmask, we could implement it with
  bitshifts & memcpy()
- When a part is selected, just delete all its cells from occupancy. Why are we
  clearing the entire grid??

